#include "stdafx.h"

#include "Tools_Granny.h"
#include "MapEditorLib/Tools_Geometry.h"
#include "MapEditorLib/Interface_Logger.h"
#include "Misc/StrProc.h"

int CalculateGrannyTypedefOffset( granny_data_type_definition *pType, const char *pName )
{
	int nOffset = 0;
	while ( pType && ( pType->Type != GrannyEndMember ) )
	{
		if ( strcmp( pName, pType->Name ) == 0 )
		{
			return nOffset;
		}
		nOffset += GrannyGetMemberTypeSize( pType );
		++pType;
	}
	return INVALID_GRANNY_TYPEDEF_OFFSET;
}


int CalculateGrannyMemberArraySize( granny_data_type_definition *pType, const char *pName )
{
	while ( pType && ( pType->Type != GrannyEndMember ) )
	{
		if ( strcmp( pName, pType->Name ) == 0 )
		{
			return GrannyGetMemberTypeSize( pType );
		}
		++pType;
	}
	return 0;
}

bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo )
{
	NI_ASSERT( pvMin != 0, "GetGrannyMeshBoundidngBox(): mMin == 0" );
	NI_ASSERT( pvMax != 0, "GetGrannyMeshBoundidngBox(): pMax == 0" );
	NI_ASSERT( pInfo != 0, "GetGrannyMeshBoundidngBox(): pInfo == 0" );

	bool bResult = false;
	CVec3 vMin = VNULL3;
	CVec3 vMax = VNULL3;
	bool bFirstVertex = true;
	for ( int nMeshIndex = 0; nMeshIndex < pInfo->MeshCount; ++nMeshIndex )
	{
		granny_mesh *pMesh = pInfo->Meshes[nMeshIndex];
		const int nPositionOffset = CalculateGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexPositionName );
		const int nVertexSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
		if ( nPositionOffset != INVALID_GRANNY_TYPEDEF_OFFSET )
		{
			char *pUntypedVertices = (char*)( pMesh->PrimaryVertexData->Vertices );
			for ( int nVertexIndex = 0; nVertexIndex < pMesh->PrimaryVertexData->VertexCount; ++nVertexIndex )
			{
				char *pVertex = pUntypedVertices + nVertexIndex * nVertexSize;
				CVec3 vPosition = VNULL3;
				memcpy( &vPosition, pVertex + nPositionOffset, 3 * sizeof( float ) );
				if ( bFirstVertex )
				{
					vMin = vPosition;
					vMax = vPosition;
					bFirstVertex = false;
				}
				else
				{
					UpdateBoundingBox( &vMin, &vMax, vPosition );
				}
				bResult = true;
			}
		}
	}
	if ( bResult )
	{
		( *pvMin ) = vMin;
		( *pvMax ) = vMax;
	}
	return bResult;
}

int GetGrannyAnimationLength( granny_file_info *pInfo )
{
	if ( pInfo->AnimationCount == 0 )
		return 0;
	granny_animation *pAnimation = pInfo->Animations[0];
	return int( pAnimation->Duration * 1000.0f );
}

bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo, const string &szMeshName )
{
	NI_ASSERT( pvMin != 0, "GetGrannyMeshBoundidngBox(): mMin == 0" );
	NI_ASSERT( pvMax != 0, "GetGrannyMeshBoundidngBox(): pMax == 0" );
	NI_ASSERT( pInfo != 0, "GetGrannyMeshBoundidngBox(): pInfo == 0" );

	bool bResult = false;
	CVec3 vMin = VNULL3;
	CVec3 vMax = VNULL3;
	bool bFirstVertex = true;
	for ( int nMeshIndex = 0; nMeshIndex < pInfo->MeshCount; ++nMeshIndex )
	{
		granny_mesh *pMesh = pInfo->Meshes[nMeshIndex];
		if ( szMeshName == pMesh->Name )
		{
			const int nPositionOffset = CalculateGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexPositionName );
			const int nVertexSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
			if ( nPositionOffset != INVALID_GRANNY_TYPEDEF_OFFSET )
			{
				char *pUntypedVertices = (char*)( pMesh->PrimaryVertexData->Vertices );
				for ( int nVertexIndex = 0; nVertexIndex < pMesh->PrimaryVertexData->VertexCount; ++nVertexIndex )
				{
					char *pVertex = pUntypedVertices + nVertexIndex * nVertexSize;
					CVec3 vPosition = VNULL3;
					memcpy( &vPosition, pVertex + nPositionOffset, 3 * sizeof( float ) );
					if ( bFirstVertex )
					{
						vMin = vPosition;
						vMax = vPosition;
						bFirstVertex = false;
					}
					else
					{
						UpdateBoundingBox( &vMin, &vMax, vPosition );
					}
					bResult = true;
				}
			}
		}
	}
	if ( bResult )
	{
		( *pvMin ) = vMin;
		( *pvMax ) = vMax;
	}
	return bResult;
}

void GetVerticesFromGrannyMesh( granny_mesh *pMesh, vector<CVec3> *pVertexList )
{
	NI_ASSERT( pMesh != 0, "GetVerticesFromGrannyMesh(): pMesh == 0" );
	NI_ASSERT( pVertexList != 0, "GetVerticesFromGrannyMesh(): pVertexList == 0" );
	//
	const int nPositionOffset = CalculateGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexPositionName );
	const int nVertexSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
	if ( nPositionOffset != INVALID_GRANNY_TYPEDEF_OFFSET )
	{
		pVertexList->resize( pMesh->PrimaryVertexData->VertexCount );
		char *pUntypedVertices = (char*)( pMesh->PrimaryVertexData->Vertices );
		//
		for ( int nVertexIndex = 0; nVertexIndex < pMesh->PrimaryVertexData->VertexCount; ++nVertexIndex )
		{
			char *pVertex = pUntypedVertices + nVertexIndex * nVertexSize;
			CVec3 vPosition = VNULL3;
			memcpy( &vPosition, pVertex + nPositionOffset, 3 * sizeof( float ) );
			pVertexList->push_back( vPosition );
		}
	}
}


void GetTrianglesFromGrannyMesh( granny_mesh *pMesh, vector<STriangle> *pTriangleList )
{
	NI_ASSERT( pMesh != 0, "GetTrianglesFromGrannyMesh(): pMesh == 0" );
	NI_ASSERT( pTriangleList != 0, "GetTrianglesFromGrannyMesh(): pTriangleList == 0" );
	//
	granny_tri_topology *pTriTopology = pMesh->PrimaryTopology;
	//
	int nTriangleCount = 0;
	for ( int nGroupIndex = 0; nGroupIndex < pTriTopology->GroupCount; ++nGroupIndex )
	{
		nTriangleCount += pTriTopology->Groups[nGroupIndex].TriCount;
	}
	//
	int nGlobalIndex = 0;
	for ( int nTriangleIndex = 0; nTriangleIndex < nTriangleCount; ++nTriangleIndex )
	{
		vector<STriangle>::iterator posTriangle = pTriangleList->insert( pTriangleList->end(), STriangle() );
		posTriangle->i1 = pTriTopology->Indices[ nGlobalIndex + 0 ];
		posTriangle->i2 = pTriTopology->Indices[ nGlobalIndex + 1 ];
		posTriangle->i3 = pTriTopology->Indices[ nGlobalIndex + 2 ];
		nGlobalIndex += 3;
	}
}

bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, const string &rszFileName, const string &rszDesiredSkeletonName, bool bFromRoot )
{
	granny_file *pFile = GrannyReadEntireFile( rszFileName.c_str() );
	if ( pFile == 0 ) 
		return false;
	granny_file_info *pInfo = GrannyGetFileInfo( pFile );
	if ( (pInfo == 0) || (pInfo->SkeletonCount != 1) )
		return false;
	//
	bool bRetVal = ReadAttributes( pBoneList, pInfo, rszDesiredSkeletonName, bFromRoot );
	//
	GrannyFreeFile( pFile );
	return bRetVal;
}

bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, granny_file_info *pInfo, const string &rszDesiredSkeletonName, bool bFromRoot )
{
	NI_ASSERT( pBoneList != 0, "ReadAttributes() pBoneList == 0" );
	//
	if ( pInfo == 0 || pInfo->SkeletonCount != 1 )
		return false;
	//
	if ( !rszDesiredSkeletonName.empty() )
	{
		string szSkeletonName = pInfo->Skeletons[0]->Name;
		NStr::ToLowerASCII( &szSkeletonName );
		string szDesiredSkeletonName = rszDesiredSkeletonName;
		NStr::ToLowerASCII( &szDesiredSkeletonName );
		if ( szDesiredSkeletonName != szSkeletonName )
			return false;
	}
	const int nStartFromBone = bFromRoot ? 0 : 1;
	if ( pInfo->Skeletons[0]->BoneCount <= nStartFromBone )
		return false;
	//
	pBoneList->reserve( pBoneList->size() + pInfo->Skeletons[0]->BoneCount );
	for ( int i = nStartFromBone; i < pInfo->Skeletons[0]->BoneCount; ++i ) 
	{
		string szBoneName = pInfo->Skeletons[0]->Bones[i].Name;
		NStr::ToLowerASCII( &szBoneName );
		CGrannyBoneAttributesList::iterator posBone = pBoneList->insert( pBoneList->end(), SGrannyBoneAttributes() );
		posBone->szBoneName = szBoneName;
		posBone->szRealName = pInfo->Skeletons[0]->Bones[i].Name;
		int nOffset = 0;
		for ( int j = 0; pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Type != GrannyEndMember; ++j )
		{
			if ( pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Type == GrannyReal32Member ) 
			{
				string szName = pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Name;
				/**
				DebugTrace( "Reading attribute: <%s>", szName.c_str() );
				/**/
				NStr::ToLowerASCII( &szName );
				const float fValue = *(float*)( ((BYTE*)pInfo->Skeletons[0]->Bones[i].ExtendedData.Object) + nOffset );
				posBone->attributeMap[szName] = fValue;
				nOffset += 4;
				/**
				DebugTrace( "Read attribute: <%s:%g>", szName.c_str(), fValue );
				/**/
			}
			else if ( pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Type == GrannyInt32Member ) 
			{
				string szName = pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Name;
				/**
				DebugTrace( "Reading attribute: <%s>", szName.c_str() );
				/**/
				NStr::ToLowerASCII( &szName );
				const float fValue = *(int*)( ((BYTE*)pInfo->Skeletons[0]->Bones[i].ExtendedData.Object) + nOffset );
				posBone->attributeMap[szName] = fValue;
				/**
				DebugTrace( "Read Attribute: <%s:%g>", szName.c_str(), fValue );
				/**/
				nOffset += 4;
			}
			else if ( pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Type == GrannyStringMember )
			{
				nOffset += 4;
//				const int nMemberSize = GrannyGetMemberTypeSize( &(pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j]) );
//				const int nTotalSize = GrannyGetTotalObjectSize( &(pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j]) );
//				bool bHasPointers = GrannyMemberHasPointers( &(pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j]) );
//				ILogger *pLogger = NLog::GetLogger();
//				pLogger->Log( LT_ERROR, StrFmt("Attribute of type 'string' for \"%s\"\n", pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Name) );
//				break;
			}
			else
			{
				NLog::GetLogger()->Log( LT_IMPORTANT, StrFmt("Unknown attribute type for \"%s\"\n", pInfo->Skeletons[0]->Bones[i].ExtendedData.Type[j].Name) );
				break;
			}
		}
	}
	return true;
}


bool SGrannyBoneAttributes::GetAttribute( const string &rszAttributeName, float *pfValue ) const
{
	SGrannyBoneAttributes::CAttributeMap::const_iterator posAttribute = attributeMap.find( rszAttributeName );
	if ( posAttribute != attributeMap.end() )
	{
		if ( pfValue )
		{
			( *pfValue ) = posAttribute->second;
		}
		return true;
	}
	else
	{
		return false;
	}
}


bool SGrannyBoneAttributes::GetAttribute( const string &rszAttributeName, int *pnValue ) const
{
	float fAttributeValue = 0.0f;
	if ( GetAttribute( rszAttributeName, &fAttributeValue ) )
	{
		if ( pnValue )
		{
			( *pnValue ) = (int)( fAttributeValue );
		}
		return true;
	}
	return false;
}


bool SGrannyBoneAttributes::GetAttribute( const string &rszAttributeName, bool *pbValue ) const
{
	float fAttributeValue = 0.0f;
	if ( GetAttribute( rszAttributeName, &fAttributeValue ) )
	{
		if ( pbValue )
		{
			( *pbValue ) = ( fAttributeValue > 0.0f );
		}
		return true;
	}
	return false;
}


// ************************************************************************************************************************ //
// **
// ** granny file info guard
// **
// **
// **
// ************************************************************************************************************************ //

CGrannyFileInfoGuard::CGrannyFileInfoGuard( const string &szFileName )
{
	pFile = GrannyReadEntireFile( szFileName.c_str() );
	pInfo = GrannyGetFileInfo( pFile );
}

CGrannyFileInfoGuard::~CGrannyFileInfoGuard()
{
	if ( pFile ) 
		GrannyFreeFile( pFile );
}

// basement storage  


