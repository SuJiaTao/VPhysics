
/* ========== <vphyscore.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vphyscore.h"
#include "vphysthread.h"
#include <stdio.h>
#include <math.h>


/* ========== BUFFER CALLBACKS					==========	*/
void vPPhysicsObjectList_initFunc(vHNDL buffer, vPPhysical* elementPtr,
	vPPhysical input)
{
	input->physObjectListPtr = elementPtr;
	*elementPtr = input;
}

/* ========== INITIALIZATION					==========	*/
VPHYSAPI vBOOL vPXInitialize(void)
{
	vZeroMemory(&_vphys, sizeof(_vPXInternals));
	_vphys.isInitialized = TRUE;
	_vphys.debugMode = FALSE;

	/* initialize lock */
	InitializeCriticalSection(&_vphys.lock);
	EnterCriticalSection(&_vphys.lock);

	/* initialize physics object list */
	_vphys.physObjectList = vCreateDBuffer("vPhysical Object List", sizeof(vPPhysical),
		PHYSOBJECT_LIST_NODE_SIZE, vPPhysicsObjectList_initFunc, NULL);

	/* initialize physics component */
	_vphys.physComponent = vCreateComponent("vPhysical Component", NULL, sizeof(vPhysical),
		NULL, vPXPhysical_initFunc, vPXPhysical_destroyFunc, NULL, NULL);

	/* initialize physics worker thread */
	_vphys.physicsThread = vCreateWorker("vPhysics Worker", 1, vPXT_initFunc,
		vPXT_exitFunc, vPXT_cycleFunc, NULL, NULL);

	/* initialize partition buffer */
	_vphys.partitions = vCreateDBuffer("vPhysics Space Partitions", sizeof(vPXPartiton),
		PARTITION_BUFFER_NODE_SIZE, NULL, NULL);
}


/* ========== DEBUG LOGGING						==========	*/
VPHYSAPI vBOOL vPXIsDebug(void)
{
	return _vphys.debugMode;
}

VPHYSAPI void vPXDebugMode(vBOOL mode)
{
	vPXLock();
	_vphys.debugMode = mode;
	vPXUnlock();
}

VPHYSAPI void vPXDebugAttatchOutputHandle(HANDLE hOut, vUI64 flushInterval)
{
	vPXLock();
	_vphys.debugModeOutput = hOut;
	_vphys.debugLogCount = 0;
	_vphys.debugFlushInterval = flushInterval;
	vPXUnlock();
}

VPHYSAPI void vPXDebugRemoveOuputHandle(void)
{
	vPXLock();
	_vphys.debugModeOutput = NULL;
	vPXUnlock();
}

VPHYSAPI void vPXDebugLog(vPCHAR message)
{
	if (_vphys.debugMode == FALSE || _vphys.debugModeOutput == NULL) return;
	vFileWrite(_vphys.debugModeOutput, vFileSize(_vphys.debugModeOutput),
		strlen(message), message);
	_vphys.debugLogCount++;

	if (_vphys.debugLogCount % _vphys.debugFlushInterval == 0)
		FlushFileBuffers(_vphys.debugModeOutput);
}

VPHYSAPI void vPXDebugLogFormatted(vPCHAR message, ...)
{
	va_list args;

	vCHAR strBuff[BUFF_MEDIUM];
	va_start(args, message);
	vsprintf_s(strBuff, sizeof(strBuff), message, args);
	vPXDebugLog(strBuff);
	va_end(args);
}

VPHYSAPI void vPXDebugPhysicalToString(vPCHAR buffer, SIZE_T buffSize,
	vPPhysical p)
{
	vPXLock();
	sprintf_s(buffer, buffSize,
		"PHYSICAL %p:\n"
		"A: %I64u M: %f T:(%f, %f) S: %f R: %f\n"
		"V:(%f, %f) A: (%f %f) L: %d\n",
		p, p->age, p->mass, 
		p->transform.position.x, p->transform.position.y,
		p->transform.scale, p->transform.rotation, 
		p->velocity.x, p->velocity.y, p->acceleration.x, p->acceleration.y,
		p->properties.collideLayer);
	vPXUnlock();
}

VPHYSAPI vPCHAR vPXDebugPhysicalToStringNew(vPPhysical physical)
{
	vPCHAR buff = vAllocZeroed(BUFF_MEDIUM);
	vPXDebugPhysicalToString(buff, BUFF_MEDIUM, physical);
	return buff;
}

VPHYSAPI void vPXDebugLogPhysical(vPPhysical physical)
{
	vPCHAR msg = vPXDebugPhysicalToStringNew(physical);
	vPXDebugLog(msg);
	vFree(msg);
}

/* ========== OBJECT CREATION					==========	*/
VPHYSAPI vPPhysical vPXCreatePhysicsObject(vPObject object, vFloat drag, vFloat friction,
	vFloat bounciness, vFloat mass, vUI8 collideLayer)
{
	/* create heap input copy */
	vPPhysical targetCopy = vAllocZeroed(sizeof(vPhysical));

	targetCopy->object = object;

	targetCopy->properties.isActive			 = TRUE; /* mark object as active		*/
	targetCopy->properties.collideWithParent = TRUE; /* default collides w/ parent	*/
	targetCopy->renderableTransformOverride  = TRUE;

	/* try to find pre-existing renderable */
	targetCopy->renderableCache = vObjectGetComponent(object, vGGetComponentHandle());
	if (targetCopy->renderableCache == NULL) vPXDebugLog("No renderable found.");

	targetCopy->material.drag = drag;
	targetCopy->material.staticFriction  = friction * STATICFRICTION_COEFF_DEFAULT;
	targetCopy->material.dynamicFriction = friction;
	targetCopy->material.bounciness = bounciness;
	targetCopy->mass = mass;

	/* component add is synchronized */
	vPXLock();
	vPComponent comp = vObjectAddComponent(object, _vphys.physComponent, targetCopy);
	vPXUnlock();

	return comp->objectAttribute;
}

VPHYSAPI void vPXDestroyPhysicsObject(vPObject object)
{
	vObjectRemoveComponent(object, _vphys.physComponent);
}


/* ========== VECTOR LOGIC						==========	*/
VPHYSAPI void vPXEnforceEpsilonF(vPFloat f1)
{
	/* set nan or inf to 0 */
	if (isnan(*f1) || isinf(*f1)) *f1 = 0.0f;

	/* if below epsilon, set to 0 */
	if (*f1 < VPHYS_EPSILON) *f1 = 0.0f;

	/* clear sign */
	*(PDWORD)(f1) &= 0x7fffffff;
}

VPHYSAPI void vPXEnforceEpsilonV(vPVect v1)
{
	vPXEnforceEpsilonF(&v1->x);
	vPXEnforceEpsilonF(&v1->y);
}

VPHYSAPI void vPXVectorReverse(vPVect v1)
{
	v1->x = -v1->x;
	v1->y = -v1->y;
}

VPHYSAPI void vPXVectorAddV(vPVect v1, vVect v2)
{
	v1->x += v2.x;
	v1->y += v2.y;
}

VPHYSAPI void vPXVectorAddF(vPVect v1, vFloat x, vFloat y)
{
	v1->x += x;
	v1->y += y;
}

VPHYSAPI void vPXVectorMultiply(vPVect v1, vFloat s)
{
	v1->x *= s;
	v1->y *= s;
}

VPHYSAPI void vPXVectorRotate(vPVect v1, vFloat theta)
{
	vFloat vr = vPXVectorMagnitudeV(*v1);
	vFloat vtheta = atan2f(v1->y, v1->x);

	vtheta += theta;

	v1->x = vr * cosf(theta);
	v1->x = vr * sinf(theta);
}

VPHYSAPI void vPXVectorRotatePrecise(vPVect v1, vFloat theta)
{
	vFloat vr = vPXVectorMagnitudePrecise(*v1);
	vFloat vtheta = atan2f(v1->y, v1->x);

	vtheta += theta;

	v1->x = vr * cosf(theta);
	v1->x = vr * sinf(theta);
}

VPHYSAPI vFloat vPXVectorMagnitudeV(vVect v1)
{
	/* quick fabs */
	*(PDWORD)(&v1.x) &= 0x7fffffff;
	*(PDWORD)(&v1.y) &= 0x7fffffff;

	/* refer to <https://youtu.be/NWBEA2ECX-A> */
	vFloat x = max(v1.x, v1.y) * 0.96f;
	vFloat y = min(v1.x, v1.y) * 0.40f;

	return x + y;
}

VPHYSAPI vFloat vPXVectorMagnitudeF(vFloat x, vFloat y)
{
	/* quick fabs */
	*(PDWORD)&x &= 0x7fffffff;
	*(PDWORD)&y &= 0x7fffffff;

	/* refer to <https://youtu.be/NWBEA2ECX-A> */
	vFloat _x = max(x, y) * 0.96f;
	vFloat _y = min(x, y) * 0.40f;

	return _x + _y;
}

VPHYSAPI vFloat vPXVectorMagnitudePrecise(vVect v1)
{
	return sqrtf((v1.x * v1.x) + (v1.y * v1.y));
}

/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void)
{
	EnterCriticalSection(&_vphys.lock);
}

VPHYSAPI void vPXUnlock(void)
{
	LeaveCriticalSection(&_vphys.lock);
}
