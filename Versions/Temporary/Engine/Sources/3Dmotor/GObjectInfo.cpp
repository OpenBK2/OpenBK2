#include "stdafx.h"
#include "System/BasicShare.h"
#include "GObjectInfo.h"
#include "GltfFormat.h"

#include <boost/uuid/uuid_io.hpp>

#include "vendor/granny/include/granny.h"
#include "GSkeleton.h"
#include "System/BinaryResources.h"
#include "System/GResource.h"
#include "System/VFSOperations.h"
#include "DBScene.h"

#include <fastgltf/tools.hpp>


//#include "../Misc/HPTimer.h" // test for perfomance


inline bool operator==( const SPlane &a, const SPlane &b ) { return a.n == b.n && a.d == b.d; }

namespace NGScene
{
int SPartAndSkeletonKeyHash::operator()( const SPartAndSkeletonKey &k ) const
{
	return ( k.pGeometry?k.pGeometry->GetDBID().GetHashKey() : 0 ) ^ k.nGeometryPart ^ 
		( k.pSkeleton ? k.pSkeleton->GetDBID().GetHashKey() : 0 ) ^ k.nLightMapped;
}

CGrannyFile::~CGrannyFile()
{
	if ( pFile ) 
		GrannyFreeFile( pFile ); 
	pFile = 0;
}

static CBasicShare<SResKey<SGrannyFileLoaderInfo>, CGrannyMemFileLoader, SGrannyFileLoaderInfoHash> shareGrannyFiles(102);

void CGrannyMemFileLoader::RecalcValue( CFileRequest *p )
{
	CFileRequest &req = *p;
#pragma warning( disable: 4530 )
	try
	{
		if ( pValue == 0 ) 
			pValue = new CGrannyFile;
		granny_int32 nBufferSize = req.GetStream()->GetSize();
		CMemoryStream memoStream;
		req.GetStream()->ReadTo( &memoStream, nBufferSize );

		//NHPTimer::STime tStart;
		//NHPTimer::GetTime( &tStart );
		granny_file *pFile = GrannyReadEntireFileFromMemory( nBufferSize, (void*)( memoStream.GetBuffer() ) );
		//const float fPassedCompress = NHPTimer::GetTimePassed( &tStart );
		//static float fReadGrannyTime = 0.0f;
		//static int nReadGrannyCount = 0;
		//fReadGrannyTime += fPassedCompress * 1000.0f;
		//++nReadGrannyCount;
		//DebugTrace( StrFmt( "Granny read src1: %.2f for %s with ID=%d (Total=%d, AvgT=%.2f)",
		//	fReadGrannyTime, GetKey().szResName.c_str(), GetKey().nID, nReadGrannyCount, fReadGrannyTime / nReadGrannyCount ) );

		pValue->pFile = pFile;
	}
	catch ( ... ) 
	{
	}
#pragma warning( default: 4530 )
}

void CGrannyMeshLoader::SetKey( const SPartAndSkeletonKey &_key )
{
	key = _key;
	pGrannyFile = 0;
	pSkeletonFileInfo = 0;
	if ( _key.pGeometry && !_key.pGeometry->szModelFileRef.empty() )
	{
		// A GLTF geometry may still bind to a legacy GR2 skeleton by bone name.
		if ( key.pSkeleton && key.pSkeleton->szModelFileRef.empty() )
			pSkeletonFileInfo = NAnimation::GetSkeletonFileInfo( key.pSkeleton );
		return;
	}
	SResKey<SGrannyFileLoaderInfo> uidKey( _key.pGeometry->uid, SGrannyFileLoaderInfo( "Geometries", _key.pGeometry->GetRecordID(), false ) );
	pGrannyFile = shareGrannyFiles.Get(  uidKey );
	if ( key.pSkeleton )
		pSkeletonFileInfo = NAnimation::GetSkeletonFileInfo( key.pSkeleton );
}

bool CGrannyMeshLoader::NeedUpdate()
{
	const bool bParentChanged = TParent::NeedUpdate();
	if ( key.pGeometry && !key.pGeometry->szModelFileRef.empty() )
	{
		if ( pSkeletonFileInfo )
			return bParentChanged | pSkeletonFileInfo.Refresh();
		return bParentChanged || pValue == 0;
	}
	if ( pSkeletonFileInfo )
		return pGrannyFile.Refresh() | pSkeletonFileInfo.Refresh();
	return pGrannyFile.Refresh();
}

static int CalcGrannyTypedefOffset( granny_data_type_definition *pType, const char *name )
{
	int nRet = 0;
	while ( pType && pType->Type != GrannyEndMember )
	{
		if ( strcmp( name, pType->Name ) == 0 )
			return nRet;
		nRet += GrannyGetMemberTypeSize( pType );
		++pType;
	}
	return -1;
}

static int CalcGrannyMemberArraySize( granny_data_type_definition *pType, const char *name )
{
	while ( pType && pType->Type != GrannyEndMember )
	{
		if ( strcmp( name, pType->Name ) == 0 )
			return GrannyGetMemberTypeSize( pType );
		++pType;
	}
	return 0;
}

void ConvertAIGeomVerticesFromGranny( granny_mesh *pMesh, std::vector<CVec3> *pRes )
{
	int nSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
	int nPosOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexPositionName );
	if ( nPosOffset < 0 )
	{
		ASSERT(0);
		return;
	}
	pRes->resize( pMesh->PrimaryVertexData->VertexCount );
	char *pUntypedVertices = (char*)( pMesh->PrimaryVertexData->Vertices );

	for ( int k = 0; k < pMesh->PrimaryVertexData->VertexCount; ++k )
	{
		char *pVertex = pUntypedVertices + k * nSize;
		CVec3 &dst = (*pRes)[k];
		memcpy( &dst, pVertex + nPosOffset, 3 * sizeof(float) );
	}
}

void ConvertAIGeomTrisFromGranny( granny_mesh *pMesh, std::vector<STriangle> *pRes )
{
	granny_tri_topology *pTopol = pMesh->PrimaryTopology;
	// ASSERT( pTopol->GroupCount == 1 );
	// int nTriCount = pTopol->Groups->TriCount;
	int nTriCount = 0;
	for ( int i = 0; i < pTopol->GroupCount; ++i )
		nTriCount += pTopol->Groups[i].TriCount;
	pRes->resize( nTriCount );
	if ( nTriCount == 0 )
		return;
	int ind = 0;
	for ( int i = 0; i < nTriCount; ++i )
	{
		(*pRes)[i].i1 = pTopol->Indices[ ind ];
		(*pRes)[i].i2 = pTopol->Indices[ ind + 1 ];
		(*pRes)[i].i3 = pTopol->Indices[ ind + 2 ];
		ind += 3;
	}
}

void ConvertVerticesFromGranny( granny_mesh *pMesh, int nMaterialIndex, std::vector<NGScene::SVertex> *pVerts )
{
	int nSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
	int nPosOffset, nNormalOffset, nTexUOffset, nTexVOffset, nTexOffset; 

	nPosOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexPositionName );
	if ( nPosOffset < 0 )
	{
		ASSERT(0);
		return;
	}

	nNormalOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexNormalName );
	nTexUOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexTangentName );
	nTexVOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexBinormalName );
	nTexOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexTextureCoordinatesName );
	if ( nTexOffset < 0 )
		nTexOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexTextureCoordinatesName "0" );

	if ( nTexUOffset < 0 || nTexVOffset < 0 || nTexOffset < 0 || nNormalOffset < 0 )
		ASSERT(0);
/**
	// Commented this out to still copy the whole set of vertices
	// relying on further optimization phase which must optimize redundant ones out.
	//
	granny_tri_topology *pTopology = pMesh->PrimaryTopology;
	granny_tri_material_group &triMaterialGroup = pTopology->Groups[nMaterialIndex];

	int nIndexFirst = 3 * triMaterialGroup.TriFirst;
	int nIndexCount = 3 * triMaterialGroup.TriCount;
	granny_int32 *pVertIndices = pTopology->Indices + nIndexFirst;
	char *pUntypedVertices = (char*)(pMesh->PrimaryVertexData->Vertices);
	pVerts->resize( nIndexCount );

	for ( int k = 0; k < nIndexCount; ++k )
	{
		char *pVertex = pUntypedVertices + nSize * pVertIndices[k];
		NGScene::SVertex &dst = (*pVerts)[k];

		memcpy( &dst.pos, pVertex + nPosOffset, 3 * sizeof(float) );
		if ( nPosOffset >= 0 )
			memcpy( &dst.tex, pVertex + nTexOffset, 2 * sizeof(float) );
		CVec3 vLoad;
		if ( nNormalOffset >= 0 )
		{
			memcpy( &vLoad, pVertex + nNormalOffset, 3 * sizeof(float) );
			NGfx::CalcCompactVector( &dst.normal, vLoad );
			dst.normal.w = 255;
		}
		if ( nTexUOffset >= 0 )
		{
			memcpy( &vLoad, pVertex + nTexUOffset, 3 * sizeof(float) );
			NGfx::CalcCompactVector( &dst.texU, vLoad );
		}
		if ( nTexVOffset >= 0 )
		{
			memcpy( &vLoad, pVertex + nTexVOffset, 3 * sizeof(float) );
			NGfx::CalcCompactVector( &dst.texV, vLoad );
		}
	}
/**/

	pVerts->resize( pMesh->PrimaryVertexData->VertexCount );
	char *pUntypedVertices = (char*)(pMesh->PrimaryVertexData->Vertices);

	for ( int k = 0; k < pMesh->PrimaryVertexData->VertexCount; ++k )
	{
		char *pVertex = pUntypedVertices + k * nSize;
		NGScene::SVertex &dst = (*pVerts)[k];

		memcpy( &dst.pos, pVertex + nPosOffset, 3 * sizeof(float) );
		if ( nPosOffset >= 0 )
			memcpy( &dst.tex, pVertex + nTexOffset, 2 * sizeof(float) );
		CVec3 vLoad;
		if ( nNormalOffset >= 0 )
		{
			memcpy( &vLoad, pVertex + nNormalOffset, 3 * sizeof(float) );
			NGfx::CalcCompactVector( &dst.normal, vLoad );
			dst.normal.w = 255;
		}
		if ( nTexUOffset >= 0 )
		{
			memcpy( &vLoad, pVertex + nTexUOffset, 3 * sizeof(float) );
			NGfx::CalcCompactVector( &dst.texU, vLoad );
		}
		if ( nTexVOffset >= 0 )
		{
			memcpy( &vLoad, pVertex + nTexVOffset, 3 * sizeof(float) );
			NGfx::CalcCompactVector( &dst.texV, vLoad );
		}
	}
}

const char *ConvertWeightsFromGrannyEx(
							  const granny_skeleton *pSkeleton, granny_mesh *pMesh, int nMaterialIndex,
							  std::vector<SVertexWeight> *pWeights, int nVertices )
{

	const char *res=0;


	if ( pMesh->BoneBindingCount <= 0 ) 
		return "Error";

	pWeights->resize( nVertices );

	if ( GrannyMeshIsRigid( pMesh ) )
	{
		const char *pszBoneName = pMesh->BoneBindings[0].BoneName;
		int nBone;
		if ( GrannyFindBoneByName( pSkeleton, pszBoneName, &nBone ) )
		{
			const float fDefault[4] = { 1, 0, 0, 0 };
			const char cDefault[4] = { nBone, 0, 0, 0 };
			for ( int i = 0; i < nVertices; ++i ) 
			{
				memcpy( (*pWeights)[i].fWeights, fDefault, 4 *sizeof(float) );
				memcpy( (*pWeights)[i].cBoneIndices, cDefault, 4 );
			}
		}
		else
		{
			if ( pSkeleton && pSkeleton->Name && pszBoneName )
			{
				NI_ASSERT( false, fmt::format( "Vertex was binded to non-present bone. Skeleton = \"{}\" Bone = \"{}\"/n", pSkeleton->Name, pszBoneName ) );
			}
			else
				ASSERT( 0 && "Vertex was binded to non-present bone" );
			return "Vertex was binded to non-present bone/n";
		}
	}
	else
	{
		int nWeightsOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexBoneWeightsName );
		int nIndicesOffset = CalcGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexBoneIndicesName );
		if ( nWeightsOffset < 0 || nIndicesOffset < 0 )
		{
			ASSERT(0);
			return "Error";
		}
		int nWeightsCount = CalcGrannyMemberArraySize( pMesh->PrimaryVertexData->VertexType, GrannyVertexBoneWeightsName );
		int nIndicesCount = CalcGrannyMemberArraySize( pMesh->PrimaryVertexData->VertexType, GrannyVertexBoneIndicesName );
		ASSERT( nWeightsCount == nIndicesCount );
		ASSERT( nIndicesCount <= 4 && nWeightsCount <= 4 && "Unsupported number of vertice-to-bone bindings in mesh!" );
		ASSERT( nIndicesCount > 0 && nWeightsCount > 0 && "Unsupported number of vertice-to-bone bindings in mesh!" );
		if ( nWeightsCount == 0 || nIndicesCount == 0 || nIndicesCount != nWeightsCount )
			return "Error";
		nWeightsCount = (std::min)( 4, nWeightsCount );
		nIndicesCount = (std::min)( 4, nIndicesCount );

		int nSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
		char *pUntypedVertices = (char*)(pMesh->PrimaryVertexData->Vertices);

		std::vector<int> index2bone;
		index2bone.resize( pMesh->BoneBindingCount );
		for ( int k = 0; k < pMesh->BoneBindingCount; ++k )
		{
			int nBone = 0;
			if ( !GrannyFindBoneByName( pSkeleton, pMesh->BoneBindings[k].BoneName, &nBone ) )
			{
				//ASSERT( 0 && "Bone binding error, was binded to non-present bone" );
				res =  "Bone binding error, was binded to non-present bone";
			}
			index2bone[ k ] = nBone;
		}
		/**
		// Commented this out to still copy the whole set of weights
		// relying on further optimization phase which must optimize redundant ones out.
		//
		//
		granny_tri_topology *pTopology = pMesh->PrimaryTopology;
		granny_tri_material_group &triMaterialGroup = pTopology->Groups[nMaterialIndex];

		const int nIndexFirst = 3 * triMaterialGroup.TriFirst;
		//const int nIndexCount = 3 * triMaterialGroup.TriCount;
		granny_int32 *pVertIndices = pTopology->Indices + nIndexFirst;

		for ( int k = 0; k < nVertices; ++k )
		{
		char *pVertex = pUntypedVertices + nSize * pVertIndices[k];

		granny_uint8 weights[4];
		granny_uint8 indices[4];
		memcpy( weights, pVertex + nWeightsOffset, nWeightsCount * sizeof(granny_uint8) );
		memcpy( indices, pVertex + nIndicesOffset, nIndicesCount * sizeof(granny_uint8) );
		SVertexWeight &wData = (*pWeights)[ pVertIndices[k] ];
		wData.cBoneIndices[0] = index2bone[ indices[0] ];
		wData.cBoneIndices[1] = index2bone[ indices[1] ];
		wData.cBoneIndices[2] = index2bone[ indices[2] ];
		wData.cBoneIndices[3] = index2bone[ indices[3] ];
		wData.fWeights[0] = weights[0] / 255.0f;
		wData.fWeights[1] = nWeightsCount > 0 ? weights[1] / 255.0f : 0.0f;
		wData.fWeights[2] = nWeightsCount > 1 ? weights[2] / 255.0f : 0.0f;
		wData.fWeights[3] = nWeightsCount > 2 ? weights[3] / 255.0f : 0.0f;
		}
		/**/
		/**/
		for ( int k = 0; k < pMesh->PrimaryVertexData->VertexCount; ++k )
		{
			char *pVertex = pUntypedVertices + k * nSize;
			granny_uint8 weights[4];
			granny_uint8 indices[4];
			memcpy( weights, pVertex + nWeightsOffset, nWeightsCount * sizeof(granny_uint8) );
			memcpy( indices, pVertex + nIndicesOffset, nIndicesCount * sizeof(granny_uint8) );
			SVertexWeight &wData = (*pWeights)[k];
			wData.cBoneIndices[0] = index2bone[ indices[0] ];
			wData.cBoneIndices[1] = index2bone[ indices[1] ];
			wData.cBoneIndices[2] = index2bone[ indices[2] ];
			wData.cBoneIndices[3] = index2bone[ indices[3] ];
			wData.fWeights[0] = weights[0] / 255.0f;
			wData.fWeights[1] = nWeightsCount > 0 ? weights[1] / 255.0f : 0.0f;
			wData.fWeights[2] = nWeightsCount > 1 ? weights[2] / 255.0f : 0.0f;
			wData.fWeights[3] = nWeightsCount > 2 ? weights[3] / 255.0f : 0.0f;
		}
		/**/
	}

	return res;
}


void ConvertGeometryFromGranny( granny_mesh *pMesh, int nMaterialIndex, std::vector<STriangle> *pGeometry )
{
	granny_tri_topology *pTopology = pMesh->PrimaryTopology;

	if ( nMaterialIndex != (-1) )
	{
		//
		granny_tri_material_group &triMaterialGroup = pTopology->Groups[nMaterialIndex];
		ASSERT( triMaterialGroup.MaterialIndex == nMaterialIndex );
		int nTriCount = triMaterialGroup.TriCount;
		pGeometry->resize( nTriCount );
		granny_int32 *pTopologyVertIndices = pTopology->Indices + 3 * triMaterialGroup.TriFirst;
		int ind = 0;
		for ( int i = 0; i < nTriCount; ++i )
		{
			STriangle &t = (*pGeometry)[i];
			t.i1 = pTopologyVertIndices[ ind++ ];
			t.i2 = pTopologyVertIndices[ ind++ ];
			t.i3 = pTopologyVertIndices[ ind++ ];
		}
	}
	else
	{
	//
/**/
		// // crap, I hope not for every model it is valid (or Granny exporter is really dumb one!)
		// ASSERT( pTopol->GroupCount == 1 );
		// int nTriCount = pTopol->Groups->TriCount;
		int nTriCount = 0;
		for ( int i = 0; i < pTopology->GroupCount; ++i )
			nTriCount += pTopology->Groups[i].TriCount;
		pGeometry->resize( nTriCount );
		int ind = 0;
		for ( int i = 0; i < nTriCount; ++i )
		{
			STriangle &t = (*pGeometry)[i];
			t.i1 = pTopology->Indices[ ind++ ];
			t.i2 = pTopology->Indices[ ind++ ];
			t.i3 = pTopology->Indices[ ind++ ];
		}
/**/
	}
}

granny_model *FindFirstAppropriateModel( granny_file_info *pData, granny_mesh *pMesh )
{
	for ( int nM = 0; nM < pData->ModelCount; ++nM )
	{
		granny_model *pM = pData->Models[ nM ];
		for ( int nMB = 0; nMB < pM->MeshBindingCount; ++nMB )
		{
			if ( pM->MeshBindings[ nMB ].Mesh == pMesh )
				return pM;
		}
	}
	return 0;
}

bool EndsWith( const char *pszA, const char *pszB )
{
	int nA = strlen( pszA ), nB = strlen( pszB );
	if ( nA < nB )
		return false;
	return strcmp( pszA + nA - nB, pszB ) == 0;
}

static NGScene::ELoadMode cMode = E_CACHED_LIGHTMAPS;

namespace
{
template <class TValue>
std::vector<TValue> ReadGltfAccessor( const fastgltf::Asset &asset, std::size_t accessorIndex )
{
	std::vector<TValue> result;
	if ( accessorIndex >= asset.accessors.size() )
		return result;
	const fastgltf::Accessor &accessor = asset.accessors[accessorIndex];
	result.reserve( accessor.count );
	for ( const TValue &value : fastgltf::iterateAccessor<TValue>(asset, accessor) )
		result.push_back( value );
	return result;
}

std::size_t FindAttribute( const fastgltf::Primitive &primitive, const char *pszName )
{
	const auto it = primitive.findAttribute( pszName );
	return it == primitive.attributes.end() ? static_cast<std::size_t>(-1) : it->accessorIndex;
}

void MakeFallbackBasis( const CVec3 &normal, CVec3 *pTangent, CVec3 *pBinormal )
{
	const CVec3 helper = fabsf(normal.z) < 0.9f ? CVec3(0, 0, 1) : CVec3(0, 1, 0);
	*pTangent = helper ^ normal;
	if ( !Normalize(pTangent) )
		*pTangent = CVec3(1, 0, 0);
	*pBinormal = normal ^ *pTangent;
	Normalize( pBinormal );
}

bool GetTargetBoneMap( const SPartAndSkeletonKey &key,
	NAnimation::CGrannyFileInfo *pGrannySkeletonFile,
	const NGltf::TGltfFilePtr &geometryFile, std::size_t sourceSkin,
	std::unordered_map<std::string, int> *pResult )
{
	pResult->clear();
	if ( key.pSkeleton && !key.pSkeleton->szModelFileRef.empty() )
	{
		const NGltf::TGltfFilePtr file = NGltf::LoadFile( key.pSkeleton, key.pSkeleton->szModelFileRef );
		NGltf::SSkeletonDefinition skeleton;
		if ( !NGltf::BuildSkeleton(file, key.pSkeleton->szRootJoint,
			key.nSkeletonPart, &skeleton) )
			return false;
		*pResult = skeleton.boneByName;
		return true;
	}
	if ( key.pSkeleton && pGrannySkeletonFile )
	{
		const granny_skeleton *pSkeleton =
			NAnimation::GetSkeleton( pGrannySkeletonFile, key.nSkeletonPart );
		if ( !pSkeleton || pSkeleton->BoneCount > 256 )
			return false;
		for ( int i = 0; i < pSkeleton->BoneCount; ++i )
			(*pResult)[pSkeleton->Bones[i].Name] = i;
		return true;
	}
	NGltf::SSkeletonDefinition skeleton;
	if ( NGltf::BuildSkeleton(geometryFile, static_cast<int>(sourceSkin), &skeleton) )
	{
		*pResult = skeleton.boneByName;
		return true;
	}
	return false;
}

void AppendGltfPrimitive( const NGltf::TGltfFilePtr &file, std::size_t nodeIndex,
	const fastgltf::Primitive &primitive, const SPartAndSkeletonKey &key,
	NAnimation::CGrannyFileInfo *pGrannySkeletonFile,
	CObjectInfo::SData *pData )
{
	if ( primitive.type != fastgltf::PrimitiveType::Triangles &&
		primitive.type != fastgltf::PrimitiveType::TriangleStrip &&
		primitive.type != fastgltf::PrimitiveType::TriangleFan )
		return;
	const std::size_t positionAccessor = FindAttribute( primitive, "POSITION" );
	if ( positionAccessor == static_cast<std::size_t>(-1) )
		return;

	const fastgltf::Asset &asset = file->asset;
	const fastgltf::Node &node = asset.nodes[nodeIndex];
	const std::vector<fastgltf::math::fvec3> positions =
		ReadGltfAccessor<fastgltf::math::fvec3>( asset, positionAccessor );
	const std::size_t normalAccessor = FindAttribute( primitive, "NORMAL" );
	const std::size_t tangentAccessor = FindAttribute( primitive, "TANGENT" );
	const std::size_t texAccessor = FindAttribute( primitive, "TEXCOORD_0" );
	const std::vector<fastgltf::math::fvec3> normals =
		normalAccessor == static_cast<std::size_t>(-1)
			? std::vector<fastgltf::math::fvec3>()
			: ReadGltfAccessor<fastgltf::math::fvec3>( asset, normalAccessor );
	const std::vector<fastgltf::math::fvec4> tangents =
		tangentAccessor == static_cast<std::size_t>(-1)
			? std::vector<fastgltf::math::fvec4>()
			: ReadGltfAccessor<fastgltf::math::fvec4>( asset, tangentAccessor );
	const std::vector<fastgltf::math::fvec2> texcoords =
		texAccessor == static_cast<std::size_t>(-1)
			? std::vector<fastgltf::math::fvec2>()
			: ReadGltfAccessor<fastgltf::math::fvec2>( asset, texAccessor );

	const std::size_t vertexOffset = pData->verts.size();
	pData->verts.resize( vertexOffset + positions.size() );
	const bool bSkinned = node.skinIndex.has_value();
	for ( std::size_t i = 0; i < positions.size(); ++i )
	{
		SVertex &vertex = pData->verts[vertexOffset + i];
		vertex.pos = NGltf::ConvertPosition( positions[i] );
		CVec3 normal = i < normals.size() ? NGltf::ConvertDirection(normals[i]) : CVec3(0, 0, 1);
		Normalize( &normal );
		CVec3 tangent;
		CVec3 binormal;
		if ( i < tangents.size() )
		{
			tangent = NGltf::ConvertDirection(
				fastgltf::math::fvec3(tangents[i][0], tangents[i][1], tangents[i][2]) );
			Normalize( &tangent );
			binormal = (normal ^ tangent) * -tangents[i][3];
			Normalize( &binormal );
		}
		else
			MakeFallbackBasis( normal, &tangent, &binormal );

		if ( !bSkinned )
		{
			const SHMatrix &world = file->nodeWorldTransforms[nodeIndex];
			CVec3 transformed;
			world.RotateHVector( &transformed, vertex.pos );
			vertex.pos = transformed;
			world.RotateVector( &transformed, normal );
			normal = transformed;
			Normalize( &normal );
			world.RotateVector( &transformed, tangent );
			tangent = transformed;
			Normalize( &tangent );
			world.RotateVector( &transformed, binormal );
			binormal = transformed;
			Normalize( &binormal );
		}
		NGfx::CalcCompactVector( &vertex.normal, normal );
		vertex.normal.w = 255;
		NGfx::CalcCompactVector( &vertex.texU, tangent );
		NGfx::CalcCompactVector( &vertex.texV, binormal );
		vertex.tex = i < texcoords.size()
			? CVec2(texcoords[i][0], texcoords[i][1]) : CVec2(0, 0);
	}

	std::vector<std::uint32_t> indices;
	if ( primitive.indicesAccessor.has_value() )
		indices = ReadGltfAccessor<std::uint32_t>( asset, *primitive.indicesAccessor );
	if ( primitive.type == fastgltf::PrimitiveType::Triangles )
	{
		for ( std::size_t i = 0; i + 2 < indices.size(); i += 3 )
		{
			STriangle &triangle = pData->geometry.emplace_back();
			// Coordinate conversion changes handedness, so reverse winding.
			triangle.i1 = static_cast<int>(vertexOffset + indices[i]);
			triangle.i2 = static_cast<int>(vertexOffset + indices[i + 2]);
			triangle.i3 = static_cast<int>(vertexOffset + indices[i + 1]);
		}
	}
	else if ( primitive.type == fastgltf::PrimitiveType::TriangleStrip )
	{
		for ( std::size_t i = 2; i < indices.size(); ++i )
		{
			const std::uint32_t a = indices[i - 2];
			const std::uint32_t b = indices[i - 1];
			const std::uint32_t c = indices[i];
			if ( a == b || b == c || a == c )
				continue;
			STriangle &triangle = pData->geometry.emplace_back();
			// A strip alternates its source winding. Reverse each source triangle
			// explicitly because the coordinate conversion changes handedness.
			triangle.i1 = static_cast<int>(vertexOffset + (i & 1 ? b : a));
			triangle.i2 = static_cast<int>(vertexOffset + c);
			triangle.i3 = static_cast<int>(vertexOffset + (i & 1 ? a : b));
		}
	}
	else
	{
		for ( std::size_t i = 2; i < indices.size(); ++i )
		{
			STriangle &triangle = pData->geometry.emplace_back();
			triangle.i1 = static_cast<int>(vertexOffset + indices[0]);
			triangle.i2 = static_cast<int>(vertexOffset + indices[i]);
			triangle.i3 = static_cast<int>(vertexOffset + indices[i - 1]);
		}
	}

	const std::size_t jointsAccessor = FindAttribute( primitive, "JOINTS_0" );
	const std::size_t weightsAccessor = FindAttribute( primitive, "WEIGHTS_0" );
	if ( bSkinned || !pData->weights.empty() )
	{
		// Keep the arrays parallel even for a partially exported primitive. Bone
		// zero is the safe fallback until valid source influences are rebound.
		const std::size_t oldSize = pData->weights.size();
		pData->weights.resize( pData->verts.size() );
		for ( std::size_t i = oldSize; i < pData->weights.size(); ++i )
		{
			memset( &pData->weights[i], 0, sizeof(pData->weights[i]) );
			pData->weights[i].fWeights[0] = 1.0f;
		}
	}
	if ( !node.skinIndex.has_value() ||
		jointsAccessor == static_cast<std::size_t>(-1) ||
		weightsAccessor == static_cast<std::size_t>(-1) ||
		*node.skinIndex >= asset.skins.size() )
		return;

	const fastgltf::Skin &sourceSkin = asset.skins[*node.skinIndex];
	std::unordered_map<std::string, int> targetBones;
	if ( !GetTargetBoneMap(key, pGrannySkeletonFile, file, *node.skinIndex, &targetBones) )
		return;
	const std::vector<fastgltf::math::uvec4> joints =
		ReadGltfAccessor<fastgltf::math::uvec4>( asset, jointsAccessor );
	const std::vector<fastgltf::math::fvec4> weights =
		ReadGltfAccessor<fastgltf::math::fvec4>( asset, weightsAccessor );
	for ( std::size_t i = 0; i < positions.size(); ++i )
	{
		SVertexWeight &target = pData->weights[vertexOffset + i];
		memset( &target, 0, sizeof(target) );
		if ( i >= joints.size() || i >= weights.size() )
		{
			target.fWeights[0] = 1.0f;
			continue;
		}
		float totalWeight = 0.0f;
		int targetInfluence = 0;
		for ( int influence = 0; influence < 4 && targetInfluence < 4; ++influence )
		{
			if ( weights[i][influence] <= 0.0f )
				continue;
			const std::size_t joint = joints[i][influence];
			if ( joint >= sourceSkin.joints.size() )
				continue;
			const std::size_t sourceNodeIndex = sourceSkin.joints[joint];
			if ( sourceNodeIndex >= asset.nodes.size() )
				continue;
			const fastgltf::Node &sourceNode = asset.nodes[sourceNodeIndex];
			const std::string name = sourceNode.name.empty()
				? "Node_" + std::to_string(sourceNodeIndex) : std::string(sourceNode.name);
			const auto found = targetBones.find( name );
			if ( found == targetBones.end() || found->second < 0 || found->second > 255 )
				continue;
			target.cBoneIndices[targetInfluence] = static_cast<uint8_t>(found->second);
			target.fWeights[targetInfluence] = weights[i][influence];
			totalWeight += target.fWeights[targetInfluence++];
		}
		if ( totalWeight > FP_EPSILON )
			for ( int influence = 0; influence < targetInfluence; ++influence )
				target.fWeights[influence] /= totalWeight;
		else
			target.fWeights[0] = 1.0f;
	}
}

bool LoadGltfMesh( const SPartAndSkeletonKey &key,
	NAnimation::CGrannyFileInfo *pGrannySkeletonFile,
	CObjectInfo::SData *pResult )
{
	const NGltf::TGltfFilePtr file = NGltf::LoadFile( key.pGeometry, key.pGeometry->szModelFileRef );
	std::vector<std::size_t> meshNodes;
	if ( !NGltf::GetMeshNodes(file, key.pGeometry->szRootMesh, &meshNodes) ||
		key.nGeometryPart < 0 || key.nGeometryPart >= static_cast<int>(meshNodes.size()) )
		return false;
	const std::size_t nodeIndex = meshNodes[key.nGeometryPart];
	const fastgltf::Node &node = file->asset.nodes[nodeIndex];
	if ( !node.meshIndex.has_value() || *node.meshIndex >= file->asset.meshes.size() )
		return false;
	const fastgltf::Mesh &mesh = file->asset.meshes[*node.meshIndex];
	if ( key.nMaterialPart >= 0 )
	{
		if ( key.nMaterialPart >= static_cast<int>(mesh.primitives.size()) )
			return false;
		AppendGltfPrimitive( file, nodeIndex, mesh.primitives[key.nMaterialPart],
			key, pGrannySkeletonFile, pResult );
	}
	else
	{
		for ( const fastgltf::Primitive &primitive : mesh.primitives )
			AppendGltfPrimitive( file, nodeIndex, primitive, key, pGrannySkeletonFile, pResult );
	}
	return !pResult->verts.empty() && !pResult->geometry.empty();
}
}

void SetLoadMode( NGScene::ELoadMode eMode )
{
	cMode = eMode;
}

void CGrannyMeshLoader::Recalc()
{	
	if ( key.pGeometry && !key.pGeometry->szModelFileRef.empty() )
	{
		if ( pSkeletonFileInfo )
			pSkeletonFileInfo.Refresh();
		CObjectInfo::SData data;
		if ( !LoadGltfMesh(key, pSkeletonFileInfo ? pSkeletonFileInfo->GetValue() : 0, &data) )
		{
			pValue = 0;
			return;
		}
		if ( !pValue )
			pValue = new CObjectInfo;
		pValue->Assign( &data, true );
		pValue->SetLightmappable( false );
		bLightMapped = false;
		sLightMapped.clear();
		return;
	}
	if( cMode == E_CACHED_LIGHTMAPS )
	{
		bool bNewWay = false;
	    bool bLightMap = false;
		std::string buff;

		bLightMap = key.nLightMapped != 0;

		if( !key.pGeometry->uid.is_nil() )
		{
			buff = fmt::format("bin\\Geometries\\{}_{}_{}_{}_{}_{}",
				boost::uuids::to_string(key.pGeometry->uid ),
				key.pGeometry->GetRecordID(), 
				key.nGeometryPart, 
				key.nMaterialPart,
				key.pSkeleton->GetRecordID(), 
				key.nSkeletonPart
				);
		}

		std::string szFileName = buff;
		sLightMapped = szFileName+"_l";


		const std::string szTryFileName( bLightMap ? sLightMapped : szFileName );


		const bool bIsTryFileExist = NVFS::GetMainVFS()->DoesFileExist( szTryFileName );
		CFileStream stream( NVFS::GetMainVFS(), bIsTryFileExist ? szTryFileName : szFileName );
		bLightMap = bLightMap && bIsTryFileExist;

		bNewWay = stream.IsOk();

		if (  stream.IsOk() )
		{
			if ( pValue == 0 )
				pValue = new CObjectInfo;

			NGScene::CObjectInfo::SBinData sBinD;
			CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
			sBinD & *pSaver;

		
			pValue->AssignDestructive( &sBinD );
			bLightMapped = bLightMap;
			return;
		
		}

	}


	if ( ( pGrannyFile->GetValue() == 0 ) || ( pSkeletonFileInfo && pSkeletonFileInfo->GetValue() == 0 ) )
	{
		pValue = 0;
		return;
	}
#pragma warning( disable: 4530 )
	try
	{
		granny_file *pFile = pGrannyFile->GetValue()->pFile;

		if ( pValue == 0 ) 
			pValue = new CObjectInfo;

		granny_file_info *pData = GrannyGetFileInfo( pFile );
		granny_model *pModel = pData->Models[0];
		bool bIsLightmappable = !EndsWith( pModel->Name, "noLM" );

		if ( key.nGeometryPart >= 0 && key.nGeometryPart < pModel->MeshBindingCount ) 
		{
			granny_mesh *pMesh = pModel->MeshBindings[key.nGeometryPart].Mesh;
			granny_int32 nGroupCount = GrannyGetMeshTriangleGroupCount( pMesh );
			if ( key.nMaterialPart < nGroupCount )
			{
				NGScene::CObjectInfo::SData objData;
				ConvertVerticesFromGranny( pMesh, key.nMaterialPart, &objData.verts );
				ConvertGeometryFromGranny( pMesh, key.nMaterialPart, &objData.geometry );

				const granny_skeleton *pSkeleton = 0;
				if ( key.pSkeleton )
					pSkeleton = NAnimation::GetSkeleton( pSkeletonFileInfo->GetValue(), key.nSkeletonPart );
				else
					pSkeleton = FindFirstAppropriateModel( pData, pMesh )->Skeleton;

				ConvertWeightsFromGrannyEx( pSkeleton, pMesh, key.nMaterialPart, &objData.weights, objData.verts.size() );
				pValue->Assign( &objData, true );
				pValue->SetLightmappable( bIsLightmappable );

				bLightMapped = false;

			}
		}
	}
	catch ( ... ) 
	{
	}
#pragma warning( default: 4530 )
}

} // namespace NGScene

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x13173BC1, CGrannyMemFileLoader )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x30174280, CGrannyMeshLoader )

