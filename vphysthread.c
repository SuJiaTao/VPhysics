
/* ========== <vphysthread.c>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal worker thread logic for physics thread			*/


/* ========== INCLUDES							==========	*/
#include "vphysthread.h"
#include "vspacepart.h"
#include "vcollision.h"
#include <math.h>
#include <float.h>
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

/* ========== HELPER FUNCS						==========	*/
static void PXApplyFriction(vPPhysical phys, vFloat coeff)
{
	vPXVectorMultiply(&phys->velocity, (1.0f - coeff));
}

static void PXApplyForce(vPPhysical phys, vVect forceVect)
{
	/* ensure force doesn't exceed max */
	forceVect.x = min(forceVect.x, VPHYS_MAX_FORCE_COMPONENT);
	forceVect.x = max(forceVect.x, -VPHYS_MAX_FORCE_COMPONENT);
	forceVect.y = min(forceVect.y, VPHYS_MAX_FORCE_COMPONENT);
	forceVect.y = max(forceVect.y, -VPHYS_MAX_FORCE_COMPONENT);

	/* apply force */
	vPXVectorAddV(&phys->acceleration, forceVect);
}

static void PXPushApartObjects(vPPhysical source, vPPhysical target,
	vVect pushVector, vFloat pushVectorMag)
{
	/* get push factor of each */
	vFloat totalMass = source->mass + target->mass;
	vFloat sPushFact = source->mass / totalMass;
	vFloat tPushFact = target->mass / totalMass;

	/* if both masses zero, factors should be equal */
	if (isinf(sPushFact) || isnan(sPushFact)) sPushFact = 1.0f;
	if (isinf(tPushFact) || isnan(tPushFact)) tPushFact = 1.0f;

	/* apply displacement */
	vVect srcDIVect = pushVector;
	vVect trgDIVect = pushVector;
	vPXVectorMultiply(&srcDIVect, pushVectorMag * POS_DEINTERSECT_COEFF * sPushFact);
	vPXVectorMultiply(&trgDIVect, -pushVectorMag * POS_DEINTERSECT_COEFF * tPushFact);
	vPXVectorAddV(&source->transform.position, srcDIVect);
	vPXVectorAddV(&target->transform.position, trgDIVect);

	/* apply force */
	vVect srcVIVect = pushVector;
	vVect trgVIVect = pushVector;
	vPXVectorMultiply(&srcVIVect, pushVectorMag * VEL_DEINTERSECT_COEFF);
	vPXVectorMultiply(&trgVIVect, -pushVectorMag * VEL_DEINTERSECT_COEFF);
	PXApplyForce(source, srcVIVect);
	PXApplyForce(target, trgVIVect);
}

static void PXCalculateCollisionFriction(vPPhysical source, vPPhysical target)
{
	/* if velocity is low enough, use static friction coeffs */
	vFloat velDiff = vPXFastFabs(
		vPXVectorMagnitudeV(source->velocity)
		- vPXVectorMagnitudeV(target->velocity));

	vFloat sFricCoeff, tFricCoeff;
	if (velDiff <= STATICFRICTION_VELOCITY)
	{
		sFricCoeff = source->material.staticFriction;
		tFricCoeff = target->material.staticFriction;
	}
	else
	{
		sFricCoeff = source->material.dynamicFriction;
		tFricCoeff = target->material.dynamicFriction;
	}

	vFloat avgFricCoeff = (sFricCoeff + tFricCoeff) * 0.5f;
	PXApplyFriction(source, avgFricCoeff);
	PXApplyFriction(target, avgFricCoeff);
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

/* ========== ITERATE FUNCS			==========	*/
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

	/* if nothing in the partition is moving around, skip */
	if (part->totalVelocity < PARITION_MINVELOCITY) return;

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

			/* push objects apart based on pushvector */
			PXPushApartObjects(source, target, pushVector, pushVectorMag);

			/* apply friction based on each object's friction coeff */
			PXCalculateCollisionFriction(source, target);

			/* call collision funcs if exists */
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

	/* call object's update func if exists */
	if (phys->updateFunc != NULL)
		phys->updateFunc(phys);

	/* apply object drag */
	PXApplyFriction(phys, phys->material.drag);

	/* update object velocity */
	vPXVectorAddV(&phys->velocity, phys->acceleration);

	/* ensure velocity doesn't exceed max */
	phys->velocity.x = min(phys->velocity.x,  VPHYS_MAX_VELOCITY_COMPONENT);
	phys->velocity.x = max(phys->velocity.x, -VPHYS_MAX_VELOCITY_COMPONENT);
	phys->velocity.y = min(phys->velocity.y, VPHYS_MAX_VELOCITY_COMPONENT);
	phys->velocity.y = max(phys->velocity.y, -VPHYS_MAX_VELOCITY_COMPONENT);

	/* update object position */
	vPXVectorAddV(&phys->transform.position, phys->velocity);
}

/* ========== RENDER THREAD FUNCTIONS			==========	*/
void vPXT_initFunc(vPWorker worker, vPTR workerData, vPTR input)
{
	
}

void vPXT_exitFunc(vPWorker worker, vPTR workerData)
{

}

ULONGLONG __pxCycleTimeTaken = 0;
void vPXT_cycleFunc(vPWorker worker, vPTR workerData)
{
	/* reset profiler accumulator */
	if (worker->cycleCount % PROFILER_REFRESH_INTERVAL == 0)
	{
		__pxCycleTimeTaken /= PROFILER_REFRESH_INTERVAL;
		vPXDebugLogFormatted("Physics Tick Rate: %d\n",
			__pxCycleTimeTaken);
		__pxCycleTimeTaken = 0;
	}

	ULONGLONG cycleStartTime = GetTickCount64();

	/* if in debug mode, draw axis lines */
	if (_vphys.debugMode == TRUE)
	{
		/* x axis line*/
		vGDrawLineF(-0xFFFF, 0, 0xFFFF, 0, vGCreateColorB(0, 0, 255, 255), 5.0f);

		/* y axis line */
		vGDrawLineF(0, -0xFFFF, 0, 0xFFFF, vGCreateColorB(255, 0, 0, 255), 5.0f);
	}

	/* clear all partitions */
	PXPartResetPartitions();

	/* setup all objects for collision calculations */
	/* (refer to function for implementation)		*/
	vDBufferIterate(_vphys.physObjectList, vPXPhysicalListIterateSetupFunc, NULL);

	/* do collision calculations and de-intersect objects */
	vDBufferIterate(_vphys.partitions, vPXPartitionIterateCollisionFunc, NULL);

	/* apply all dynamics from forces accumulated during */
	/* collision detection and user-defined update func   */
	vDBufferIterate(_vphys.physObjectList, vPXPhysicalListIterateDoDynamicsFunc, NULL);

	/* debug draw all partitions */
	if (_vphys.debugMode == TRUE)
	{
		vDBufferIterate(_vphys.partitions, vPXDebugDrawPartitionIterateFunc, NULL);
	}

	__pxCycleTimeTaken += (GetTickCount64() - cycleStartTime);
}