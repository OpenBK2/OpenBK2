#ifndef GR2_GRANNY_H
#define GR2_GRANNY_H

// libgr2: a native reader and animation runtime for the Granny 2 (.gr2) files
// Blitzkrieg 2 ships, in place of RAD Game Tools' proprietary granny2.dll.
//
// This header is the whole public surface. It is C, it includes nothing from
// the engine, and it deliberately reproduces the *shape* of the Granny API it
// replaces: the same 55 entry points, same names, same signatures, same calling
// convention, and a DLL with the same file name. That is what lets the engine be
// relinked against this library without a single source change, and, on Windows,
// lets both implementations run side by side in one process so that every call
// can be asserted against the real one. See docs/GrannyReplacement.md.
//
// The Granny-shaped names are scaffolding with a planned end. Once the engine is
// refactored onto a format-neutral skeleton and pose interface, this header goes
// away and the internal C++17 API becomes the public one.
//
// Nothing here is implemented yet. Every entry point is a stub that returns a
// null, a zero or a false. This is the skeleton: it fixes the header, the build,
// the export set and the file layout, so that the milestones can be filled in
// one at a time against a target that already links.

#include <stdint.h>

#if !defined( __cplusplus )
// Granny declares its predicates as plain bool. In C++ that is the built-in
// type; in C it has to come from somewhere, and stdbool.h's _Bool is one byte on
// every toolchain this targets, which is what the ABI needs it to be.
#include <stdbool.h>
#endif

// How an entry point is decorated, and how it is called.
//
// Granny on Windows is __stdcall with __cdecl callbacks, which is the default
// for neither, so both have to be stated. Off Windows there is no calling
// convention to choose and visibility is the only question.
//
// Define GR2_BUILD_SHARED when compiling this library into a shared object, and
// GR2_STATIC when linking it statically. The unqualified case is a consumer
// importing from the DLL, which is the common one and so the default.
//
// Raw platform macros rather than boost/predef, against the tree's usual rule,
// because this is a public C header of a library that is meant to be extracted
// and used on its own. Requiring Boost of everyone who includes it would cost
// more than the rule buys here.
#if defined( _WIN32 )
	#if defined( GR2_STATIC )
		#define GR2_API( ret ) ret __stdcall
	#elif defined( GR2_BUILD_SHARED )
		#define GR2_API( ret ) __declspec( dllexport ) ret __stdcall
	#else
		#define GR2_API( ret ) __declspec( dllimport ) ret __stdcall
	#endif
	#define GR2_CALLBACK( ret ) ret __cdecl
#else
	#if defined( GR2_STATIC )
		#define GR2_API( ret ) ret
	#else
		#define GR2_API( ret ) __attribute__( ( visibility( "default" ) ) ) ret
	#endif
	#define GR2_CALLBACK( ret ) ret
#endif

#if defined( __cplusplus )
extern "C"
{
#endif

// The scalar types the entry points below are written in.
//
// granny_int32x is the one worth reading twice: the trailing x reads like
// "natural width for the machine", but Granny fixes it at 32 bits on every
// target, x64 included. Widening it here would silently change the ABI of two
// thirds of this API.
typedef int8_t granny_int8;
typedef int16_t granny_int16;
typedef int32_t granny_int32;
typedef uint8_t granny_uint8;
typedef uint16_t granny_uint16;
typedef uint32_t granny_uint32;

typedef granny_int32 granny_int32x;
typedef granny_uint32 granny_uint32x;
typedef uintptr_t granny_uintaddrx;

typedef float granny_real32;

// Handles the engine passes back in, none of which it needs the layout of.
//
// These stay opaque permanently. They are runtime objects this library owns and
// hands out, and the engine only ever holds pointers to them.
typedef struct granny_file granny_file;
typedef struct granny_model_instance granny_model_instance;
typedef struct granny_local_pose granny_local_pose;
typedef struct granny_world_pose granny_world_pose;
typedef struct granny_control granny_control;
typedef struct granny_controlled_animation_builder granny_controlled_animation_builder;
typedef struct granny_track_mask granny_track_mask;

// Records the engine reads directly, opaque only because there is no
// implementation yet.
//
// Twenty-odd places in 3Dmotor and SceneB2 walk these structures field by field:
// granny_file_info to reach the meshes and models, granny_mesh for vertices and
// topology, granny_skeleton for the bone array, granny_transform for a bone's
// position, orientation and scale-shear. Each therefore has a layout this
// library has to reproduce exactly, and each gains its real definition in the
// milestone that first needs it: the type tree at M1, geometry at M2, skeleton,
// transform and curve at M3.
typedef struct granny_file_info granny_file_info;
typedef struct granny_data_type_definition granny_data_type_definition;
typedef struct granny_mesh granny_mesh;
typedef struct granny_model granny_model;
typedef struct granny_skeleton granny_skeleton;
typedef struct granny_animation granny_animation;
typedef struct granny_track_group granny_track_group;
typedef struct granny_curve2 granny_curve2;
typedef struct granny_transform granny_transform;

// What a track group does with the root bone's motion. Passed by value, so
// unlike the structures above it needs its values now rather than at M4.
typedef enum granny_accumulation_mode
{
	GrannyNoAccumulation,
	GrannyConstantExtractionAccumulation,
	GrannyVariableDeltaAccumulation,
	Grannyaccumulation_mode_forceint = 0x7fffffff
} granny_accumulation_mode;

// An installed allocator is called back into the caller's code, so these keep
// Granny's __cdecl rather than the __stdcall of the entry points around them.
// The engine has a pair of these ready in 3Dmotor/GrannyMemoryMap.cpp and never
// installs them, but the signatures are part of the ABI either way.
typedef GR2_CALLBACK( void * ) granny_allocate_callback( char const *File, granny_int32x Line,
                                                        granny_uintaddrx Alignment,
                                                        granny_uintaddrx Size,
                                                        granny_int32x AllocationIntent );
typedef GR2_CALLBACK( void ) granny_deallocate_callback( char const *File, granny_int32x Line,
                                                         void *Memory );

// Called per member by GrannyConvertSingleObject, to take over the conversion of
// one member the caller wants handled its own way. __cdecl for the same reason
// as the allocator callbacks above.
typedef GR2_CALLBACK( bool ) granny_conversion_handler(
	granny_data_type_definition const *SourceType, void const *SourceMember,
	granny_data_type_definition const *DestType, void *DestMember );

// Memory. Exported because the engine links them, but never called by it.
GR2_API( void ) GrannyGetAllocator( granny_allocate_callback **AllocateCallback,
                                    granny_deallocate_callback **DeallocateCallback );
GR2_API( void ) GrannySetAllocator( granny_allocate_callback *AllocateCallback,
                                    granny_deallocate_callback *DeallocateCallback );

// Container: sections, fixups, the two Oodle codecs, and the parse into owned
// structures. M1.
GR2_API( granny_file * ) GrannyReadEntireFile( char const *FileName );
GR2_API( granny_file * ) GrannyReadEntireFileFromMemory( granny_int32x MemorySize,
                                                         void const *Memory );
GR2_API( void ) GrannyFreeFile( granny_file *File );
GR2_API( granny_file_info * ) GrannyGetFileInfo( granny_file *File );

// Type tree. M1.
GR2_API( granny_int32x ) GrannyGetMemberTypeSize( granny_data_type_definition const *MemberType );
GR2_API( granny_int32x )
	GrannyGetTotalObjectSize( granny_data_type_definition const *TypeDefinition );
GR2_API( void ) GrannyConvertSingleObject( granny_data_type_definition const *SourceType,
                                           void const *SourceObject,
                                           granny_data_type_definition const *DestType,
                                           void *DestObject,
                                           granny_conversion_handler *OverrideHandler );

// Geometry. M2.
GR2_API( granny_int32x ) GrannyGetMeshTriangleGroupCount( granny_mesh const *Mesh );
GR2_API( bool ) GrannyMeshIsRigid( granny_mesh const *Mesh );
GR2_API( bool ) GrannyFindBoneByName( granny_skeleton const *Skeleton, char const *BoneName,
                                      granny_int32x *BoneIndex );

// Transforms. M3.
GR2_API( void ) GrannyMakeIdentity( granny_transform *Result );
GR2_API( void ) GrannyPostMultiplyBy( granny_transform *Transform,
                                      granny_transform const *PostMult );

// Model instances. M3.
GR2_API( granny_model_instance * ) GrannyInstantiateModel( granny_model const *Model );
GR2_API( void ) GrannyFreeModelInstance( granny_model_instance *ModelInstance );
GR2_API( void ) GrannySetModelClock( granny_model_instance const *ModelInstance,
                                     granny_real32 NewClock );

// Curve sampling and pose evaluation. M3.
GR2_API( granny_local_pose * ) GrannyNewLocalPose( granny_int32x BoneCount );
GR2_API( void ) GrannyFreeLocalPose( granny_local_pose *LocalPose );
GR2_API( granny_int32x ) GrannyGetLocalPoseBoneCount( granny_local_pose const *LocalPose );
GR2_API( granny_transform * ) GrannyGetLocalPoseTransform( granny_local_pose const *LocalPose,
                                                           granny_int32x BoneIndex );
GR2_API( granny_world_pose * ) GrannyNewWorldPose( granny_int32x BoneCount );
GR2_API( void ) GrannyFreeWorldPose( granny_world_pose *WorldPose );
GR2_API( void ) GrannyBuildWorldPose( granny_skeleton const *Skeleton, granny_int32x FirstBone,
                                      granny_int32x BoneCount, granny_local_pose const *LocalPose,
                                      granny_real32 const *Offset4x4, granny_world_pose *Result );
GR2_API( granny_real32 * ) GrannyGetWorldPose4x4( granny_world_pose const *WorldPose,
                                                  granny_int32x BoneIndex );
GR2_API( granny_real32 * ) GrannyGetWorldPoseComposite4x4( granny_world_pose const *WorldPose,
                                                           granny_int32x BoneIndex );
GR2_API( void ) GrannySampleModelAnimations( granny_model_instance const *ModelInstance,
                                             granny_int32x FirstBone, granny_int32x BoneCount,
                                             granny_local_pose *Result );
GR2_API( void ) GrannyEvaluateCurveAtT( granny_int32x Dimension, bool Normalize, bool BackwardsLoop,
                                        granny_curve2 const *Curve, bool ForwardsLoop,
                                        granny_real32 CurveDuration, granny_real32 t,
                                        granny_real32 *Result,
                                        granny_real32 const *IdentityVector );

// Binding an animation to a model, and masking which bones it reaches. M4.
GR2_API( granny_controlled_animation_builder * )
	GrannyBeginControlledAnimation( granny_real32 StartTime, granny_animation const *Animation );
GR2_API( granny_control * )
	GrannyEndControlledAnimation( granny_controlled_animation_builder *Builder );
GR2_API( void ) GrannySetTrackGroupTarget( granny_controlled_animation_builder *Builder,
                                           granny_int32x TrackGroupIndex,
                                           granny_model_instance *Model );
GR2_API( void ) GrannySetTrackGroupAccumulation( granny_controlled_animation_builder *Builder,
                                                 granny_int32x TrackGroupIndex,
                                                 granny_accumulation_mode Mode );
GR2_API( void ) GrannySetTrackGroupModelMask( granny_controlled_animation_builder *Builder,
                                              granny_int32x TrackGroupIndex,
                                              granny_track_mask *ModelMask );
GR2_API( granny_track_mask * ) GrannyNewTrackMask( granny_real32 DefaultWeight,
                                                   granny_int32x BoneCount );
GR2_API( void ) GrannySetSkeletonTrackMaskFromTrackGroup( granny_track_mask *Mask,
                                                          granny_skeleton const *Skeleton,
                                                          granny_track_group const *TrackGroup,
                                                          granny_real32 IdentityValue,
                                                          granny_real32 ConstantValue,
                                                          granny_real32 AnimatedValue );

// Playback: clocks, speed, looping, completion, and the ease curves that weight
// one clip against another. M4, and the part no open source project has written,
// because importers and viewers never need it.
GR2_API( void ) GrannyFreeControl( granny_control *Control );
GR2_API( void ) GrannyFreeControlOnceUnused( granny_control *Control );
GR2_API( bool ) GrannyControlIsComplete( granny_control const *Control );
GR2_API( void ) GrannyCompleteControlAt( granny_control *Control, granny_real32 AtSeconds );
GR2_API( void ) GrannySetControlActive( granny_control *Control, bool Active );
GR2_API( granny_real32 ) GrannyGetControlClampedLocalClock( granny_control *Control );
GR2_API( void ) GrannySetControlRawLocalClock( granny_control *Control, granny_real32 LocalClock );
GR2_API( granny_real32 ) GrannyGetControlDuration( granny_control const *Control );
GR2_API( granny_real32 ) GrannyGetControlDurationLeft( granny_control *Control );
GR2_API( granny_real32 ) GrannyGetControlEffectiveWeight( granny_control const *Control );
GR2_API( granny_real32 ) GrannyGetControlSpeed( granny_control const *Control );
GR2_API( void ) GrannySetControlSpeed( granny_control *Control, granny_real32 Speed );
GR2_API( void ) GrannySetControlLoopCount( granny_control *Control, granny_int32x LoopCount );
GR2_API( void ) GrannySetControlForceClampedLooping( granny_control *Control, bool Clamp );
GR2_API( granny_real32 ) GrannyEaseControlIn( granny_control *Control, granny_real32 Duration,
                                              bool FromCurrent );
GR2_API( granny_real32 ) GrannyEaseControlOut( granny_control *Control, granny_real32 Duration );
GR2_API( void ) GrannySetControlEaseIn( granny_control *Control, bool EaseIn );
GR2_API( void ) GrannySetControlEaseOut( granny_control *Control, bool EaseOut );
GR2_API( void ) GrannySetControlEaseInCurve( granny_control *Control, granny_real32 StartSeconds,
                                             granny_real32 EndSeconds, granny_real32 StartValue,
                                             granny_real32 StartTangent, granny_real32 EndTangent,
                                             granny_real32 EndValue );
GR2_API( void ) GrannySetControlEaseOutCurve( granny_control *Control, granny_real32 StartSeconds,
                                              granny_real32 EndSeconds, granny_real32 StartValue,
                                              granny_real32 StartTangent, granny_real32 EndTangent,
                                              granny_real32 EndValue );

#if defined( __cplusplus )
}
#endif

#endif
