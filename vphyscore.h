
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


/* ========== FASTALLOC STACK					==========	*/
VPHYSAPI void  vPXFastAllocLock(void);
VPHYSAPI void  vPXFastAllocUnlock(void);
VPHYSAPI vPTR  vPXFastAllocPush(SIZE_T amount);
VPHYSAPI void  vPXFastAllocPop(SIZE_T amount);


/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void);
VPHYSAPI void vPXUnlock(void);

#endif
