#include "stdafx.h"
#include "Misc/StrProc.h"
#include <float.h>
#include "ED_Common/Tools_Granny.h"
#include "MapEditorLib/CommonExporterMethods.h"
#include "MapEditorLib/Interface_MOD.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/RandomGen.h"
#include "System/BinaryResources.h"
#include "3Dmotor/DBScene.h"
#include "TraceModel.h"

//
// http://www.acm.org/jgt/papers/MollerTrumbore97/code.html
//
int IntersectTriangle(	float *t, 
												float *pU, 
												float *pV,
												const CVec3 &rvOrig,
												const CVec3 &rvDir,
												const CVec3 &rvVert0,
												const CVec3 &rvVert1,
												const CVec3 &rvVert2 )
{
   /* find vectors for two edges sharing vert0 */
   CVec3 vEdge1 = (rvVert1 - rvVert0);
	 CVec3 vEdge2 = (rvVert2 - rvVert0);	
   
   /* begin calculating determinant - also used to calculate U parameter */
	 CVec3 vPvec = rvDir ^ vEdge2;
   
   /* if determinant is near zero, ray lies in plane of triangle */
   float fDet = vEdge1 * vPvec;

   if ( fDet > -FLT_EPSILON && fDet < FLT_EPSILON )
     return 0;

   float fInvDet = 1.0 / fDet;

   /* calculate distance from vert0 to ray origin */
   CVec3 vTvec = rvOrig - rvVert0;

   /* calculate U parameter and test bounds */
   *pU = (vTvec * vPvec) * fInvDet;
   if (*pU < 0.0 || *pU > 1.0)
     return 0;

   /* prepare to test V parameter */
	 CVec3 vQvec = vTvec ^ vEdge1;	
   
   /* calculate V parameter and test bounds */
   *pV = ( rvDir * vQvec ) * fInvDet;
   if (*pV < 0.0 || *pU + *pV > 1.0)
     return 0;

   /* calculate t, ray intersects triangle */
   *t = ( vEdge2 * vQvec ) * fInvDet;

   return 1;
}

static int TraceCell(	SModelSurfacePoint *pSurfacePoint0, SModelSurfacePoint *pSurfacePoint1,
											const std::vector<STriangleForTrace> &rTrianglesForTrace, 
											const CVec3 &vRayOrig, const CVec3 vDir )
{
	//
	float fMinDist = 0;
	float fMaxDist = 0;
	int nMinIdx = -1;
	int nMaxIdx = -1;
	float fMinT = 0;
	float fMaxT = 0;
	for ( int k = 0; k < rTrianglesForTrace.size(); ++k )
	{
		float t = 0;
		float u = 0;
		float v = 0;
		//
		const STriangleForTrace &rTr = rTrianglesForTrace[k];
		if ( IntersectTriangle( &t, &u, &v, vRayOrig, vDir, 
				 rTr.vertices[0], rTr.vertices[1], rTr.vertices[2] ) )
		{
			CVec3 vIntersPoint = vRayOrig + vDir * t;
			float fCurDist = fabs( vIntersPoint - vRayOrig );
			//
			if ( nMinIdx == -1 )
			{
				nMinIdx = k;
				fMinDist = fCurDist;
				fMinT = t;
			}
			else
			{
				if ( fCurDist < fMinDist )
				{
					nMinIdx = k;
					fMinDist = fCurDist;
					fMinT = t;
				}
			}
			//
			if ( nMaxIdx == -1 )
			{
				nMaxIdx = k;

				fMaxDist = fCurDist;
				fMaxT = t;
			}
			else
			{
				if ( fCurDist > fMaxDist )
				{
					nMaxIdx = k;
					fMaxDist = fCurDist;
					fMaxT = t;
				}
			}
		}
	}
	//
	int nResult = 0;
	if ( nMaxIdx != -1 )
	{
		SModelSurfacePoint msp;
		msp.szBodyPart = rTrianglesForTrace[nMaxIdx].szBodyPart;
		msp.vPos = vRayOrig + vDir * fMaxT;
		Vis2AI( &msp.vPos, msp.vPos );
		CVec3 vEdg1 = ( rTrianglesForTrace[nMaxIdx].vertices[1] - rTrianglesForTrace[nMaxIdx].vertices[0] );
		CVec3 vEdg2 = ( rTrianglesForTrace[nMaxIdx].vertices[2] - rTrianglesForTrace[nMaxIdx].vertices[1] );
		msp.vNormal = vEdg1 ^ vEdg2;
		Normalize( &msp.vNormal );
		*pSurfacePoint1 = msp;
		nResult |= 2;
	}
	//
	if ( nMinIdx != -1 )
	{
		SModelSurfacePoint msp;
		msp.szBodyPart = rTrianglesForTrace[nMinIdx].szBodyPart;
		msp.vPos = vRayOrig + vDir * fMinT;
		Vis2AI( &msp.vPos, msp.vPos );
		CVec3 vEdg1 = ( rTrianglesForTrace[nMinIdx].vertices[1] - rTrianglesForTrace[nMinIdx].vertices[0] );
		CVec3 vEdg2 = ( rTrianglesForTrace[nMinIdx].vertices[2] - rTrianglesForTrace[nMinIdx].vertices[1] );
		msp.vNormal = vEdg1 ^ vEdg2;
		Normalize( &msp.vNormal );
		*pSurfacePoint0 = msp;
		nResult |= 1;
	}
	//
	return nResult;
}

void TraceTriangles(	std::vector<SModelSurfacePoint> *pSurfacePoints, 
											const std::vector<STriangleForTrace> &rTrianglesForTrace,
											const CVec3 &rvMin, const CVec3 &rvMax,
											int nNumPointsPerCell )
{
	//
	const int N_MAX_CYCLES_NUM = 100;
	//
	float fAITileSize = VIS_TILE_SIZE / float( AI_TILES_IN_VIS_TILE );
	//
	int nDivX = (rvMax.x - rvMin.x) / Clamp( ( rvMax.x - rvMin.x ) / 10.0f, fAITileSize, fAITileSize * 8.0f );
	int nDivY = (rvMax.y - rvMin.y) / Clamp( ( rvMax.y - rvMin.y ) / 10.0f, fAITileSize, fAITileSize * 8.0f );
	int nDivZ = (rvMax.z - rvMin.z) / Clamp( ( rvMax.z - rvMin.z ) / 10.0f, fAITileSize, fAITileSize * 8.0f );
	//
	float fDx = (rvMax.x - rvMin.x) / nDivX;
	float fDy = (rvMax.y - rvMin.y) / nDivY;
	float fDz = (rvMax.z - rvMin.z) / nDivZ;
	//
	// side (Xmin..Xmax, Ymin, Zmin..Zmax)
	//
	for ( int i = 0; i < nDivX; ++i )
	{
		for ( int j = 0; j < nDivZ; ++j )
		{
			CVec3 vCellPos = CVec3( rvMin.x + (fDx * i), rvMin.y, rvMin.z + (fDz * j) );
			CVec3 vRayOrig = vCellPos;

			int nNumPointsFound = 0;
			for ( int nCycle = 0; nCycle < N_MAX_CYCLES_NUM; ++nCycle )
			{
				vRayOrig.x += NRandom::Random( 0.0f, fDx ); RecordRandomCall();
				vRayOrig.z += NRandom::Random( 0.0f, fDz ); RecordRandomCall();

				SModelSurfacePoint sp0;
				SModelSurfacePoint sp1;
				int nRes = TraceCell(	&sp0, &sp1, rTrianglesForTrace, vRayOrig, V3_AXIS_Y ); 

				if ( nRes )
				{
					if ( nRes & 1 ) 
						pSurfacePoints->push_back( sp0 );
					if ( nRes & 2 ) 
						pSurfacePoints->push_back( sp1 );

					++nNumPointsFound;
					if ( nNumPointsFound >= nNumPointsPerCell )
						break;
				}
			}
		}
	}
	//
	//
	// side (Xmin, Ymin..Ymax, Zmin..Zmax)
	//
	for ( int i = 0; i < nDivY; ++i )
	{
		for ( int j = 0; j < nDivZ; ++j )
		{
			CVec3 vCellPos = CVec3( rvMin.x, rvMin.y + (fDy * i), rvMin.z + (fDz * j) );
			CVec3 vRayOrig = vCellPos;
	
			int nNumPointsFound = 0;
			for ( int nCycle = 0; nCycle < N_MAX_CYCLES_NUM; ++nCycle )
			{
				vRayOrig.y += NRandom::Random( 0.0f, fDy ); RecordRandomCall();
				vRayOrig.z += NRandom::Random( 0.0f, fDz ); RecordRandomCall();

				SModelSurfacePoint sp0;
				SModelSurfacePoint sp1;
				int nRes = TraceCell(	&sp0, &sp1, rTrianglesForTrace, vRayOrig, V3_AXIS_X ); 

				if ( nRes )
				{
					if ( nRes & 1 ) 
						pSurfacePoints->push_back( sp0 );
					if ( nRes & 2 ) 
						pSurfacePoints->push_back( sp1 );

					++nNumPointsFound;
					if ( nNumPointsFound >= nNumPointsPerCell )
						break;
				}
			}
		}
	}
	//
}

int TraceModel( std::vector<SModelSurfacePoint> *pSurfacePoints, granny_file_info *pFile )
{
	if ( !pSurfacePoints )
		return -1;
	//
	if ( !pFile )
		return -1;
	//
	if ( pFile->ModelCount < 1 )
		return -1;
	//
	CVec3 vMin, vMax;
	if ( !GetGrannyMeshBoundingBox( &vMin, &vMax, pFile ) )
	{
		return -1;
	}
	//
	granny_model *pModel = pFile->Models[0];
	//
	std::vector<STriangleForTrace> trianglesForTrace;
	int nTotalTriangles = 0;
	for ( int nMeshIdx = 0; nMeshIdx < pModel->MeshBindingCount; ++nMeshIdx )
	{
		granny_mesh *pMesh = pModel->MeshBindings[ nMeshIdx ].Mesh;
		//
		granny_int32 nIndexCount = GrannyGetMeshIndexCount(pMesh);
		granny_int32 nBytesPerIndex = GrannyGetMeshBytesPerIndex(pMesh);
		granny_uint8 *pIndices = (granny_uint8*)GrannyGetMeshIndices(pMesh);
		//
		granny_int32x nVertexCount = GrannyGetMeshVertexCount(pMesh);
		granny_data_type_definition *pVertexType = GrannyGetMeshVertexType(pMesh);
		const int nPositionOffset = CalculateGrannyTypedefOffset( pVertexType, GrannyVertexPositionName );
		const int nVertexSize = GrannyGetTotalObjectSize( pVertexType );
		granny_uint8 *pVertices = (granny_uint8*)GrannyGetMeshVertices(pMesh);
		//
		int nNumTriangles = nIndexCount / 3;
		for(int nTriIndex = 0; nTriIndex < nNumTriangles; ++nTriIndex )
		{
			STriangleForTrace tr;
			tr.szBodyPart = pMesh->Name;
			for ( int v = 0; v < 3; ++v )
			{
				int nVertexIndex = 0;
				if ( nBytesPerIndex == sizeof(short) )
				{
					nVertexIndex = *(short*)( &pIndices[ ( nTriIndex * 3 + v ) * nBytesPerIndex ] );
				}
				else
				{
					nVertexIndex = *(int*)( &pIndices[ ( nTriIndex * 3 + v ) * nBytesPerIndex ] );
				}
				
				granny_uint8 *pVtxData = &pVertices[ nVertexIndex * nVertexSize ];
				granny_uint8 *pPosData = &pVtxData[ nPositionOffset ];
				//
				tr.vertices[v] = *(CVec3*)pPosData;
			}
			//
			trianglesForTrace.push_back( tr );
			//
			++nTotalTriangles;
		}
	}
	//
	TraceTriangles( pSurfacePoints, trianglesForTrace, vMin, vMax, 1 );
	//
	return pSurfacePoints->size();
}

bool TraceModel( std::vector<SModelSurfacePoint> *pSurfacePoints, const std::string &rszGeometryResourceName )
{
	if ( !pSurfacePoints )
		return false;

	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( !pUserData )
		return false;;

	const std::string szGeometriesFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Geometries\\";

	//char pszBuf[32] = {0};
	//sprintf( pszBuf, "%d", nGeometryResourceID );
	//std::string szFilePath = szGeometriesFolder + pszBuf;
	CDBPtr<NDb::SGeometry> pGeometry = NDb::Get<NDb::SGeometry>( CDBID( rszGeometryResourceName ) );
	std::string szFilePath = NBinResources::GetExistentBinaryFileName( szGeometriesFolder, pGeometry->GetRecordID(), pGeometry->uid  ); // uid

	WaitForFile( szFilePath, 10000 );
	granny_file *pFile = GrannyReadEntireFile( szFilePath.c_str() );
	if ( !pFile )
		return false;

	granny_file_info *pInfo = GrannyGetFileInfo( pFile );				
	if ( !pInfo )
		return false;

	bool bRes = TraceModel( pSurfacePoints, pInfo );

	GrannyFreeFile( pFile );

	return bRes;
}


