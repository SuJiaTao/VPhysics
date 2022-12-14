
/* ========== <vphysical.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal physics component callbacks and logic			*/

#ifndef _VPHYS_INTERNAL_PHYSICAL_INCLUDE_
#define _VPHYS_INTERNAL_PHYSICAL_INCLUDE_ 


/* ========== INCLUDES							==========	*/
#include "vphys.h"


/* ========== COMPONENT CALLBACKS				==========	*/
void vPXPhysical_initFunc(vPObject object, vPComponent component, vPTR input);
void vPXPhysical_destroyFunc(vPObject object, vPComponent component);


#endif
