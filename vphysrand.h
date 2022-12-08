
/* ========== <vphysrand.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Random number generator									*/

#ifndef _VPHYS_RAND_INCLUDE_
#define _VPHYS_RAND_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vphys.h"

/* ========== RANDOM NUMBER GENERATION			==========	*/
VPHYSAPI void vPXRandInit(void);
VPHYSAPI vFloat vPXRandNormalizedSeed(vUI32 seed);
VPHYSAPI vFloat vPXRandNormalized(void);
VPHYSAPI vFloat vPXRandRangeSeed(vUI32 seed, vFloat low, vFloat high);
VPHYSAPI vFloat vPXRandRange(vFloat low, vFloat high);

#endif
