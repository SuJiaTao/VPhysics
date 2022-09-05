
/* ========== <vphysdefs.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Physics macros, defs, structs and types					*/

#ifndef _VPHYS_DEFS_INCLUDE_
#define _VPHYS_DEFS_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"
#include "vgfx.h"


/* ========== DEFINITIONS						==========	*/
#define PHYSICS_RENDEROBJECTS_MAX		0x20


/* ========== STRUCTURES						==========	*/
typedef struct vPhysicsMaterial
{
	float staticFriction;
	float dynamicFriction;
	float bounciness;
	float mass;
} vPhysicsMaterial, *vPPhysicsMaterial;

typedef struct vPhysicsState
{
	vUI16 collideLayer;				/* bitfield of layers that object exists on. will	*/
									/* collide w/ objects that are on same layer (any)	*/
	vUI16 noCollideLayer;			/* will not collide with object that have the same	*/
									/* layer (any). overrides collideLayer				*/

	vBOOL isActive		 : 1;		/* whether the object should be updated				*/
	vBOOL staticPosition : 1;		/* whether the object can be moved					*/
	vBOOL staticRotation : 1;		/* whether the object can be rotated				*/
	vBOOL collideWithParent : 1;	/* whether the object collides w/ it's parent		*/
} vPhysicsState, *vPPhysicsState;

typedef struct vPhysicsObject
{
	vPPhysicsObject parent;			/* parent physics object							*/

	vPhysicsState state;			/* simulation properties							*/
	vPhysicsMaterial material;		/* physical properties								*/
	
	vRect boundingRect;				/* bounding rectangle								*/
	vTransform2 transform;			/* object transform									*/

	/* optional associated render objects				*/
	vPRenderObject renderObjects[PHYSICS_RENDEROBJECTS_MAX];
	

} vPhysicsObject, *vPPhysicsObject;

typedef struct _vPHYSInternals
{
	vBOOL isInitialized;
} _vPHYSInternals, *vPPHYSInternals;

#endif
