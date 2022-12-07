
/* ========== <vcollision.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal collision handling logic						*/

/* ========== INCLUDES							==========	*/
#include "vcollision.h"


/* ========== HELPER							==========	*/



/* ========== COLLISION FUNCTIONS				==========	*/
vBOOL PXDetectCollisionPreEstimate(vPPhysical p1, vPPhysical p2)
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

	/* if the sum of the max bounding box components is less than	*/
	/* the approximate distance, consider collision					*/
	return ((dh * 2.0f) < (mx + my));
}

vBOOL PXDetectCollisionSAT(vPPhysical p1, vPPhysical p2)
{
	
}
