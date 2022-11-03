
/* ========== <vphysdefs.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Physics macros, defs, structs and types					*/

#ifndef _VPHYS_DEFS_INCLUDE_
#define _VPHYS_DEFS_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"
#include "vgfx.h"


/* ========== DEFINITIONS						==========	*/
#define PHYSICS_OBJECTS_MAX				0x1000
#define PHYSICS_RENDEROBJECTS_MAX		0x08
#define FASTMEM_STACK_BYTES				0x1000


/* ========== TYPEDEFS							==========	*/
typedef vPosition vVect;
typedef vVect*    vPVect;
typedef float	  vFloat;


/* ========== STRUCTURES						==========	*/
typedef struct vPhysicsMaterial
{
	vFloat staticFriction;
	vFloat dynamicFriction;
	vFloat bounciness;
} vPhysicsMaterial, *vPPhysicsMaterial;

typedef struct vPhysicsProperties
{
	vUI16 collideLayer;				/* bitfield of layers that object exists on. will	*/
									/* collide w/ objects that are on same layer (any).	*/
									/* when set to ZERO is the same as isGhost == TRUE	*/

	vUI16 noCollideLayer;			/* will not collide with object that have the same	*/
									/* layer (any). overrides collideLayer				*/

	vBOOL isActive		 : 1;		/* whether the object should be updated				*/
	vBOOL isGhost		 : 1;		/* whether the object should collide w/ nothing		*/
	vBOOL staticPosition : 1;		/* whether the object can be moved					*/
	vBOOL staticRotation : 1;		/* whether the object can be rotated				*/
	vBOOL collideWithParent : 1;	/* whether the object collides w/ it's parent		*/
} vPhysicsProperties, *vPPhysicsProperties;

typedef struct vPhysicsObject
{
	/* ===== PHYSICS METADATA				===== */
	vPPhysicsObject parent;			/* parent physics object							*/
	vUI64 age;						/* ticks spent active								*/
	vPGRenderable visuals[PHYSICS_RENDEROBJECTS_MAX];	/* render objects				*/

	/* ==== STATIC PHYSICS PROPERTIES		===== */
	vPPhysicsProperties properties;	/* simulation properties							*/
	vPPhysicsMaterial   material;	/* physical material properties						*/
	
	/* ==== OBJECT PHYSICS PROPERTIES		===== */
	vGRect boundingRect;			/* bounding rectangle			*/
	vTransform physicsTransform;	/* physics transform			*/
	vFloat mass;					/* object mass					*/
	vVect  velocity;				/* change in position			*/
	vVect  acceleration;			/* change of change in position	*/

} vPhysicsObject, *vPPhysicsObject;

/* pre-allocated memory stack */
typedef struct _vPHYSFastMemoryStack
{
	CRITICAL_SECTION rwLock;
	vPBYTE block;
	vUI64  ptr;
} _vPHYSFastMemoryStack, *_vPPHYSFastMemoryStack;

typedef struct _vPHYSInternals
{
	vBOOL isInitialized;
	CRITICAL_SECTION lock;			/* physics lock						*/

	vPWorker physicsThread;			/* worker thread object				*/
	vHNDL physObjectList;			/* dynamic list of phys objects		*/

	_vPHYSFastMemoryStack fastMemStack;	/* quick allocation stack		*/


} _vPHYSInternals, *vPPHYSInternals;
_vPHYSInternals _vphys;	/* INSTANCE	*/

#endif
