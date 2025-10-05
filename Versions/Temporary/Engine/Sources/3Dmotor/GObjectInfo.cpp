#include "stdafx.h"
#include "System/BasicShare.h"
#include "GObjectInfo.h"
#include "vendor/granny/include/granny.h"
#include "GSkeleton.h"
#include "System/BinaryResources.h"
#include "System/GResource.h"
#include "System/VFSOperations.h"
#include "DBScene.h"


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
	SResKey<SGrannyFileLoaderInfo> uidKey( _key.pGeometry->uid, SGrannyFileLoaderInfo( "Geometries", _key.pGeometry->GetRecordID(), false ) );
	pGrannyFile = shareGrannyFiles.Get(  uidKey );
	if ( key.pSkeleton )
		pSkeletonFileInfo = NAnimation::GetSkeletonFileInfo( key.pSkeleton );
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
				NI_ASSERT( false, StrFmt( "Vertex was binded to non-present bone. Skeleton = \"%s\" Bone = \"%s\"/n", pSkeleton->Name, pszBoneName ) );
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
		nWeightsCount = Min( 4, nWeightsCount );
		nIndicesCount = Min( 4, nIndicesCount );

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

void SetLoadMode( NGScene::ELoadMode eMode )
{
	cMode = eMode;
}

void CGrannyMeshLoader::Recalc()
{	
	if( cMode == E_CACHED_LIGHTMAPS )
	{
		bool bNewWay = false;
	    bool bLightMap = false;
		char buff[1024];

		bLightMap = key.nLightMapped != 0;

		if( !NBinResources::IsEmptyGUID( key.pGeometry->uid ) )
		{
			sprintf( buff, "bin\\Geometries\\%s_%i_%i_%i_%i_%i", 
				NBinResources::GUIDToString( key.pGeometry->uid ).c_str(),
				key.pGeometry->GetRecordID(), 
				key.nGeometryPart, 
				key.nMaterialPart,
				key.pSkeleton->GetRecordID(), 
				key.nSkeletonPart
				);
		}
		else
		{
			buff[0] = 0;
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
REGISTER_SAVELOAD_CLASS( 0x13173BC1, CGrannyMemFileLoader )
REGISTER_SAVELOAD_CLASS( 0x30174280, CGrannyMeshLoader )

