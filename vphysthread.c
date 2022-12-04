
/* ========== <vphysthread.c>					==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Internal worker thread logic for physics thread			*/


/* ========== INCLUDES							==========	*/
#include "vphysthread.h"
#include <stdio.h>


/* ========== PHYSICS UPDATE ITERATE FUNC		==========	*/
void vPXPhysicalListIterateUpdateFunc(vHNDL dbHndl, vPPhysical* objectPtr, vPTR input)
{
	vPPhysical pObj = *objectPtr;

	/* if object is inactive, skip */
	if (pObj->properties.isActive == FALSE) return;

	/* increment object's age */
	pObj->age++;

	/* update */


}


/* ========== RENDER THREAD FUNCTIONS			==========	*/
void vPXT_initFunc(vPWorker worker, vPTR workerData, vPTR input)
{

}

void vPXT_exitFunc(vPWorker worker, vPTR workerData)
{

}

void vPXT_cycleFunc(vPWorker worker, vPTR workerData)
{
	vDBufferIterate(_vphys.physObjectList, vPXPhysicalListIterateUpdateFunc, NULL);
}