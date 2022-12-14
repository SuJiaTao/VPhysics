
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
VPHYSAPI vBOOL vPXDetectCollisionPreEstimate(vPPhysical p1, vPPhysical p2)
{
	/* get approximate distance between two */
	vFloat dh = vPXVectorMagnitudeF(
		p1->worldBound.center.x - p2->worldBound.center.x,
		p1->worldBound.center.y - p2->worldBound.center.y);

	/* get maximum bounding box dimension of each object */
	vFloat mx = max(p1->worldBound.boundingBoxDims.x,
		p2->worldBound.boundingBoxDims.x);
	vFloat my = max(p1->worldBound.boundingBoxDims.y,
		p2->worldBound.boundingBoxDims.y);

	/* if the approx dist is less than the sum of the max,	*/
	/* consider collision									*/
	return (dh < (mx + my));
}

VPHYSAPI vBOOL vPXDetectCollisionSAT(vPPhysical source, vPPhysical target,
	vPVect pushVector, vPFloat pushVectorMagnitude)
{
	/* if exists, reset pushvector and magnitude */
	if (pushVector != NULL)
		*pushVector = vCreatePosition(0.0f, 0.0f);
	if (pushVectorMagnitude != NULL)
		*pushVectorMagnitude = 0.0f;

	/* grab worldbounds of target and source */
	vPPXWorldBoundMesh targWB = &target->worldBound;
	vPPXWorldBoundMesh sourceWB = &source->worldBound;

	/* get displacement from target to source and normalize */
	vVect displacementVect = vCreatePosition(sourceWB->center.x - targWB->center.x,
		sourceWB->center.y - targWB->center.y);
	vPXVectorNormalize(&displacementVect);

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
				min(sourceMax, targMax) - max(sourceMin, targMin);

			/* if in admissible direction, and */
			/* if smaller than previous overlap, assign new */
			if (vPXVectorDotProduct(projVect, displacementVect) > 0.0f
				&& overlapRegion < pushBackVectorMag)
			{
				pushBackVectorMag = overlapRegion;
				pushBackVectorDir = projVect;
				vPXVectorNormalize(&pushBackVectorDir);
			}

			projectionOverlapCount++;
		}
		else
		{
			/* if there is a region of no overlap, then no collision. */
			return FALSE;
		}
	}

	/* on reached here, objects are colliding, assign pushvector and magnitude */
	if (pushVector != NULL)
		*pushVector = pushBackVectorDir;
	if (pushVectorMagnitude != NULL)
		*pushVectorMagnitude = pushBackVectorMag;
		
	return TRUE;
}
