
/* ========== <vphysical.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vphysical.h"


/* ========== COMPONENT CALLBACKS				==========	*/
void vPXPhysical_initFunc(vPObject object, vPComponent component, vPTR input)
{
	/* get input copy (from heap) */
	vPPhysicsObject copy = input;
	vPPhysicsObject self = component->objectAttribute;

	/* copy contents and free */
	vMemCopy(self, copy, sizeof(vPhysicsObject));
	vFree(copy);

	/* add to internal physics object list buffer */
	vDBufferAdd(_vphys.physObjectList, self);
	
}

void vPXPhysical_destroyFunc(vPObject object, vPComponent component)
{

}
