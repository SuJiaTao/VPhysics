
/* ========== <vspacepart.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal space partitioning logic						*/


/* ========== INCLUDES							==========	*/
#include "vspacepart.h"
#include <math.h>
#include <stdio.h>


/* ========== INTERNAL STRUCTS					==========	*/
typedef struct PXPartitionAssignIterateInput
{
	vI32 px, py;
	vPPhysical object;
	vBOOL partitionFound;
} PXPartitionAssignIterateInput, * PXPPartitionAssignIterateInput;

/* ========== HELPERS							==========	*/
static vPTR PXRealloc(vPTR block, SIZE_T oldSize, SIZE_T newSize)
{
	vPTR newBlock = vAllocZeroed(newSize);
	vMemCopy(newBlock, block, oldSize);
	vFree(block);
	return newBlock;
}

static void PXCalculatePartitionValue(vPI32 xOut, vPI32 yOut, vFloat fInx, vFloat fIny)
{
	/* find corresponding partition */
	*xOut = (vI32)floorf(fInx / _vphys.partitionSize);
	*yOut = (vI32)floorf(fIny / _vphys.partitionSize);
}

static void PXEnsurePartitionSizeRequirement(vPPXPartition partition, 
	vI32 sizeRequired)
{
	/* ensure minimum allocation */
	if (partition->list == NULL)
	{
		vPXDebugLogFormatted("Allocating partition [%d %d] list\n",
			partition->x, partition->y);
		partition->list = vAlloc(sizeof(vPPhysical) * PARTITION_CAPACITY_MIN);
		partition->capacity = PARTITION_CAPACITY_MIN;
	}
	
	/* increment partition size (if needed) */
	if (partition->capacity <= sizeRequired)
	{
		vPXDebugLogFormatted("Expanding partition [%d %d] from size %d -> %d\n",
			partition->x, partition->y, partition->capacity,
			partition->capacity + PARTITION_CAPACITY_STEP);

		vUI64 oldSize = partition->capacity;
		partition->capacity += PARTITION_CAPACITY_STEP;
		partition->list = PXRealloc(partition->list, sizeof(vPPhysical) * oldSize,
			sizeof(vPPhysical) * partition->capacity);
	}
}

static void PXAssignObjToPartitionFinalization(vPPXPartition part, vPPhysical pObj)
{
	/* ensure partition is big enough */
	PXEnsurePartitionSizeRequirement(part, part->useage + 1);

	/* add object to partition's object list */
	part->list[part->useage] = pObj;
	part->useage++;

	/* cases to ignore optimization */
	vFloat velMag = pObj->velocity.x + pObj->velocity.y + pObj->angularVelocity;
	if (velMag < PARITION_MINVELOCITY && pObj->age < PARTITION_OPTIMIZE_MINAGE
		|| pObj->properties.noPartitionOptimize)
		velMag = 0xFFFF;

	part->totalVelocity += velMag;
}

static void PXAssignObjToPartitionIterateFunc(vHNDL dbHndl, vPPXPartition partition, 
	PXPPartitionAssignIterateInput input)
{
	/* do not continue to evaluate if solution is found */
	if (input->partitionFound == TRUE) return;

	/* if reached UNUSED partition, means no partitions are left */
	/* mark partition as used and set it to match our purposes   */
	if (partition->inUse == FALSE)
	{
		/* match requirements */
		partition->x = input->px;
		partition->y = input->py;
		partition->layer = input->object->properties.collideLayer;

		/* mark as USED */
		partition->inUse = TRUE;

		/* finalize assigning obj to partition */
		PXAssignObjToPartitionFinalization(partition, input->object);

		/* mark as found */
		input->partitionFound = TRUE;

		/* end */
		return;
	}

	/* check if partition has matching values */
	if (partition->x == input->px && partition->y == input->py 
		&& partition->layer == input->object->properties.collideLayer)
	{
		/* finalize assigning obj to partition */
		PXAssignObjToPartitionFinalization(partition, input->object);

		/* mark as found */
		input->partitionFound = TRUE;
	}
}

static void PXAssignObjectToPartition(vI32 pX, vI32 pY, vPPhysical obj)
{
	/* setup iteration input */
	PXPartitionAssignIterateInput input;
	input.px = pX; input.py = pY;
	input.object = obj;
	input.partitionFound = FALSE;

	/* try and assign object to an allocated partition */
	vDBufferIterate(_vphys.partitions, PXAssignObjToPartitionIterateFunc,
		&input);

	/* on partition found, return */
	if (input.partitionFound == TRUE) return;

	/* if no allocated partitions found, create a new partition to assign */
	vPPXPartition newPartition = vDBufferAdd(_vphys.partitions, NULL);

	/* setup partition parameters */
	newPartition->inUse = TRUE;
	newPartition->x = pX; newPartition->y = pY; 
	newPartition->layer = obj->properties.collideLayer;

	vPXDebugLogFormatted("Created new parition [%d %d]\n",
		newPartition->x, newPartition->y);

	/* finalize assigning to partition */
	PXAssignObjToPartitionFinalization(newPartition, obj);
}

static void PXPartitionResetIterateFunc(vHNDL dbHndl, vPPXPartition partition, vPTR input)
{
	partition->inUse  = FALSE;	/* mark as unused */
	partition->useage = ZERO;	/* reset useage counter */
	partition->totalVelocity = 0.0f;	/* reset total velocity */
}


/* ========== SPACE PARTITIONING FUNCTIONS		==========	*/
void PXPartResetPartitions(void)
{
	vDBufferIterate(_vphys.partitions, PXPartitionResetIterateFunc, NULL);
}

void PXPartObjectOrangizeIntoPartitions(vPPhysical phys)
{
	/* get range of partitions to assign object to */
	vI32 xMin = 0, yMin = 0;
	PXCalculatePartitionValue(&xMin, &yMin,
		phys->worldBound.boundingBox.left, phys->worldBound.boundingBox.bottom);
	vI32 xMax = 0, yMax = 0;
	PXCalculatePartitionValue(&xMax, &yMax,
		phys->worldBound.boundingBox.right, phys->worldBound.boundingBox.top);

	/* for each in range, assign the pObj to that partition */
	for (vI32 pWalkX = xMin; pWalkX <= xMax; pWalkX++)
	{
		for (vI32 pWalkY = yMin; pWalkY <= yMax; pWalkY++)
		{
			PXAssignObjectToPartition(pWalkX, pWalkY, phys);
		}
	}
}