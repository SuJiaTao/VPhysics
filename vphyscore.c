
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

	/* initialize quick alloc stack */
	InitializeCriticalSection(&_vphys.fastMemStack.rwLock);
	_vphys.fastMemStack.block = vAllocZeroed(FASTMEM_STACK_BYTES);

	/* initialize physics object list */
	_vphys.physObjectList = vCreateDBuffer("vPhysics Object List", sizeof(vPPhysicsObject),
		PHYSOBJECT_LIST_NODE_SIZE, vPPhysicsObjectList_initFunc, NULL);

	/* initialize physics component */
	_vphys.physComponent = vCreateComponent("vPhysics Component", NULL, sizeof(vPhysicsObject),
		NULL, NULL, NULL, NULL, NULL);

	/* initialize physics worker thread */
	_vphys.physicsThread = vCreateWorker("vPhysics Worker", 1, NULL, NULL, NULL, NULL, NULL);
}


/* ========== FASTALLOC STACK					==========	*/
VPHYSAPI void  vPXFastAllocLock(void)
{
	EnterCriticalSection(&_vphys.fastMemStack.rwLock);
}

VPHYSAPI void  vPXFastAllocUnlock(void)
{
	LeaveCriticalSection(&_vphys.fastMemStack.rwLock);
}

VPHYSAPI vPTR vPXFastAllocPush(SIZE_T amount)
{
	EnterCriticalSection(&_vphys.fastMemStack.rwLock);

	/* overflow check */
	if (_vphys.fastMemStack.ptr + amount >= FASTMEM_STACK_BYTES) 
	{
		LeaveCriticalSection(&_vphys.fastMemStack.rwLock);
		vLogError(__func__, "FastAlloc failed: Stack overflow.");
		return NULL;
	}
	
	/* move stack ptr, zero memory, return */
	_vphys.fastMemStack.ptr += amount;

	vPTR ptr = _vphys.fastMemStack.block + _vphys.fastMemStack.ptr;
	vZeroMemory(ptr, amount);

	LeaveCriticalSection(&_vphys.fastMemStack.rwLock);
	return ptr;;
}

VPHYSAPI void vPXFastAllocPop(SIZE_T amount)
{
	EnterCriticalSection(&_vphys.fastMemStack.rwLock);
	/* underflow check */
	if (_vphys.fastMemStack.ptr - amount < 0)
	{
		LeaveCriticalSection(&_vphys.fastMemStack.rwLock);
		vLogError(__func__, "FastAlloc failed: Stack underflow.");
		return;
	}

	_vphys.fastMemStack.ptr -= amount;
	LeaveCriticalSection(&_vphys.fastMemStack.rwLock);
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
