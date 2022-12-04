
/* ========== <vspacepart.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal space partitioning logic						*/


/* ========== INCLUDES							==========	*/
#include "vspacepart.h"


/* ========== INTERNAL STRUCTS					==========	*/


/* ========== HELPERS							==========	*/


/* ========== ITERATE CALLBACKS					==========	*/
void PXPartitionClearIterateFunc(vHNDL dbHndl, vPPXPartition partition, vPTR input)
{
	partition->inUse  = FALSE;	/* mark as unused */
	partition->useage = ZERO;	/* reset useage counter */
}


/* ========== SPACE PARTITIONING FUNCTIONS		==========	*/
void PXPartObjectSetup(vPPhysical phys)
{
	
}

void PXPartClearPartitions(void)
{
	vDBufferIterate(_vphys.partitions, PXPartitionClearIterateFunc, NULL);
}

void PXPartObjectAssign(vPPhysical phys)
{

}