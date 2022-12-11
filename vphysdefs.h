
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
#define PHYSOBJECT_LIST_NODE_SIZE		0x200
#define VPHYS_EPSILON					0.005f
#define PARTITION_CAPACITY_MIN			0x20
#define PARTITION_CAPACITY_STEP			0x40
#define PARTITION_BUFFER_NODE_SIZE		0x80
#define PARTITION_SIZE_DEFAULT			3.0f

#define POS_DEINTERSECT_COEFF			1.00f

#define VPHYS_DEGTORAD					0.0174533f

#define BOUND_MESH_COLORb				255, 128, 32, 255
#define BOUND_MESH_LINESIZE				5.0f
#define BOUND_BOX_COLORb				255, 255, 255, 32
#define BOUND_BOX_LINESIZE				1.0f
#define PARTITION_COLORb				64, 255, 0, 64
#define PARTITION_LINESIZE				3.0f
#define PUSHVECTOR_COLORb				200, 64, 64, 200
#define PUSHVECTOR_LINESIZE				7.5f

#define PARITION_MINVELOCITY			0.01f
#define PARTITION_OPTIMIZE_MINAGE		0x100

#define PROFILER_REFRESH_INTERVAL		0x40

#define RAND_STARTSEED					12345678
#define RAND_NUMTABLE_SIZE				0x800
#define RAND_GRANULARITY				500.0f


/* ========== TYPEDEFS							==========	*/
typedef vPosition vVect;
typedef vVect*    vPVect;
typedef float	  vFloat;
typedef vFloat*   vPFloat;
typedef (*vPXPFPHYSICALUPDATEFUNC)(struct vPhysicial* object);
typedef (*vPXPFPHYSICALCOLLISIONFUNC)(struct vPhysical* self,
	struct vPhysical* collideObject);


/* ========== STRUCTURES						==========	*/
typedef struct vPXWorldBoundMesh
{
	vVect  mesh[4];			/* world-space bound quad	*/
	vVect  center;			/* center vertex			*/
	vGRect boundingBox;		/* world-space bounding box	*/
	vVect  boundingBoxDims;
} vPXWorldBoundMesh, *vPPXWorldBoundMesh;

typedef struct vPXProperties
{
	vUI8  collideLayer;	/* collision layer (ranges from 0 - 255) */

	vBOOL noPartitionOptimize : 1;	/* ignore parition velocity optimizations			*/
	vBOOL isActive		 : 1;		/* whether the object should be updated				*/
	vBOOL isGhost		 : 1;		/* whether the object should collide w/ nothing		*/
	vBOOL staticPosition : 1;		/* whether the object can be moved					*/
	vBOOL staticRotation : 1;		/* whether the object can be rotated				*/
	vBOOL collideWithParent : 1;	/* whether the object collides w/ it's parent		*/
} vPXProperties, *vPPXProperties;

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
	vPXProperties properties;	/* simulation properties							*/
	vFloat drag;
	vFloat friction;
	
	/* ==== OBJECT PHYSICS PROPERTIES		===== */
	vGRect bound;					/* bounding rectangle			*/
	vTransform transform;			/* physics transform			*/
	vFloat mass;					/* object mass					*/
	vVect  velocity;				/* change in position			*/
	vVect  acceleration;			/* change of change in position	*/
	vFloat rotationalVelocity;		/* change in rotation			*/
	vFloat rotationalAcceleration;

	/* ==== CALCULATION INTERMEDIATE DATA	===== */
	vVect anticipatedPos;			/* position if velocity is applied	*/
	vPXWorldBoundMesh worldBound;	/* bound turned into a quad mesh	*/

	/* ==== OBJECT CALLBACKS				===== */
	vPXPFPHYSICALUPDATEFUNC	   updateFunc;
	vPXPFPHYSICALCOLLISIONFUNC collisionFunc;

} vPhysical, *vPPhysical;

typedef struct vPXPartition
{
	vBOOL inUse;

	vI32  x, y;	 /* partition coordinates	*/
	vUI8  layer; /* partition layer			*/

	vFloat totalVelocity;	/* for optimization */
	
	vPPhysical* list;	/* "dyanmic" array of all elements  */
	vUI16 capacity;		/* list capacity (can be increased) */
	vUI16 useage;		/* list useage (always <= capacity) */

} vPXPartiton, *vPPXPartition;

typedef struct _vPXInternals
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

	vPFloat randomNumberTable;

	vFloat partitionSize;	/* space partition size			*/
	vHNDL  partitions;		/* dbuffer of space partitions	*/

} _vPXInternals, *vPPXInternals;
_vPXInternals _vphys;	/* INSTANCE	*/

#endif
