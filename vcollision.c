
/* ========== <vcollision.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal collision handling logic						*/

/* ========== INCLUDES							==========	*/
#include "vcollision.h"


/* ========== HELPER							==========	*/



/* ========== COLLISION FUNCTIONS				==========	*/
vBOOL PXDetectCollisionLevel0(vPPhysical p1, vPPhysical p2)
{
	/* get x and y displacement between objects */
	vFloat dx = vPXFastFabs(p1->worldBound.center.x - p1->worldBound.center.x);
	vFloat dy = vPXFastFabs(p1->worldBound.center.y - p1->worldBound.center.y);

	/* get maximum bounding box dimension of each object */
	vFloat mx = max(p1->worldBound.boundingBoxDims.x, 
		p2->worldBound.boundingBoxDims.x);
	vFloat my = max(p1->worldBound.boundingBoxDims.y,
		p2->worldBound.boundingBoxDims.y);

	/* if the sum of the max bounding box components is less than		*/
	/* the smallest displacement between the two, consider collision	*/
	return ((mx + my) < min(dx, dy));
}

vBOOL PXDetectCollisionLevel1(vPPhysical p1, vPPhysical p2)
{
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
	return ((mx + my) < dh);
}

vBOOL PXDetectCollisionLevel2(vPPhysical p1, vPPhysical p2)
{
	
}
