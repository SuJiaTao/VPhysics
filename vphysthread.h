
/* ========== <vphysthread.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal worker thread logic for physics thread			*/

#ifndef _VPHYS_INTERNAL_THREAD_INCLUDE_
#define _VPHYS_INTERNAL_THREAD_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vphys.h"


/* ========== RENDER THREAD FUNCTIONS			==========	*/
void vPXT_initFunc(vPWorker worker, vPTR workerData, vPTR input);
void vPXT_exitFunc(vPWorker worker, vPTR workerData);
void vPXT_cycleFunc(vPWorker worker, vPTR workerData);


#endif
