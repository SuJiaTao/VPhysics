
/* ========== <vphyscore.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vphyscore.h"
#include "vphysthread.h"
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
	vZeroMemory(&_vphys, sizeof(_vPHYSInternals));
	_vphys.isInitialized = TRUE;

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
}


/* ========== OBJECT CREATION					==========	*/
VPHYSAPI vPPhysical vPXCreatePhysicsObject(vPObject object, vFloat friction,
	vFloat bounciness, vFloat mass, vUI16 collideLayerMask, vUI16 noCollideLayerMask)
{
	/* create heap input copy */
	vPPhysical targetCopy = vAllocZeroed(sizeof(vPhysical));

	targetCopy->object = object;

	targetCopy->properties.isActive			 = TRUE; /* mark object as active		*/
	targetCopy->properties.collideWithParent = TRUE; /* default collides w/ parent	*/

	targetCopy->material.staticFriction  = friction * STATICFRICTION_COEFF_DEFAULT;
	targetCopy->material.dynamicFriction = friction;
	targetCopy->material.bounciness = bounciness;
	targetCopy->mass = mass;
	targetCopy->properties.collideLayer   = collideLayerMask;
	targetCopy->properties.noCollideLayer = noCollideLayerMask;

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

VPHYSAPI void vPXEnforceEpsilonV(vPPosition v1)
{
	vPXEnforceEpsilonF(&v1->x);
	vPXEnforceEpsilonF(&v1->y);
}

VPHYSAPI void vPXVectorReverse(vPPosition v1)
{
	v1->x = -v1->x;
	v1->y = -v1->y;
}

VPHYSAPI void vPXVectorAddV(vPPosition v1, vPosition v2)
{
	v1->x += v2.x;
	v1->y += v2.y;
}

VPHYSAPI void vPXVectorAddF(vPPosition v1, vFloat x, vFloat y)
{
	v1->x += x;
	v1->y += y;
}

VPHYSAPI void vPXVectorMultiply(vPPosition v1, vFloat s)
{
	v1->x *= s;
	v1->y *= s;
}

VPHYSAPI void vPXVectorRotate(vPPosition v1, vFloat theta)
{
	vFloat vr = vPXVectorMagnitudeV(*v1);
	vFloat vtheta = atan2f(v1->y, v1->x);

	vtheta += theta;

	v1->x = vr * cosf(theta);
	v1->x = vr * sinf(theta);
}

VPHYSAPI vFloat vPXVectorMagnitudeV(vPosition v1)
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

/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void)
{
	EnterCriticalSection(&_vphys.lock);
}

VPHYSAPI void vPXUnlock(void)
{
	LeaveCriticalSection(&_vphys.lock);
}
