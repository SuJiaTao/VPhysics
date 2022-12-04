
/* ========== <vphysical.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vphysical.h"


/* ========== COMPONENT CALLBACKS				==========	*/
void vPXPhysical_initFunc(vPObject object, vPComponent component, vPTR input)
{
	/* get input copy (from heap) */
	vPPhysical copy = input;
	vPPhysical self = component->objectAttribute;

	/* copy contents and free */
	vMemCopy(self, copy, sizeof(vPhysical));
	vFree(copy);

	/* add to internal physics object list buffer		*/
	/* refer to <vphyscore.c> for add implementation	*/
	vDBufferAdd(_vphys.physObjectList, self);

	/* if debug mode, log the creation */
	if (vPXIsDebug())
	{
		vPCHAR stringMessage = vPXDebugPhysicalToStringNew(self);
		vPXDebugLogFormatted("Created New %s\n", stringMessage);
		vFree(stringMessage);
	}
}

void vPXPhysical_destroyFunc(vPObject object, vPComponent component)
{
	vPPhysical self = component->objectAttribute;
	vDBufferRemove(_vphys.physObjectList, self->physObjectListPtr);
}
