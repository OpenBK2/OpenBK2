#ifndef GR2_GRANNY_H
#define GR2_GRANNY_H

// libgr2: a native reader and animation runtime for the Granny 2 (.gr2) files
// Blitzkrieg 2 ships, in place of RAD Game Tools' proprietary granny2.dll.
//
// This header is the whole public surface. It is C, it includes nothing from
// the engine, and it deliberately reproduces the *shape* of the Granny API it
// replaces: the same 63 entry points, same names, same signatures, same calling
// convention, and a DLL with the same file name. That is what lets the engine be
// relinked against this library without a single source change, and, on Windows,
// lets both implementations run side by side in one process so that every call
// can be asserted against the real one. See docs/GrannyReplacement.md.
//
// The Granny-shaped names are scaffolding with a planned end. Once the engine is
// refactored onto a format-neutral skeleton and pose interface, this header goes
// away and the internal C++17 API becomes the public one.
//
// Three entry points are stubs: the allocator pair, which nothing calls, and
// GrannyConvertSingleObject, whose ExtendedData is null in every shipped file.
// The rest are implemented and measured against the real DLL. See README.md for
// which is which and how each was verified.

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

// Granny's own spelling of the same decoration. A caller does not only *pass*
// callbacks, it declares them, and 3Dmotor/GrannyMemoryMap.cpp declares its
// allocator pair with this macro, so it is part of the compatible surface
// rather than an internal spelling.
#define GRANNY_CALLBACK( ret ) GR2_CALLBACK( ret )

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

// The small fixed-size vectors the records below are written in. Arrays, not
// structs, because that is what they are in the ABI: a granny_triple decays to
// a float pointer at every call site that passes one.
typedef granny_real32 granny_triple[3];
typedef granny_real32 granny_quad[4];
typedef granny_real32 granny_matrix_4x4[4][4];

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

// Records the engine reads directly.
//
// Twenty-odd places in 3Dmotor, SceneB2 and the editor walk these field by
// field: granny_file_info to reach the meshes and models, granny_mesh for
// vertices and topology, granny_skeleton for the bone array, granny_transform
// for a bone's position, orientation and scale-shear. So these are not types
// this library is free to design. They are an ABI, and a wrong offset is not a
// crash, it is a plausible wrong number twenty frames later.
//
// The layouts are the ones src/Structures.h reproduces and pins with
// static_asserts against the sizes granny211.h asserts, for both 32- and
// 64-bit; that file is where the reasoning about each field lives, and its
// asserts now cover these definitions too. The member *names* are fixed as
// well, because the engine spells them at every one of those call sites.
//
// Packed, like everything in this family. Granny states it indirectly, with a
// size check per structure that only holds with no padding at all, and
// granny_bone is the clearest case: packed it is 164 bytes on x64, and at
// natural alignment the eight-byte variant at the end would push it to 168.
#pragma pack( push, 1 )

// A member's storage class in the type tree, and the one enum here whose
// numeric values matter to a caller: the engine switches on them while walking
// a vertex type. The unused names are kept so the numbering cannot drift, which
// is the only thing holding the useful ones in place.
typedef enum granny_member_type
{
	GrannyEndMember,
	GrannyInlineMember,
	GrannyReferenceMember,
	GrannyReferenceToArrayMember,
	GrannyArrayOfReferencesMember,
	GrannyVariantReferenceMember,
	GrannyUnsupportedMemberType_Remove,
	GrannyReferenceToVariantArrayMember,
	GrannyStringMember,
	GrannyTransformMember,
	GrannyReal32Member,
	GrannyInt8Member,
	GrannyUInt8Member,
	GrannyBinormalInt8Member,
	GrannyNormalUInt8Member,
	GrannyInt16Member,
	GrannyUInt16Member,
	GrannyBinormalInt16Member,
	GrannyNormalUInt16Member,
	GrannyInt32Member,
	GrannyUInt32Member,
	GrannyReal16Member,
	GrannyEmptyReferenceMember,
	GrannyOnePastLastMemberType,
	GrannyBool32Member = GrannyInt32Member,
	Grannymember_type_forceint = 0x7fffffff
} granny_member_type;

// Which parts of a transform are meaningful. A part whose bit is clear is the
// identity whatever the bytes say, which GrannyBuildWorldPose was measured to
// honour.
typedef enum granny_transform_flags
{
	GrannyHasPosition = 0x1,
	GrannyHasOrientation = 0x2,
	GrannyHasScaleShear = 0x4,
	Grannytransform_flags_forceint = 0x7fffffff
} granny_transform_flags;

typedef struct granny_data_type_definition granny_data_type_definition;
typedef struct granny_material granny_material;

typedef struct granny_variant
{
	granny_data_type_definition *Type;
	void *Object;
} granny_variant;

struct granny_data_type_definition
{
	granny_member_type Type;
	char const *Name;
	granny_data_type_definition *ReferenceType;
	granny_int32 ArrayWidth;
	granny_int32 Extra[3];
	// TraversalID in 2.5, and pointer sized since, so x86 and x64 differ here.
	granny_uintaddrx Ignored_Ignored;
};

typedef struct granny_transform
{
	granny_uint32 Flags;
	granny_triple Position;
	granny_quad Orientation;
	granny_triple ScaleShear[3];
} granny_transform;

typedef struct granny_art_tool_info
{
	char const *FromArtToolName;
	granny_int32 ArtToolMajorRevision;
	granny_int32 ArtToolMinorRevision;
	granny_int32 ArtToolPointerSize;
	granny_real32 UnitsPerMeter;
	granny_triple Origin;
	granny_triple RightVector;
	granny_triple UpVector;
	granny_triple BackVector;
	granny_variant ExtendedData;
} granny_art_tool_info;

typedef struct granny_exporter_info
{
	char *ExporterName;
	granny_int32 ExporterMajorRevision;
	granny_int32 ExporterMinorRevision;
	granny_int32 ExporterCustomization;
	granny_int32 ExporterBuildNumber;
	granny_variant ExtendedData;
} granny_exporter_info;

typedef struct granny_bone
{
	char const *Name;
	granny_int32 ParentIndex;
	granny_transform LocalTransform;
	granny_matrix_4x4 InverseWorld4x4;
	// Absent from these files. Granny fills it with 0, measured over 60 of them.
	granny_real32 LODError;
	granny_variant ExtendedData;
} granny_bone;

typedef struct granny_skeleton
{
	char const *Name;
	granny_int32 BoneCount;
	granny_bone *Bones;
	// Absent from these files. Granny fills it with 0.
	granny_int32 LODType;
	granny_variant ExtendedData;
} granny_skeleton;

typedef struct granny_tri_material_group
{
	granny_int32 MaterialIndex;
	granny_int32 TriFirst;
	granny_int32 TriCount;
} granny_tri_material_group;

typedef struct granny_tri_annotation_set
{
	char const *Name;
	granny_data_type_definition *TriAnnotationType;
	granny_int32 TriAnnotationCount;
	granny_uint8 *TriAnnotations;
	granny_int32 IndicesMapFromTriToAnnotation;
	granny_int32 TriAnnotationIndexCount;
	granny_int32 *TriAnnotationIndices;
} granny_tri_annotation_set;

typedef struct granny_tri_topology
{
	granny_int32 GroupCount;
	granny_tri_material_group *Groups;
	granny_int32 IndexCount;
	granny_int32 *Indices;
	granny_int32 Index16Count;
	granny_uint16 *Indices16;
	granny_int32 VertexToVertexCount;
	granny_int32 *VertexToVertexMap;
	granny_int32 VertexToTriangleCount;
	granny_int32 *VertexToTriangleMap;
	granny_int32 SideToNeighborCount;
	granny_uint32 *SideToNeighborMap;
	granny_int32 PolygonIndexStartCount;
	granny_int32 *PolygonIndexStarts;
	granny_int32 PolygonIndexCount;
	granny_int32 *PolygonIndices;
	granny_int32 BonesForTriangleCount;
	granny_int32 *BonesForTriangle;
	granny_int32 TriangleToBoneCount;
	granny_int32 *TriangleToBoneIndices;
	granny_int32 TriAnnotationSetCount;
	granny_tri_annotation_set *TriAnnotationSets;
} granny_tri_topology;

typedef struct granny_vertex_annotation_set
{
	char const *Name;
	granny_data_type_definition *VertexAnnotationType;
	granny_int32 VertexAnnotationCount;
	granny_uint8 *VertexAnnotations;
	granny_int32 IndicesMapFromVertexToAnnotation;
	granny_int32 VertexAnnotationIndexCount;
	granny_int32 *VertexAnnotationIndices;
} granny_vertex_annotation_set;

typedef struct granny_vertex_data
{
	granny_data_type_definition *VertexType;
	granny_int32 VertexCount;
	granny_uint8 *Vertices;
	granny_int32 VertexComponentNameCount;
	char const **VertexComponentNames;
	granny_int32 VertexAnnotationSetCount;
	granny_vertex_annotation_set *VertexAnnotationSets;
} granny_vertex_data;

typedef struct granny_morph_target
{
	char const *ScalarName;
	granny_vertex_data *VertexData;
	// Absent from these files. Granny fills it with 0.
	granny_int32 DataIsDeltas;
} granny_morph_target;

typedef struct granny_material_binding
{
	granny_material *Material;
} granny_material_binding;

typedef struct granny_bone_binding
{
	char const *BoneName;
	granny_triple OBBMin;
	granny_triple OBBMax;
	granny_int32 TriangleCount;
	granny_int32 *TriangleIndices;
} granny_bone_binding;

typedef struct granny_mesh
{
	char const *Name;
	granny_vertex_data *PrimaryVertexData;
	granny_int32 MorphTargetCount;
	granny_morph_target *MorphTargets;
	granny_tri_topology *PrimaryTopology;
	granny_int32 MaterialBindingCount;
	granny_material_binding *MaterialBindings;
	granny_int32 BoneBindingCount;
	granny_bone_binding *BoneBindings;
	granny_variant ExtendedData;
} granny_mesh;

typedef struct granny_model_mesh_binding
{
	granny_mesh *Mesh;
} granny_model_mesh_binding;

typedef struct granny_model
{
	char const *Name;
	granny_skeleton *Skeleton;
	granny_transform InitialPlacement;
	granny_int32 MeshBindingCount;
	granny_model_mesh_binding *MeshBindings;
	granny_variant ExtendedData;
} granny_model;

typedef struct granny_pixel_layout
{
	granny_int32 BytesPerPixel;
	granny_int32 ShiftForComponent[4];
	granny_int32 BitsForComponent[4];
} granny_pixel_layout;

typedef struct granny_texture_mip_level
{
	granny_int32 Stride;
	granny_int32 PixelByteCount;
	void *PixelBytes;
} granny_texture_mip_level;

typedef struct granny_texture_image
{
	granny_int32 MIPLevelCount;
	granny_texture_mip_level *MIPLevels;
} granny_texture_image;

typedef struct granny_texture
{
	char const *FromFileName;
	granny_int32 TextureType;
	granny_int32 Width;
	granny_int32 Height;
	granny_int32 Encoding;
	granny_int32 SubFormat;
	granny_pixel_layout Layout;
	granny_int32 ImageCount;
	granny_texture_image *Images;
	granny_variant ExtendedData;
} granny_texture;

typedef struct granny_material_map
{
	char const *Usage;
	granny_material *Material;
} granny_material_map;

struct granny_material
{
	char const *Name;
	granny_int32 MapCount;
	granny_material_map *Maps;
	granny_texture *Texture;
	granny_variant ExtendedData;
};

// What a granny_curve2's variant points at, and the first byte of every curve
// object whatever its format.
typedef struct granny_curve_data_header
{
	granny_uint8 Format;
	granny_uint8 Degree;
} granny_curve_data_header;

// granny_curve_data_format, in the order granny211.h declares the type globals
// in. Only the second is produced here, and the real DLL was measured writing
// exactly that value: these files were written by a 2.5-era exporter, whose
// curves are granny_old_curves, and the conversion turns every one of them into
// the single uncompressed 2.11 format that has the same three fields.
typedef enum granny_curve_data_format
{
	GrannyCurveDataDaKeyframes32fFormat = 0,
	GrannyCurveDataDaK32fC32fFormat = 1,
	Grannycurve_data_format_forceint = 0x7fffffff
} granny_curve_data_format;

typedef struct granny_curve_data_da_k32f_c32f
{
	granny_curve_data_header CurveDataHeader;
	granny_int16 Padding;
	granny_int32 KnotCount;
	granny_real32 *Knots;
	granny_int32 ControlCount;
	granny_real32 *Controls;
} granny_curve_data_da_k32f_c32f;

// The 2.5 shape, which is what is in the file rather than what comes out of it.
typedef struct granny_old_curve
{
	granny_int32 Degree;
	granny_int32 KnotCount;
	granny_real32 *Knots;
	granny_int32 ControlCount;
	granny_real32 *Controls;
} granny_old_curve;

typedef struct granny_curve2
{
	granny_variant CurveData;
} granny_curve2;

typedef struct granny_vector_track
{
	char const *Name;
	// Absent from these files: 2.11 added both.
	granny_uint32 TrackKey;
	granny_int32 Dimension;
	granny_curve2 ValueCurve;
} granny_vector_track;

typedef struct granny_transform_track
{
	char const *Name;
	// Absent from these files.
	granny_int32 Flags;
	// Declared in this order by the ABI, and in the order Position, Orientation,
	// ScaleShear by the file. Members are read by name, so the two orders never
	// have to agree.
	granny_curve2 OrientationCurve;
	granny_curve2 PositionCurve;
	granny_curve2 ScaleShearCurve;
} granny_transform_track;

typedef struct granny_text_track_entry
{
	granny_real32 TimeStamp;
	char const *Text;
} granny_text_track_entry;

typedef struct granny_text_track
{
	char const *Name;
	granny_int32 EntryCount;
	granny_text_track_entry *Entries;
} granny_text_track;

typedef struct granny_periodic_loop
{
	granny_real32 Radius;
	granny_real32 dAngle;
	granny_real32 dZ;
	granny_triple BasisX;
	granny_triple BasisY;
	granny_triple Axis;
} granny_periodic_loop;

typedef struct granny_track_group
{
	char const *Name;
	// The file calls these ScalarTracks.
	granny_int32 VectorTrackCount;
	granny_vector_track *VectorTracks;
	granny_int32 TransformTrackCount;
	granny_transform_track *TransformTracks;
	// Absent from these files: 2.11 added them for LOD.
	granny_int32 TransformLODErrorCount;
	granny_real32 *TransformLODErrors;
	granny_int32 TextTrackCount;
	granny_text_track *TextTracks;
	granny_transform InitialPlacement;
	// The file calls this AccumulationFlags.
	granny_int32 Flags;
	granny_triple LoopTranslation;
	granny_periodic_loop *PeriodicLoop;
	granny_variant ExtendedData;
} granny_track_group;

typedef struct granny_animation
{
	char const *Name;
	granny_real32 Duration;
	granny_real32 TimeStep;
	// Absent from these files: 2.11 added all three.
	granny_real32 Oversampling;
	granny_int32 TrackGroupCount;
	granny_track_group **TrackGroups;
	granny_int32 DefaultLoopCount;
	granny_int32 Flags;
	granny_variant ExtendedData;
} granny_animation;

typedef struct granny_file_info
{
	granny_art_tool_info *ArtToolInfo;
	granny_exporter_info *ExporterInfo;
	char const *FromFileName;
	granny_int32 TextureCount;
	granny_texture **Textures;
	granny_int32 MaterialCount;
	granny_material **Materials;
	granny_int32 SkeletonCount;
	granny_skeleton **Skeletons;
	granny_int32 VertexDataCount;
	granny_vertex_data **VertexDatas;
	granny_int32 TriTopologyCount;
	granny_tri_topology **TriTopologies;
	granny_int32 MeshCount;
	granny_mesh **Meshes;
	granny_int32 ModelCount;
	granny_model **Models;
	granny_int32 TrackGroupCount;
	granny_track_group **TrackGroups;
	granny_int32 AnimationCount;
	granny_animation **Animations;
	granny_variant ExtendedData;
} granny_file_info;

#pragma pack( pop )

// The vertex component names the engine looks for in a granny_vertex_data's
// VertexComponentNames. Macros rather than exported strings, as in the ABI, so
// nothing links against them.
#define GrannyVertexPositionName "Position"
#define GrannyVertexNormalName "Normal"
#define GrannyVertexTangentName "Tangent"
#define GrannyVertexBinormalName "Binormal"
#define GrannyVertexTangentBinormalCrossName "TangentBinormalCross"
#define GrannyVertexBoneWeightsName "BoneWeights"
#define GrannyVertexBoneIndicesName "BoneIndices"
#define GrannyVertexDiffuseColorName "DiffuseColor"
#define GrannyVertexSpecularColorName "SpecularColor"
#define GrannyVertexTextureCoordinatesName "TextureCoordinates"

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
GR2_API( granny_int32x ) GrannyGetMeshVertexCount( granny_mesh const *Mesh );
GR2_API( void * ) GrannyGetMeshVertices( granny_mesh const *Mesh );
GR2_API( granny_data_type_definition * ) GrannyGetMeshVertexType( granny_mesh const *Mesh );
GR2_API( granny_int32x ) GrannyGetMeshIndexCount( granny_mesh const *Mesh );
GR2_API( void * ) GrannyGetMeshIndices( granny_mesh const *Mesh );
GR2_API( granny_int32x ) GrannyGetMeshBytesPerIndex( granny_mesh const *Mesh );
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

// Which Granny this claims to be. 2.11.8.0, the ABI reproduced here.
//
// Two of these, deliberately. The macros are what the caller was *compiled*
// against and the entry points are what it is *running* against, and the whole
// point of GrannyVersionsMatch is to compare them: with one implementation in
// the process they cannot disagree, but on Windows a real granny2.dll earlier
// on the search path can still answer these calls, and then the mismatch is the
// report. src/Version.cpp holds the same four numbers separately for that
// reason. The map editor logs the string at startup and shows it in its About
// box (MapEditorLib/BuildDetailsWx.cpp).
#define GrannyProductVersion "2.11.8.0"
#define GrannyProductMajorVersion 2
#define GrannyProductMinorVersion 11
#define GrannyProductBuildNumber 8
#define GrannyProductCustomization 0
#define GrannyProductReleaseName release
#define GrannyVersionsMatch                                                            \
	GrannyVersionsMatch_( GrannyProductMajorVersion, GrannyProductMinorVersion,          \
	                      GrannyProductBuildNumber, GrannyProductCustomization )

GR2_API( char const * ) GrannyGetVersionString( void );
GR2_API( bool ) GrannyVersionsMatch_( granny_int32x MajorVersion, granny_int32x MinorVersion,
                                      granny_int32x BuildNumber, granny_int32x Customization );

#if defined( __cplusplus )
}
#endif

#endif
