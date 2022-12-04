
/* ========== <vphysdefs.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Physics macros, defs, structs and types					*/

#ifndef _VPHYS_DEFS_INCLUDE_
#define _VPHYS_DEFS_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"
#include "vgfx.h"
#include "vtypes.h"


/* ========== API DEFINITION					==========	*/
#ifdef VPHYSICS_EXPORTS
#define VPHYSAPI __declspec(dllexport)
#else
#define VPHYSAPI __declspec(dllimport)
#endif


/* ========== DEFINITIONS						==========	*/
#define PHYSICS_OBJECTS_MAX				0x1000
#define FASTMEM_STACK_BYTES				0x1000
#define PHYSOBJECT_LIST_NODE_SIZE		0x200
#define STATICFRICTION_COEFF_DEFAULT	1.55f
#define VPHYS_EPSILON					0.005f
#define PARTITION_CAPACITY_MIN			0x20
#define PARTITION_CAPACITY_STEP			0x40
#define PARTITION_BUFFER_NODE_SIZE		0x80
#define PARTITION_SIZE_DEFAULT			500.0f
#define PARTITION_MINSCALE_MULT			2.25f


/* ========== TYPEDEFS							==========	*/
typedef vPosition vVect;
typedef vVect*    vPVect;
typedef float	  vFloat;
typedef vFloat*   vPFloat;
typedef (*vPXPFPHYSICALUPDATEFUNC)(struct vPhysicial* object);
typedef (*vPXPFPHYSICALCOLLISIONFUNC)(struct vPhysical* self,
	struct vPPhysical* collideObject);


/* ========== STRUCTURES						==========	*/
typedef struct vPhysicsMaterial
{
	vFloat drag;
	vFloat staticFriction;
	vFloat dynamicFriction;
	vFloat bounciness;
} vPhysicsMaterial, *vPPhysicsMaterial;

typedef struct vPhysicsProperties
{
	vUI8  collideLayer;	/* collision layer (ranges from 0 - 255) */

	vBOOL isActive		 : 1;		/* whether the object should be updated				*/
	vBOOL isGhost		 : 1;		/* whether the object should collide w/ nothing		*/
	vBOOL staticPosition : 1;		/* whether the object can be moved					*/
	vBOOL staticRotation : 1;		/* whether the object can be rotated				*/
	vBOOL collideWithParent : 1;	/* whether the object collides w/ it's parent		*/
} vPhysicsProperties, *vPPhysicsProperties;

typedef struct vPhysical
{
	/* ===== PHYSICS METADATA				===== */
	vPObject object;
	vPTR physObjectListPtr;			/* ptr to corresponding element in list				*/

	struct vPhysical* parent;		/* parent physics object							*/
	vUI64 age;						/* ticks spent active								*/

	vBOOL renderableTransformOverride;	/* whether to copy phys transform to rtransform */
	vPGRenderable renderableCache;		/* object's renderable component (if exists)	*/

	/* ==== STATIC PHYSICS PROPERTIES		===== */
	vPhysicsProperties properties;	/* simulation properties							*/
	vPhysicsMaterial   material;	/* physical material properties						*/
	
	/* ==== OBJECT PHYSICS PROPERTIES		===== */
	vGRect boundingRect;			/* bounding rectangle			*/
	vTransform transform;			/* physics transform			*/
	vVect anticipatedPos;			/* not for user acess			*/
	vFloat mass;					/* object mass					*/
	vVect  velocity;				/* change in position			*/
	vVect  acceleration;			/* change of change in position	*/

	/* ==== OBJECT CALLBACKS				===== */
	vPXPFPHYSICALUPDATEFUNC	   updateFunc;
	vPXPFPHYSICALCOLLISIONFUNC collisionFunc;

} vPhysical, *vPPhysical;

typedef struct vPHYSPartition
{
	vBOOL inUse;

	vUI16 x, y;	/* partition coordinates */
	
	vPObject* list;	/* "dyanmic" array of all elements  */
	vUI16 capacity;	/* list capacity (can be increased) */
	vUI16 useage;	/* list useage (always <= capacity) */

} vPHYSPartiton, *vPPHYSPartition;

typedef struct _vPHYSInternals
{
	vBOOL  isInitialized;
	vBOOL  debugMode;
	HANDLE debugModeOutput;
	vUI64  debugFlushInterval;
	vUI64  debugLogCount;

	CRITICAL_SECTION lock;			/* physics lock						*/

	vPWorker physicsThread;			/* worker thread object				*/
	vHNDL physObjectList;			/* dynamic list of phys objects		*/

	vUI16 physComponent;	/* physics component handle	*/

	float partitionSize;	/* space partition size			*/
	vHNDL partitions;		/* dbuffer of space partitions	*/

} _vPHYSInternals, *vPPHYSInternals;
_vPHYSInternals _vphys;	/* INSTANCE	*/

#endif
