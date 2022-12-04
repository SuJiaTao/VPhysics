
/* ========== <vspacepart.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal space partitioning logic						*/


/* ========== INCLUDES							==========	*/
#include "vspacepart.h"
#include <fenv.h>	/* for rounding specification */
#include <math.h>


/* ========== INTERNAL STRUCTS					==========	*/


/* ========== HELPERS							==========	*/


/* ========== ITERATE CALLBACKS					==========	*/
void PXPartitionResetIterateFunc(vHNDL dbHndl, vPPXPartition partition, vPTR input)
{
	partition->inUse  = FALSE;	/* mark as unused */
	partition->useage = ZERO;	/* reset useage counter */
}


/* ========== SPACE PARTITIONING FUNCTIONS		==========	*/
void PXPartResetPartitions(void)
{
	vDBufferIterate(_vphys.partitions, PXPartitionResetIterateFunc, NULL);
}

void PXPartObjectAssign(vPPhysical phys)
{
	/* find corresponding partition */
	fesetround(FE_TONEAREST);	/* NOT round towards 0, to nearest int */
	vI32 posX = lrintf(phys->transform.position.x / _vphys.partitionSize);
	vI32 posY = lrintf(phys->transform.position.y / _vphys.partitionSize);


}