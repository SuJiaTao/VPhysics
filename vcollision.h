
/* ========== <vcollision.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal collision handling logic						*/

#ifndef _VPHYS_COLLISION_INCLUDE_
#define _VPHYS_COLLISION_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vphys.h"

/* ========== COLLISION FUNCTIONS				==========	*/
vBOOL PXDetectCollisionLevel0(vPPhysical p1, vPPhysical p2);
vBOOL PXDetectCollisionLevel1(vPPhysical p1, vPPhysical p2);
vBOOL PXDetectCollisionLevel2(vPPhysical p1, vPPhysical p2);

#endif
