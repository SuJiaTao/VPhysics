
/* ========== <vphyscore.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Physics core functions such as initialization and info	*/
/* polling.													*/

#ifndef _VPHYS_CORE_INCLUDE_
#define _VPHYS_CORE_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include <stdarg.h>
#include "vphysdefs.h"
#include "vphysical.h"


/* ========== INITIALIZATION					==========	*/
VPHYSAPI vBOOL vPXInitialize(HANDLE debugOut, vUI64 flushInterval);


/* ========== DEBUG LOGGING						==========	*/
VPHYSAPI vBOOL vPXIsDebug(void);
VPHYSAPI void vPXDebugMode(vBOOL mode);
VPHYSAPI void vPXDebugAttatchOutputHandle(HANDLE hOut, vUI64 flushInterval);
VPHYSAPI void vPXDebugRemoveOuputHandle(void);
VPHYSAPI void vPXDebugLog(vPCHAR message);
VPHYSAPI void vPXDebugLogFormatted(vPCHAR message, ...);
VPHYSAPI void vPXDebugPhysicalToString(vPCHAR buffer, SIZE_T buffSize,
	vPPhysical physical);

/* ========== OBJECT CREATION					==========	*/
VPHYSAPI vPPhysical vPXCreatePhysicsObject(vPObject object, vTransform transform,
	vGRect boundingBox, vFloat drag, vFloat friction, vFloat bounciness, 
	vFloat mass, vUI8 collideLayer);
VPHYSAPI void vPXSetPhysicsObjectCallbacks(vPPhysical pObj,
	vPXPFPHYSICALUPDATEFUNC updateFunc,
	vPXPFPHYSICALCOLLISIONFUNC collisionCallback);
VPHYSAPI void vPXDestroyPhysicsObject(vPObject object);


/* ========== VECTOR LOGIC						==========	*/
VPHYSAPI vVect vPXCreateVect(vFloat x, vFloat y);
VPHYSAPI void vPXEnforceEpsilonF(vPFloat f1);
VPHYSAPI void vPXEnforceEpsilonV(vPVect v1);
VPHYSAPI void vPXVectorReverse(vPVect v1);
VPHYSAPI void vPXVectorAddV(vPVect v1, vVect v2);
VPHYSAPI void vPXVectorAddF(vPVect v1, vFloat x, vFloat y);
VPHYSAPI void vPXVectorMultiply(vPVect v1, vFloat s);
VPHYSAPI void vPXVectorRotate(vPVect v1, vFloat theta);
VPHYSAPI void vPXVectorRotatePrecise(vPVect v1, vFloat theta);
VPHYSAPI vFloat vPXVectorMagnitudeV(vVect v1);
VPHYSAPI vFloat vPXVectorMagnitudeF(vFloat x, vFloat y);
VPHYSAPI vFloat vPXVectorMagnitudePrecise(vVect v1);
VPHYSAPI void vPXVectorTransform(vPVect v1, vVect translate, 
	vFloat scale, vFloat rotate);
VPHYSAPI vVect vPXVectorAverage(vVect v1, vVect v2);
VPHYSAPI vVect vPXVectorAverageV(vPVect vv, vUI16 count);


/* ========== MISC LOGIC						==========	*/
VPHYSAPI void vPXBoundToMesh(vPVect meshArray, vGRect rect);


/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void);
VPHYSAPI void vPXUnlock(void);

#endif
