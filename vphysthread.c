
/* ========== <vphysthread.h>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal worker thread logic for physics thread			*/


/* ========== INCLUDES							==========	*/
#include "vphysthread.h"


/* ========== PHYSICS UPDATE ITERATE FUNC		==========	*/
void vPXPhysicalListIterateUpdateFunc(vHNDL dbHndl, vPPhysical* objectPtr, vPTR input)
{
	vPPhysical object = *objectPtr;
}


/* ========== RENDER THREAD FUNCTIONS			==========	*/
void vPXT_initFunc(vPWorker worker, vPTR workerData, vPGInitializeData input)
{

}

void vPXT_exitFunc(vPWorker worker, vPTR workerData)
{

}

void vPXT_cycleFunc(vPWorker worker, vPTR workerData)
{

}