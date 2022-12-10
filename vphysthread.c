
/* ========== <vphysthread.c>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal worker thread logic for physics thread			*/


/* ========== INCLUDES							==========	*/
#include "vphysthread.h"
#include "vspacepart.h"
#include "vcollision.h"
#include <stdio.h>


/* ========== DEBUG DRAW BOUNDS FUNC			==========	*/
static void vPXDebugDrawPartitionIterateFunc(vHNDL dBuffer, vPPXPartition part, vPTR input)
{
	/* skip unused partitions */
	if (part->inUse == FALSE) return;

	/* calculate bounding box of partition */
	vFloat rootX = part->x * _vphys.partitionSize;
	vFloat rootY = part->y * _vphys.partitionSize;
	vGRect pBound = vGCreateRect(rootX, rootX + _vphys.partitionSize,
		rootY, rootY + _vphys.partitionSize);

	/* convert to mesh and draw */
	vPosition drawMesh[4];
	vPXBoundToMesh(drawMesh, pBound);
	vGDrawLinesConnected(drawMesh, 4, vGCreateColorB(PARTITION_COLORb),
		PARTITION_LINESIZE);
}

static void vPXDebugDrawBounds(void)
{

}

/* ========== WORLDBOUND GENERATION				==========	*/
static void vPXGenerateWorldBounds(vPPhysical phys)
{
	/* create mesh using bounds */
	vPXBoundToMesh(phys->worldBound.mesh, phys->bound);

	/* transform each vertex */
	for (int i = 0; i < 4; i++)
	{
		vPXVectorTransform(phys->worldBound.mesh + i, phys->anticipatedPos,
			phys->transform.scale, phys->transform.rotation);
	}

	/* calculate bounding box */
	float minX, maxX; minX = maxX = phys->worldBound.mesh[0].x;
	float minY, maxY; minY = maxY = phys->worldBound.mesh[0].y;

	for (int i = 0; i < 4; i++)
	{
		minX = min(minX, phys->worldBound.mesh[i].x);
		maxX = max(maxX, phys->worldBound.mesh[i].x);
		minY = min(minY, phys->worldBound.mesh[i].y);
		maxY = max(maxY, phys->worldBound.mesh[i].y);
	}

	/* create bounding box */
	phys->worldBound.boundingBox = vGCreateRect(minX, maxX, minY, maxY);

	/* set bounding box dims */
	phys->worldBound.boundingBoxDims = vCreatePosition(maxX - minX, maxY - minY);

	/* calculate "center" */
	phys->worldBound.center = vPXVectorAverageV(phys->worldBound.mesh, 4);
}

/* ========== SETUP OBJECTS FOR CYCLE		==========	*/
static void vPXPhysicalListIterateSetupFunc(vHNDL dbHndl, vPPhysical* objectPtr, 
	vPTR input)
{
	vPPhysical pObj = *objectPtr;

	/* if object is inactive, skip */
	if (pObj->properties.isActive == FALSE) return;

	/* increment object's age */
	pObj->age++;

	/* clear object acceleration */
	pObj->acceleration = vPXCreateVect(0.0f, 0.0f);

	/* update renderable transform (if applicable) */
	if (pObj->renderableTransformOverride == TRUE
		&& pObj->renderableCache != NULL )
	{
		vGLock();
		pObj->renderableCache->transform = pObj->transform;
		vGUnlock();
	}

	/* ENSURE ALL VALUES ARE VALID */
	vPXEnforceEpsilonV(&pObj->transform.position);
	vPXEnforceEpsilonV(&pObj->velocity);

	/* generate anticipated position */
	pObj->anticipatedPos = pObj->transform.position;
	vPXVectorAddV(&pObj->anticipatedPos, pObj->velocity);

	/* generate object's world bounds */
	vPXGenerateWorldBounds(pObj);

	/* assign object to partitions */
	PXPartObjectOrangizeIntoPartitions(pObj);

	/* if debug mode, draw bounds */
	if (_vphys.debugMode == TRUE)
	{
		/* draw bounds of world-mesh */
		vGDrawLinesConnected(pObj->worldBound.mesh, 4,
			vGCreateColorB(BOUND_MESH_COLORb), BOUND_MESH_LINESIZE);

		/* draw center of world mesh */
		vGDrawCross(pObj->worldBound.center, 0.05f, 
			vGCreateColorB(BOUND_MESH_COLORb), BOUND_MESH_LINESIZE);

		/* draw velocity vector */
		vVect velVectDraw = pObj->worldBound.center;
		vPXVectorAddV(&velVectDraw, pObj->velocity);
		vGDrawLineV(pObj->worldBound.center, velVectDraw, 
			vGCreateColorB(BOUND_MESH_COLORb), BOUND_MESH_LINESIZE);

		/* draw bounding box of mesh */
		vPVect boundingBoxMesh[4];
		vPXBoundToMesh(boundingBoxMesh, pObj->worldBound.boundingBox);
		vGDrawLinesConnected(boundingBoxMesh, 4,
			vGCreateColorB(BOUND_BOX_COLORb), BOUND_BOX_LINESIZE);
	}
}

static void vPXPartitionIterateCollisionFunc(vHNDL dbHndl, vPPXPartition part,
	vPTR input)
{
	/* if partition has 1 element or less, skip */
	if (part->useage <= 1) return;

	/* loop through each element in the partition and collide with every	*/
	/* other object, accumulate de-intersection vectors						*/
	for (int i = 0; i < part->useage; i++)
	{
		/* get source object */
		vPPhysical source = part->list[i];

		for (int j = 0; j < part->useage; j++)
		{
			/* avoid self collision */
			if (i == j) continue;

			/* get target object */
			vPPhysical target = part->list[j];

			/* if not close, don't consider collision */
			if (vPXDetectCollisionPreEstimate(source, target) == FALSE) continue;

			/* get de-intersection vector */
			vVect pushVector;
			vFloat pushVectorMag;
			vPXDetectCollisionSAT(source, target, &pushVector, &pushVectorMag);
			
			/* calculate delta v required to cancel v components */
			/* in opposite direction of de-intersection vector	 */
			vVect velocityCorrection = pushVector;
			vFloat dot = vPXVectorDotProduct(source->velocity, velocityCorrection);
			vPXVectorMultiply(&velocityCorrection, -dot);

			/* add delta v to accelration accumulator */
			vPXVectorAddV(&source->acceleration, velocityCorrection);

			/* de-intersect each object by half the vector */
			vVect srcDIVect = pushVector;
			vVect trgDIVect = pushVector;
			vPXVectorMultiply(&srcDIVect,  pushVectorMag * 0.5f);
			vPXVectorMultiply(&trgDIVect, -pushVectorMag * 0.5f);
			vPXVectorAddV(&source->transform.position, srcDIVect);
			vPXVectorAddV(&target->transform.position, trgDIVect);

			/* call collision func if exists */
			if (source->collisionFunc != NULL)
				source->collisionFunc(source, target);

			if (target->collisionFunc != NULL)
				target->collisionFunc(target, source);
		}
	}
}

static void vPXPhysicalListIterateDoDynamicsFunc(vHNDL dbHndl, vPPhysical* objectPtr,
	vPTR input)
{
	vPPhysical phys = *objectPtr;
}

/* ========== RENDER THREAD FUNCTIONS			==========	*/
void vPXT_initFunc(vPWorker worker, vPTR workerData, vPTR input)
{
	
}

void vPXT_exitFunc(vPWorker worker, vPTR workerData)
{

}

void vPXT_cycleFunc(vPWorker worker, vPTR workerData)
{
	/* clear all partitions */
	PXPartResetPartitions();

	/* setup all objects for collision calculations */
	/* (refer to function for implementation)		*/
	vDBufferIterate(_vphys.physObjectList, vPXPhysicalListIterateSetupFunc, NULL);

	/* do collision calculations and de-intersect objects */
	vDBufferIterate(_vphys.partitions, vPXPartitionIterateCollisionFunc, NULL);

	/* debug draw all partitions */
	if (_vphys.debugMode == TRUE)
	{
		vDBufferIterate(_vphys.partitions, vPXDebugDrawPartitionIterateFunc, NULL);
	}
}