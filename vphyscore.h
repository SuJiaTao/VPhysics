
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


/* ========== SYNCHRONIZATION					==========	*/
VPHYSAPI void vPXLock(void);
VPHYSAPI void vPXUnlock(void);

#endif
