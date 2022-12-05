
/* ========== <vphysthread.c>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal worker thread logic for physics thread			*/


/* ========== INCLUDES							==========	*/
#include "vphysthread.h"
#include "vspacepart.h"
#include <stdio.h>


/* ========== DEBUG DRAW BOUNDS FUNC			==========	*/
void vPXDebugDrawPartitionIterateFunc(vHNDL dBuffer, vPPXPartition part, vPTR input)
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
	vGDrawLinesConnected(drawMesh, 4, vGCreateColorB(BOUND_BOX_COLORb),
		2.0f);
}

void vPXDebugDrawBounds(void)
{

}

/* ========== WORLDBOUND GENERATION				==========	*/
void vPXGenerateWorldBounds(vPPhysical phys)
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

	phys->worldBound.boundingBox = vGCreateRect(minX, maxX, minY, maxY);

	/* calculate "center" */
	phys->worldBound.center = vPXVectorAverageV(phys->worldBound.mesh, 4);
}

/* ========== PHYSICS UPDATE ITERATE FUNC		==========	*/
void vPXPhysicalListIterateUpdateFunc(vHNDL dbHndl, vPPhysical* objectPtr, vPTR input)
{
	vPPhysical pObj = *objectPtr;

	/* if object is inactive, skip */
	if (pObj->properties.isActive == FALSE) return;

	/* increment object's age */
	pObj->age++;

	/* clear object acceleration */
	pObj->acceleration = vPXCreateVect(0.0f, 0.0f);

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
		vGDrawLinesConnected(pObj->worldBound.mesh, 4,
			vGCreateColorB(BOUND_MESH_COLORb), BOUND_LINESIZE);
		vGDrawCross(pObj->worldBound.center, 0.05f, 
			vGCreateColorB(BOUND_MESH_COLORb), BOUND_LINESIZE);
		vPVect boundingBoxMesh[4];
		vPXBoundToMesh(boundingBoxMesh, pObj->worldBound.boundingBox);
		vGDrawLinesConnected(boundingBoxMesh, 4,
			vGCreateColorB(BOUND_BOX_COLORb), BOUND_LINESIZE);
		
	}
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

	/* setup all objects */
	vDBufferIterate(_vphys.physObjectList, vPXPhysicalListIterateUpdateFunc, NULL);

	/* debug draw all partitions */
	if (_vphys.debugMode == TRUE)
	{
		vDBufferIterate(_vphys.partitions, vPXDebugDrawPartitionIterateFunc, NULL);
	}
}