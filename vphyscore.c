
/* ========== <vphyscore.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#define _CRT_SECURE_NO_WARNINGS 
#include "vphyscore.h"
#include "vphysthread.h"
#include <stdio.h>
#include <math.h>


/* ========== BUFFER CALLBACKS					==========	*/
static void vPPhysicsObjectList_initFunc(vHNDL buffer, vPPhysical* elementPtr,
	vPPhysical input)
{
	input->physObjectListPtr = elementPtr;
	*elementPtr = input;
}

/* ========== INITIALIZATION					==========	*/
VPHYSAPI vBOOL vPXInitialize(HANDLE debugOut, vUI64 flushInterval)
{
	vZeroMemory(&_vphys, sizeof(_vPXInternals));
	_vphys.isInitialized = TRUE;
	_vphys.debugMode = FALSE;

	/* initialize lock */
	InitializeCriticalSection(&_vphys.lock);
	EnterCriticalSection(&_vphys.lock);

	/* setup debug out */
	vPXDebugAttatchOutputHandle(debugOut, flushInterval);

	/* initialize physics object list */
	_vphys.physObjectList = vCreateDBuffer("vPhysical Object List", sizeof(vPPhysical),
		PHYSOBJECT_LIST_NODE_SIZE, vPPhysicsObjectList_initFunc, NULL);

	/* initialize physics component */
	_vphys.physComponent = vCreateComponent("vPhysical Component", NULL, sizeof(vPhysical),
		NULL, vPXPhysical_initFunc, vPXPhysical_destroyFunc, NULL, NULL);

	/* initialize partition buffer */
	_vphys.partitionSize = PARTITION_SIZE_DEFAULT;
	_vphys.partitions = vCreateDBuffer("vPhysics Space Partitions", sizeof(vPXPartiton),
		PARTITION_BUFFER_NODE_SIZE, NULL, NULL);

	/* initialize physics worker thread */
	_vphys.physicsThread = vCreateWorker("vPhysics Worker", 10, vPXT_initFunc,
		vPXT_exitFunc, vPXT_cycleFunc, NULL, NULL);

	/* initialize random number table */
	vPXRandInit();
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

/* ========== OBJECT CREATION					==========	*/
VPHYSAPI vPPhysical vPXCreatePhysicsObject(vPObject object, vTransform transform,
	vGRect boundingBox, vFloat drag, vFloat friction,
	vFloat mass, vUI8 collideLayer)
{
	/* create heap input copy */
	vPPhysical targetCopy = vAllocZeroed(sizeof(vPhysical));

	targetCopy->object = object;

	targetCopy->properties.isActive			 = TRUE; /* mark object as active		*/
	targetCopy->properties.collideWithParent = TRUE; /* default collides w/ parent	*/
	targetCopy->renderableTransformOverride  = TRUE;

	/* setup default transform */
	targetCopy->transform = transform;

	/* try to find pre-existing renderable */
	vPComponent renderComp = vObjectGetComponent(object, vGGetComponentHandle());
	if (renderComp == NULL)
	{
		vPXDebugLog("No renderable found.\n");
	}
	else
	{
		targetCopy->renderableCache = renderComp->objectAttribute;
		vPXDebugLogFormatted("Found existing renderable: %p\n", 
			targetCopy->renderableCache);
	}

	targetCopy->drag = drag;
	targetCopy->friction = friction;
	targetCopy->mass  = max(VPHYS_EPSILON, mass); /* ensure min mass */
	targetCopy->bound = boundingBox;

	/* component add is synchronized */
	vPXLock();
	vPComponent comp = vObjectAddComponent(object, _vphys.physComponent, targetCopy);
	vPXUnlock();

	/* if debug mode, log the creation */
	if (vPXIsDebug())
	{
		vPPhysical phys = comp->objectAttribute;
		vPXDebugLogFormatted("Created New Object at <%f %f>\n",
			phys->transform.position.x, phys->transform.position.y);
	}

	return comp->objectAttribute;
}

VPHYSAPI void vPXSetPhysicsObjectCallbacks(vPPhysical pObj,
	vPXPFPHYSICALUPDATEFUNC updateFunc,
	vPXPFPHYSICALCOLLISIONFUNC collisionCallback)
{
	pObj->updateFunc = updateFunc;
	pObj->collisionFunc = collisionCallback;
}

VPHYSAPI void vPXDestroyPhysicsObject(vPObject object)
{
	vObjectRemoveComponent(object, _vphys.physComponent);
}


/* ========== VECTOR LOGIC						==========	*/
VPHYSAPI vVect vPXCreateVect(vFloat x, vFloat y)
{
	vVect rVec;
	rVec.x = x;
	rVec.y = y;
	return rVec;
}

VPHYSAPI void vPXEnforceEpsilonF(vPFloat f1)
{
	/* set nan or inf to 0 */
	if (isnan(*f1) || isinf(*f1)) *f1 = 0.0f;

	/* if within epsilon of zero, set to 0 */
	if ((*(PDWORD)(f1) & 0x7fffffff) < VPHYS_EPSILON) *f1 = 0.0f;
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

VPHYSAPI vVect vPXVectorAddCopy(vVect v1, vVect v2)
{
	vPXVectorAddV(&v1, v2);
	return v1;
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

VPHYSAPI vVect vPXVectorMultiplyCopy(vVect v1, vFloat s)
{
	vPXVectorMultiply(&v1, s);
	return v1;
}

VPHYSAPI void vPXVectorRotate(vPVect v1, vFloat theta)
{
	if (theta == 0.0f) return;

	vFloat vr = vPXVectorMagnitudeV(*v1);
	vFloat vtheta = atan2f(v1->y, v1->x);

	vtheta += (theta * VPHYS_DEGTORAD);

	v1->x = vr * cosf(vtheta);
	v1->y = vr * sinf(vtheta);
}

VPHYSAPI void vPXVectorRotatePrecise(vPVect v1, vFloat theta)
{
	if (theta == 0.0f) return;

	vFloat vr = vPXVectorMagnitudePrecise(*v1);
	vFloat vtheta = atan2f(v1->y, v1->x);

	vtheta += (theta * VPHYS_DEGTORAD);

	v1->x = vr * cosf(vtheta);
	v1->y = vr * sinf(vtheta);
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

VPHYSAPI void vPXVectorTransform(vPVect v1, vVect translate,
	vFloat scale, vFloat rotate)
{
	vPXVectorRotatePrecise(v1, rotate);
	vPXVectorMultiply(v1, scale);
	vPXVectorAddV(v1, translate);
}

VPHYSAPI vVect vPXVectorAverage(vVect v1, vVect v2)
{
	return vPXCreateVect((v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f);
}

VPHYSAPI vVect vPXVectorAverageV(vPVect vv, vUI16 count)
{
	float xAccum = 0.0f, yAccum = 0.0f;
	for (int i = 0; i < count; i++)
	{
		xAccum += vv[i].x;
		yAccum += vv[i].y;
	}
	return vPXCreateVect(xAccum / (float)count, yAccum / (float)count);
}

VPHYSAPI float vPXVectorDotProduct(vVect v1, vVect v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y);
}

VPHYSAPI void vPXVectorNormalize(vPVect v)
{
	vFloat mag = vPXVectorMagnitudePrecise(*v);
	v->x /= mag;
	v->y /= mag;
}


/* ========== MISC LOGIC						==========	*/
VPHYSAPI void vPXBoundToMesh(vPVect meshArray, vGRect rect)
{
	/* setup pre-transformed world mesh */
	meshArray[0]
		= vPXCreateVect(rect.left, rect.bottom);
	meshArray[1]
		= vPXCreateVect(rect.left, rect.top);
	meshArray[2]
		= vPXCreateVect(rect.right, rect.top);
	meshArray[3]
		= vPXCreateVect(rect.right, rect.bottom);
}

VPHYSAPI vFloat vPXFastFabs(vFloat f)
{
	((*(PDWORD)&f) &= 0x7fffffff);
	return f;
}

VPHYSAPI void   vPXFastFabsP(vPFloat pf)
{
	*(PDWORD)pf &= 0x7fffffff;
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
