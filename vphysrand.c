
/* ========== <vphysrand.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Random number generator									*/

/* ========== INCLUDES							==========	*/
#include "vphysrand.h"
#include <math.h>
#include <stdio.h>

/* ========== RANDOM NUMBER GENERATION			==========	*/
VPHYSAPI void vPXRandInit(void)
{
	/* allocate table */
	vLogInfo(__func__, "Initializing Physics random number table.");
	_vphys.randomNumberTable = vAllocZeroed(sizeof(vFloat) * RAND_NUMTABLE_SIZE);

	/* setup first random number */
	_vphys.randomNumberTable[0] = RAND_STARTSEED;

	/* generate large random numbers */
	for (int i = 1; i < RAND_NUMTABLE_SIZE; i++)
	{
		vFloat num = _vphys.randomNumberTable[i - 1];
		num = fmodf(RAND_VAL_A * num + RAND_VAL_C, RAND_VAL_M);
		_vphys.randomNumberTable[i] = num;
	}

	/* mod and normalize (range -1 to 1) */
	for (int i = 0; i < RAND_NUMTABLE_SIZE; i++)
	{
		vFloat num = _vphys.randomNumberTable[i];
		num = fabsf(num);
		num = fmodf(num, RAND_GRANULARITY * 2);

		num -= RAND_GRANULARITY;
		num /= RAND_GRANULARITY;
		_vphys.randomNumberTable[i] = num;
	}
}

VPHYSAPI vFloat vPXRandNormalizedSeed(vUI32 seed)
{
	return _vphys.randomNumberTable[seed % RAND_NUMTABLE_SIZE];
}

static vUI32 __internalRNSeed = 0;
VPHYSAPI vFloat vPXRandNormalized(void)
{
	return vPXRandNormalizedSeed(__internalRNSeed++);
}

VPHYSAPI vFloat vPXRandRangeSeed(vUI32 seed, vFloat low, vFloat high)
{
	vFloat range = high - low;
	return low + (vPXFastFabs(vPXRandNormalizedSeed(seed)) * range);
}

static vUI32 __internalRRSeed = 0;
VPHYSAPI vFloat vPXRandRange(vFloat low, vFloat high)
{
	return vPXRandRangeSeed(__internalRRSeed++, low, high);
}
