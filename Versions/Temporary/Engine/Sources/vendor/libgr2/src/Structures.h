#pragma once

// The structures the engine reads, laid out the way granny211.h declares them.
//
// This is the *target* of the conversion, not the shape of the file. The engine
// compiles against granny211.h, by way of a two-line shim at
// vendor/granny/include/granny.h, so these layouts are an ABI: a wrong offset is
// not a crash, it is a plausible wrong number twenty frames later.
//
// Two things make them what they are.
//
// They are packed. Granny asserts it indirectly throughout granny211.h, with
// GrannyTypeSizeCheck(sizeof(T) == sizeof(a) + sizeof(b) + ...) for every
// structure, which only holds with no padding. granny_bone is the clearest case:
// packed it is 164 bytes on x64, and at natural alignment the eight-byte variant
// at the end would push it to 168. The static_asserts at the bottom of this file
// are that check, restated where a compiler will enforce it.
//
// And they are 2.11's, not the file's. These files were written by a 2.5-era
// exporter and carry 2.5-era type trees: a bone in them has seven members ending
// LightInfo, CameraInfo and ExtendedData, where 2.11 has six ending LODError and
// ExtendedData, and the two in between were renamed. Convert.cpp is where one
// becomes the other. See docs/GrannyReplacement.md, "The structures in the file
// are not the structures the engine reads".

#include <cstdint>

// MSVC, GCC and Clang all take this form, which is why it is used rather than an
// attribute that only two of them accept.
#pragma pack( push, 1 )

namespace NGr2
{

struct SDataTypeDefinition;

struct SVariant
{
	SDataTypeDefinition *pType;
	void *pObject;
};

//! granny_transform_flags. Each bit says whether the matching part of a
//! transform is meaningful; a part whose bit is clear is the identity whatever
//! the bytes say, which GrannyBuildWorldPose was measured to honour.
enum ETransformFlags
{
	TRANSFORM_HAS_POSITION = 0x1,
	TRANSFORM_HAS_ORIENTATION = 0x2,
	TRANSFORM_HAS_SCALESHEAR = 0x4,
};

struct STransform
{
	uint32_t nFlags;
	float Position[3];
	float Orientation[4];
	float ScaleShear[3][3];
};

struct SDataTypeDefinition
{
	int32_t nType;
	const char *pszName;
	SDataTypeDefinition *pReferenceType;
	int32_t nArrayWidth;
	int32_t Extra[3];
	//! TraversalID in 2.5, and pointer sized since, so x86 and x64 differ here.
	uintptr_t nIgnored;
};

struct SArtToolInfo
{
	const char *pszFromArtToolName;
	int32_t nArtToolMajorRevision;
	int32_t nArtToolMinorRevision;
	int32_t nArtToolPointerSize;
	float fUnitsPerMeter;
	float Origin[3];
	float RightVector[3];
	float UpVector[3];
	float BackVector[3];
	SVariant ExtendedData;
};

struct SExporterInfo
{
	const char *pszExporterName;
	int32_t nExporterMajorRevision;
	int32_t nExporterMinorRevision;
	int32_t nExporterCustomization;
	int32_t nExporterBuildNumber;
	SVariant ExtendedData;
};

struct SBone
{
	const char *pszName;
	int32_t nParentIndex;
	STransform LocalTransform;
	float InverseWorld4x4[16];
	//! Absent from these files. Granny fills it with 0, measured over 60 of them.
	float fLODError;
	SVariant ExtendedData;
};

struct SSkeleton
{
	const char *pszName;
	int32_t nBoneCount;
	SBone *pBones;
	//! Absent from these files. Granny fills it with 0.
	int32_t nLODType;
	SVariant ExtendedData;
};

struct STriMaterialGroup
{
	int32_t nMaterialIndex;
	int32_t nTriFirst;
	int32_t nTriCount;
};

struct STriAnnotationSet
{
	const char *pszName;
	SDataTypeDefinition *pTriAnnotationType;
	int32_t nTriAnnotationCount;
	uint8_t *pTriAnnotations;
	int32_t nIndicesMapFromTriToAnnotation;
	int32_t nTriAnnotationIndexCount;
	int32_t *pTriAnnotationIndices;
};

struct STriTopology
{
	int32_t nGroupCount;
	STriMaterialGroup *pGroups;
	int32_t nIndexCount;
	int32_t *pIndices;
	int32_t nIndex16Count;
	uint16_t *pIndices16;
	int32_t nVertexToVertexCount;
	int32_t *pVertexToVertexMap;
	int32_t nVertexToTriangleCount;
	int32_t *pVertexToTriangleMap;
	int32_t nSideToNeighborCount;
	uint32_t *pSideToNeighborMap;
	int32_t nPolygonIndexStartCount;
	int32_t *pPolygonIndexStarts;
	int32_t nPolygonIndexCount;
	int32_t *pPolygonIndices;
	int32_t nBonesForTriangleCount;
	int32_t *pBonesForTriangle;
	int32_t nTriangleToBoneCount;
	int32_t *pTriangleToBoneIndices;
	int32_t nTriAnnotationSetCount;
	STriAnnotationSet *pTriAnnotationSets;
};

struct SVertexAnnotationSet
{
	const char *pszName;
	SDataTypeDefinition *pVertexAnnotationType;
	int32_t nVertexAnnotationCount;
	uint8_t *pVertexAnnotations;
	int32_t nIndicesMapFromVertexToAnnotation;
	int32_t nVertexAnnotationIndexCount;
	int32_t *pVertexAnnotationIndices;
};

struct SVertexData
{
	SDataTypeDefinition *pVertexType;
	int32_t nVertexCount;
	uint8_t *pVertices;
	int32_t nVertexComponentNameCount;
	const char **ppVertexComponentNames;
	int32_t nVertexAnnotationSetCount;
	SVertexAnnotationSet *pVertexAnnotationSets;
};

struct SMorphTarget
{
	const char *pszScalarName;
	SVertexData *pVertexData;
	//! Absent from these files. Granny fills it with 0.
	int32_t nDataIsDeltas;
};

struct SMaterial;

struct SMaterialBinding
{
	SMaterial *pMaterial;
};

struct SBoneBinding
{
	const char *pszBoneName;
	float OBBMin[3];
	float OBBMax[3];
	int32_t nTriangleCount;
	int32_t *pTriangleIndices;
};

struct SMesh
{
	const char *pszName;
	SVertexData *pPrimaryVertexData;
	int32_t nMorphTargetCount;
	SMorphTarget *pMorphTargets;
	STriTopology *pPrimaryTopology;
	int32_t nMaterialBindingCount;
	SMaterialBinding *pMaterialBindings;
	int32_t nBoneBindingCount;
	SBoneBinding *pBoneBindings;
	SVariant ExtendedData;
};

struct SModelMeshBinding
{
	SMesh *pMesh;
};

struct SModel
{
	const char *pszName;
	SSkeleton *pSkeleton;
	STransform InitialPlacement;
	int32_t nMeshBindingCount;
	SModelMeshBinding *pMeshBindings;
	SVariant ExtendedData;
};

struct SPixelLayout
{
	int32_t nBytesPerPixel;
	int32_t ShiftForComponent[4];
	int32_t BitsForComponent[4];
};

struct SMipLevel
{
	int32_t nStride;
	int32_t nPixelByteCount;
	void *pPixelBytes;
};

struct STextureImage
{
	int32_t nMIPLevelCount;
	SMipLevel *pMIPLevels;
};

struct STexture
{
	const char *pszFromFileName;
	int32_t nTextureType;
	int32_t nWidth;
	int32_t nHeight;
	int32_t nEncoding;
	int32_t nSubFormat;
	SPixelLayout Layout;
	int32_t nImageCount;
	STextureImage *pImages;
	SVariant ExtendedData;
};

struct SMaterial;

struct SMaterialMap
{
	const char *pszUsage;
	SMaterial *pMaterial;
};

struct SMaterial
{
	const char *pszName;
	int32_t nMapCount;
	SMaterialMap *pMaps;
	STexture *pTexture;
	SVariant ExtendedData;
};

struct SFileInfo
{
	SArtToolInfo *pArtToolInfo;
	SExporterInfo *pExporterInfo;
	const char *pszFromFileName;
	int32_t nTextureCount;
	void **ppTextures;
	int32_t nMaterialCount;
	SMaterial **ppMaterials;
	int32_t nSkeletonCount;
	SSkeleton **ppSkeletons;
	int32_t nVertexDataCount;
	SVertexData **ppVertexDatas;
	int32_t nTriTopologyCount;
	STriTopology **ppTriTopologies;
	int32_t nMeshCount;
	SMesh **ppMeshes;
	int32_t nModelCount;
	SModel **ppModels;
	int32_t nTrackGroupCount;
	void **ppTrackGroups;
	int32_t nAnimationCount;
	void **ppAnimations;
	SVariant ExtendedData;
};

// The sizes granny211.h asserts, restated where a compiler enforces them. The
// x64 numbers were also measured against the real DLL, by reading a converted
// bone array at this stride and getting a valid parent chain out of it; the same
// read at the file's stride is noise.
#if defined( _WIN64 ) || defined( __x86_64__ ) || defined( __aarch64__ ) \
	|| ( defined( __SIZEOF_POINTER__ ) && __SIZEOF_POINTER__ == 8 )
static_assert( sizeof( SVariant ) == 16, "granny_variant" );
static_assert( sizeof( STransform ) == 68, "granny_transform" );
static_assert( sizeof( SDataTypeDefinition ) == 44, "granny_data_type_definition" );
static_assert( sizeof( SBone ) == 164, "granny_bone" );
static_assert( sizeof( SSkeleton ) == 40, "granny_skeleton" );
static_assert( sizeof( SModel ) == 112, "granny_model" );
static_assert( sizeof( SFileInfo ) == 148, "granny_file_info" );
static_assert( sizeof( SMesh ) == 76, "granny_mesh" );
static_assert( sizeof( SVertexData ) == 44, "granny_vertex_data" );
static_assert( sizeof( STriTopology ) == 132, "granny_tri_topology" );
static_assert( sizeof( SBoneBinding ) == 44, "granny_bone_binding" );
#else
static_assert( sizeof( SVariant ) == 8, "granny_variant" );
static_assert( sizeof( STransform ) == 68, "granny_transform" );
static_assert( sizeof( SDataTypeDefinition ) == 32, "granny_data_type_definition" );
static_assert( sizeof( SBone ) == 152, "granny_bone" );
static_assert( sizeof( SSkeleton ) == 24, "granny_skeleton" );
static_assert( sizeof( SModel ) == 92, "granny_model" );
static_assert( sizeof( SFileInfo ) == 92, "granny_file_info" );
static_assert( sizeof( SMesh ) == 44, "granny_mesh" );
static_assert( sizeof( SVertexData ) == 28, "granny_vertex_data" );
static_assert( sizeof( STriTopology ) == 88, "granny_tri_topology" );
static_assert( sizeof( SBoneBinding ) == 36, "granny_bone_binding" );
#endif

static_assert( sizeof( STriMaterialGroup ) == 12, "granny_tri_material_group" );
static_assert( sizeof( SPixelLayout ) == 36, "granny_pixel_layout" );

}

#pragma pack( pop )
