
/* ========== <vphyscore.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vphyscore.h"


/* ========== BUFFER CALLBACKS					==========	*/
void vPPhysicsObjectList_initFunc(vHNDL buffer, vPPhysicsObject* elementPtr,
	vPPhysicsObject input)
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
	_vphys.physObjectList = vCreateDBuffer("vPhysics Object List", sizeof(vPPhysicsObject),
		PHYSOBJECT_LIST_NODE_SIZE, vPPhysicsObjectList_initFunc, NULL);

	/* initialize physics component */
	_vphys.physComponent = vCreateComponent("vPhysical Component", NULL, sizeof(vPhysicsObject),
		NULL, NULL, NULL, NULL, NULL);

	/* initialize physics worker thread */
	_vphys.physicsThread = vCreateWorker("vPhysics Worker", 1, NULL, NULL, NULL, NULL, NULL);
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
