
/* ========== <vspacepart.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal space partitioning logic						*/

#ifndef _VPHYS_INTERNAL_SPACEPART_INCLUDE_
#define _VPHYS_INTERNAL_SPACEPART_INCLUDE_


/* ========== INCLUDES							==========	*/
#include "vphys.h"


/* ========== SPACE PARTITIONING FUNCTIONS		==========	*/
void PXPartResetPartitions(void);
void PXPartObjectOrangizeIntoPartitions(vPPhysical phys);

#endif