
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


/* ========== OBJECT CREATION					==========	*/
VPHYSAPI vPPhysical vPXCreatePhysicsObject(vPObject object, vFloat friction, 
	vFloat bounciness, vFloat mass, vUI16 collideLayerMask, vUI16 noCollideLayerMask);
VPHYSAPI void vPXDestroyPhysicsObject(vPObject object);


/* ========== VECTOR LOGIC						==========	*/
VPHYSAPI void vPXVectorReverse(vPPosition v1);
VPHYSAPI void vPXVectorAddV(vPPosition v1, vPosition v2);
VPHYSAPI void vPXVectorAddF(vPPosition v1, vFloat x, vFloat y);
VPHYSAPI void vPXVectorMultiply(vPPosition v1, vFloat s);
VPHYSAPI void vPXVectorRotate(vPPosition v1, vFloat theta);
VPHYSAPI vFloat vPXVectorMagnitudeV(vPosition v1);
VPHYSAPI vFloat vPXVectorMagnitudeF(vFloat x, vFloat y);


/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void);
VPHYSAPI void vPXUnlock(void);

#endif
