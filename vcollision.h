
/* ========== <vcollision.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal collision handling logic						*/

#ifndef _VPHYS_COLLISION_INCLUDE_
#define _VPHYS_COLLISION_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vphys.h"

/* ========== COLLISION FUNCTIONS				==========	*/
VPHYSAPI vBOOL PXDetectCollisionPreEstimate(vPPhysical p1, vPPhysical p2);
VPHYSAPI vBOOL PXDetectCollisionSAT(vPPhysical source, vPPhysical target, 
	vPVect pushVector);

#endif
