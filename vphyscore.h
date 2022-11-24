
/* ========== <vphyscore.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Physics core functions such as initialization and info	*/
/* polling.													*/

#ifndef _VPHYS_CORE_INCLUDE_
#define _VPHYS_CORE_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vphysdefs.h"
#include "vphysical.h"


/* ========== INITIALIZATION					==========	*/
VPHYSAPI vBOOL vPXInitialize(void);


/* ========== INITIALIZATION					==========	*/
VPHYSAPI vPPhysicsObject vPXCreatePhysicsObject(vPObject object, vFloat friction, 
	vFloat bounciness, vFloat mass, vUI16 collideLayerMask, vUI16 noCollideLayerMask);
VPHYSAPI void vPXDestroyPhysicsObject(vPObject object);


/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void);
VPHYSAPI void vPXUnlock(void);

#endif
