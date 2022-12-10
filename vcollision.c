
/* ========== <vcollision.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal collision handling logic						*/

/* ========== INCLUDES							==========	*/
#include "vcollision.h"
#include <stdio.h>



/* ========== INTERNAL STRUCTS					==========	*/
typedef struct PXProjectionPlane
{
	vVect  vert1;
	vVect  vert2;
	vVect  planeVector;
	vFloat planeVectorMag;
} PXProjectionPlane, *PPXProjectionPlane;



/* ========== HELPER							==========	*/
PXProjectionPlane PXGenerateProjectionPlane(vVect v1, vVect v2)
{
	PXProjectionPlane plane;
	plane.vert1 = v1;
	plane.vert2 = v2;
	plane.planeVector = vCreatePosition(v2.x - v1.x, v2.y - v1.y);
	plane.planeVectorMag = vPXVectorMagnitudePrecise(plane.planeVector);
	return plane;
}


/* ========== COLLISION FUNCTIONS				==========	*/
VPHYSAPI vBOOL PXDetectCollisionPreEstimate(vPPhysical p1, vPPhysical p2)
{
	/* what happens is we get 2x the maximum possible distance	*/
	/* the 2 quads can be touching, and we compare that against	*/
	/* 2x the approximate distance between the two quads		*/
	/* (the 2x is there because of how world-bounding box is	*/
	/* pre-calculated). if the estimated distance is less than	*/
	/* the max possible distance for collision, then consider	*/
	/* as collision												*/

	/* get approximate distance between two */
	vFloat dh = vPXVectorMagnitudeF(
		p1->worldBound.center.x - p1->worldBound.center.x,
		p1->worldBound.center.y - p1->worldBound.center.y);

	/* get maximum bounding box dimension of each object */
	vFloat mx = max(p1->worldBound.boundingBoxDims.x,
		p2->worldBound.boundingBoxDims.x);
	vFloat my = max(p1->worldBound.boundingBoxDims.y,
		p2->worldBound.boundingBoxDims.y);

	/* if the approx dist is less than the sum of the max,	*/
	/* consider collision									*/
	return ((dh * 2.0f) < (mx + my));
}

VPHYSAPI vBOOL PXDetectCollisionSAT(vPPhysical source, vPPhysical target,
	vPVect pushVector)
{
	/* grab worldbounds of target and source */
	vPPXWorldBoundMesh targWB = &target->worldBound;
	vPPXWorldBoundMesh sourceWB = &source->worldBound;

	/* get displacement from target to source */
	vVect displacementVect = vCreatePosition(sourceWB->center.x - targWB->center.x,
		sourceWB->center.y - targWB->center.y);

	/* generate edge normals to project vertexes onto	*/
	/* since both meshes are rectangles (and therefore 2x symmetrical) */
	/* only 2 of the 4 faces of each rect are required to project onto	*/
	/* as they are parallel with their counterparts, and will generate	*/
	/* identical numbers.	*/

	/* generate projection planes (2 per mesh) */
	PXProjectionPlane projectionPlanes[8];
	for (int i = 0; i < 4; i++)
	{
		projectionPlanes[i] = 
			PXGenerateProjectionPlane(sourceWB->mesh[i], sourceWB->mesh[(i + 1) & 0b11]);
		projectionPlanes[i + 4] =
			PXGenerateProjectionPlane(targWB->mesh[i], targWB->mesh[(i + 1) & 0b11]);
	}

	/* project each mesh onto each projection plane and count	*/
	/* the amount of planes which the two meshes overlap		*/
	int projectionOverlapCount = 0;

	/* keep track of pushback vector */
	vFloat pushBackVectorMag = 0x10000; /* large value */
	vVect  pushBackVectorDir;

	/* loop all projection planes */
	for (int i = 0; i < 8; i++)
	{
		/* get projection plane and projection vector	*/
		/* normalize the projection vector				*/
		PPXProjectionPlane projPlane = projectionPlanes + i;
		vVect projVect = projPlane->planeVector;
		vPXVectorNormalize(&projVect);

		/* project source verts onto plane and keep track of min/max */
		vFloat sourceMin, sourceMax;
		for (int j = 0; j < 4; j++)
		{
			vFloat sDot = vPXVectorDotProduct(sourceWB->mesh[j], projVect);
			if (j == 0)
			{
				sourceMin = sDot;
				sourceMax = sDot;
				continue;
			}
			
			sourceMin = min(sDot, sourceMin);
			sourceMax = max(sDot, sourceMax);
		}
		

		/* project source verts onto plane and keep track of min/max */
		vFloat targMin, targMax;
		for (int j = 0; j < 4; j++)
		{
			vFloat tDot = vPXVectorDotProduct(targWB->mesh[j], projVect);
			if (j == 0)
			{
				targMin = tDot;
				targMax = tDot;
				continue;
			}

			targMin = min(tDot, targMin);
			targMax = max(tDot, targMax);
		}

		/* check if projected min and max values overlap */
		if (max(sourceMax, targMax) - min(sourceMin, targMin)
			<= (sourceMax - sourceMin) + (targMax - targMin))
		{
			/* find overlap region */
			vFloat overlapRegion = 
				min(sourceMax, targMax) - max(sourceMax, targMax);

			/* if smaller than previous overlap, assign new */
			if (overlapRegion < pushBackVectorMag)
			{
				pushBackVectorMag = overlapRegion;
				pushBackVectorDir = projVect;
			}

			projectionOverlapCount++;
		}
	}

	vVect point2 = sourceWB->center;
	vPXVectorMultiply(&pushBackVectorDir, vPXFastFabs(pushBackVectorMag));
	vPXVectorAddV(&point2, pushBackVectorDir);
	vGDrawLineV(sourceWB->center, point2, vGCreateColorB(255, 255, 255, 30),
		5);

	return 0;
}
