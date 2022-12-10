
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


/* ========== INTERNAL STRUCTS					==========	*/
typedef struct PXCollisionInfo
{
	vPPhysical collidedObject;
	vVect  pushBackVector;
	vFloat pushBackMagnitude;
} PXCollisionInfo, *PPXCollisionInfo;


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

static void PXMomentumExchange(vPPhysical source, vPPhysical target)
{
	/* get initial velocities  */
	vVect v1 = source->velocity;
	vVect v2 = target->velocity;

	/* transform so that v2 = 0 */
	vPXVectorAddV(&v1, vPXVectorMultiplyCopy(v2, -1.0f));

	/* find new v1 and v2 */
	vVect v1Prime = vPXVectorMultiplyCopy(v1,
		(source->mass - target->mass) / (source->mass + target->mass));
	vVect v2Prime = vPXVectorMultiplyCopy(v1,
		(source->mass * 2.0f) / (source->mass + target->mass));

	/* shift back so that v2 no longer equals 0 */
	vPXVectorAddV(&v1Prime, v2);
	vPXVectorAddV(&v2Prime, v2);

	/* dampen based on bounciness */
	vPXVectorMultiply(&v1Prime, source->material.bounciness);
	vPXVectorMultiply(&v2Prime, target->material.bounciness);

	/* assign new velocities */
	source->velocity = v1Prime;
	target->velocity = v2Prime;
}

static void PXPushApartObjects(vPPhysical source, vPPhysical target,
	vVect pushVector, vFloat pushVectorMag)
{
	/* get push factor of each */
	vFloat totalMass = source->mass + target->mass;
	vFloat sPushFact = source->mass / totalMass;
	vFloat tPushFact = target->mass / totalMass;

	/* apply displacement */
	vVect srcDIVect = 
		vPXVectorMultiplyCopy(pushVector, pushVectorMag * POS_DEINTERSECT_COEFF * sPushFact);
	vVect trgDIVect = 
		vPXVectorMultiplyCopy(pushVector, -pushVectorMag * POS_DEINTERSECT_COEFF * tPushFact);
	
	vPXVectorAddV(&source->transform.position, srcDIVect);
	vPXVectorAddV(&target->transform.position, trgDIVect);
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

	/* collision info list */
	PPXCollisionInfo colList = vAllocZeroed(sizeof(PXCollisionInfo) * part->useage);

	/* loop all objects */
	for (int i = 0; i < part->useage; i++)
	{
		/* clear collision list */
		vZeroMemory(colList, sizeof(PXCollisionInfo) * part->useage);
		vUI32 colListUseage = 0;

		vPPhysical source = part->list[i];

		/* loop every other object (no self collision) */
		for (int j = 0; j < part->useage; j++)
		{
			if (i == j) continue;

			vPPhysical target = part->list[j];
			PPXCollisionInfo colInfo = colList + colListUseage;

			/* pre-check collision */
			if (vPXDetectCollisionPreEstimate(source, target) == FALSE) continue;

			/* do proper collision detection */
			vVect pushBackVec; vFloat pushBackMag;
			vBOOL colResult = vPXDetectCollisionSAT(source, target,
				&pushBackVec, &pushBackMag);
			if (colResult == FALSE) continue;

			/* store collision data */
			colInfo->collidedObject    = target;
			colInfo->pushBackVector    = pushBackVec;
			colInfo->pushBackMagnitude = pushBackMag;
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

	__pxCycleTimeTaken += (GetTickCount64() - cycleStartTime);

	/* debug draw all partitions */
	if (_vphys.debugMode == TRUE)
	{
		vDBufferIterate(_vphys.partitions, vPXDebugDrawPartitionIterateFunc, NULL);
	}
}