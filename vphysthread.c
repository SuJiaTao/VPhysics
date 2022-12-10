
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

typedef struct PXPushbackInfo
{
	vVect accumulator;
	vUI32 collisionCount;
} PXPushbackInfo, *PPXPushbackInfo;


/* ========== DEBUG DRAW FUNCS				==========	*/
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

static void PXDebugDrawBound(vPPhysical pObj)
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

/* ========== HELPER FUNCS						==========	*/
static void PXApplyFriction(vPPhysical phys, vFloat coeff)
{
	vPXVectorMultiply(&phys->velocity, (1.0f - coeff));
}

static vFloat PXWeightByMass(vFloat sVal, vFloat tVal,
	vPPhysical source, vPPhysical target)
{
	vFloat massTotal = source->mass + target->mass;
	sVal *= (source->mass / massTotal);
	tVal *= (target->mass / massTotal);
	return sVal + tVal;
}

static vVect PXCalculateMomentumTransferVect(vPPhysical source, vPPhysical target)
{
	/* get initial velocities  */
	vVect v1 = source->velocity;
	vVect v2 = target->velocity;

	/* transform so that v2 = 0 */
	vPXVectorAddV(&v1, vPXVectorMultiplyCopy(v2, -1.0f));

	/* dampen by friction of target object */
	vPXVectorMultiply(&v1, 1.0f - target->friction);

	/* find new v1 and v2 */
	vVect v1Prime = vPXVectorMultiplyCopy(v1,
		(source->mass - target->mass) / (source->mass + target->mass));

	/* shift back so that v2 no longer equals 0 */
	vPXVectorAddV(&v1Prime, v2);

	return v1Prime;
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

	/* if debug mode, draw bound */
	if (_vphys.debugMode == TRUE)
		PXDebugDrawBound(phys);
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
}

static void vPXPartitionIterateCollisionFunc(vHNDL dbHndl, vPPXPartition part,
	vPTR input)
{
	/* if partition has 1 element or less, skip */
	if (part->useage <= 1) return;

	/* if nothing in the partition is moving around, skip */
	if (part->totalVelocity < PARITION_MINVELOCITY) return;

	/* pushback vector accumulator */
	PPXPushbackInfo colPushList = 
		vAllocZeroed(sizeof(PXPushbackInfo) * part->useage);

	/* collision info list */
	PPXCollisionInfo colList = vAllocZeroed(sizeof(PXCollisionInfo) * part->useage);

	/* loop all objects */
	for (int i = 0; i < part->useage; i++)
	{
		/* clear collision list */
		vZeroMemory(colList, sizeof(PXCollisionInfo) * part->useage);
		vUI32 colListUseage = 0;

		/* clear pushback accumulator */
		PPXPushbackInfo pushInfo = colPushList + i;
		pushInfo->accumulator    = vCreatePosition(0.0f, 0.0f);
		pushInfo->collisionCount = 0;

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

			/* scale pushback vector by mass ratio and add to accumulator */
			vFloat massRatio = target->mass / (source->mass + target->mass);
			vPXVectorAddV(&pushInfo->accumulator,
				vPXVectorMultiplyCopy(pushBackVec,
					pushBackMag * massRatio * POS_DEINTERSECT_COEFF));
			pushInfo->collisionCount++;

			/* update collision use */
			colListUseage++;
		}

		/* if no collisions detected, skip momentum transfer portion */
		if (colListUseage == 0) continue;

		/* get average momentum transfer vector, and assign as new vector */
		vVect newVelocityAverage = vCreatePosition(0.0f, 0.0f);
		for (int j = 0; j < colListUseage; j++)
		{
			PPXCollisionInfo momentumCol = colList + j;
			vPXVectorAddV(&newVelocityAverage,
				PXCalculateMomentumTransferVect(source, momentumCol->collidedObject));
		}
		vPXVectorMultiply(&newVelocityAverage, 1.0f / (vFloat)colListUseage);
		source->velocity = newVelocityAverage;
	}

	/* loop all object's de-intersection vectors, take average and push */
	for (int i = 0; i < part->useage; i++)
	{
		/* no push if no collisions */
		PPXPushbackInfo pushInfo = colPushList + i;
		if (pushInfo->collisionCount == 0) continue;

		/* average accumulator and push object */
		vPXVectorMultiply(&pushInfo->accumulator,
			1.0f / (vFloat)pushInfo->collisionCount);
		vPXVectorAddV(&part->list[i]->transform.position,
			pushInfo->accumulator);
	}

	/* free lists */
	vFree(colPushList);
	vFree(colList);
}

static void vPXPhysicalListIterateDoDynamicsFunc(vHNDL dbHndl, vPPhysical* objectPtr,
	vPTR input)
{
	vPPhysical phys = *objectPtr;

	/* call object's update func if exists */
	if (phys->updateFunc != NULL)
		phys->updateFunc(phys);

	/* apply object drag */
	PXApplyFriction(phys, phys->drag);

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
		/* axis lines */
		vGDrawLineF(-0xFFFF, 0, 0xFFFF, 0, vGCreateColorB(0, 0, 255, 255), 5.0f);
		vGDrawLineF(0, -0xFFFF, 0, 0xFFFF, vGCreateColorB(255, 0, 0, 255), 5.0f);

		vDBufferIterate(_vphys.partitions, vPXDebugDrawPartitionIterateFunc, NULL);
	}
}