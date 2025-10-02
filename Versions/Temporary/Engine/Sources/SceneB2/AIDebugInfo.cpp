#include "stdafx.h"

#include "B2_M1_World/CommonB2M1AI.h"
#include "3Dlib/GGeometry.h"
#include "3Dmotor/DBScene.h"
#include "AIDebugInfo.h"

const int MAX_TILES_IN_MARKER = 1024;

namespace NAIVisInfo
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void MakeVertex( NGScene::SVertex *pRes, const CVec2 &vAIPoint, const CVec2 &vTex )
	{
		ICommonB2M1AI *pAI = Singleton<ICommonB2M1AI>();
		
		const CVec3 vNormal( DWORDToVec3(Scene()->GetNormal( vAIPoint )) );
		const float fZ = Scene()->GetZ( vAIPoint.x, vAIPoint.y ) + 10;

		NGfx::CalcCompactVector( &(pRes->normal), vNormal );
		NGfx::CalcCompactVector( &(pRes->texU), CVec3(1, 0, 0) );
		NGfx::CalcCompactVector( &(pRes->texV), CVec3(0, 1, 0) );

		pRes->tex = vTex;
		pRes->pos.Set( vAIPoint.x, vAIPoint.y, fZ ); ;
		AI2Vis( &(pRes->pos) );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ************************************************************************************************************************ //
	// * CEngineInfo
	// ************************************************************************************************************************ //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CEngineInfo::InitEngineInfo( const NDb::SMaterial *pColor )
	{
		bNeedUpdate = true;

		SFBTransform placement;
		Identity( &placement.forward );
		Identity( &placement.backward );

		//pMaterial = NDb::Get<NDb::SMaterial>( 142 );
		NGScene::IGameView *pGView = Scene()->GetGView();
		pMesh = pGView->CreateMesh( pGView->MakeMeshInfo( this, pColor ), placement, 0, 0 );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CEngineInfo::Recalc()
	{
		bNeedUpdate = false;

		if ( pValue == 0 )
			pValue = new NGScene::CObjectInfo;

		NGScene::CObjectInfo::SData objData;

		// calculate vis vertices
		objData.verts.resize( aiVerts.size() );

		CVec2 vTex( 0.0f, 0.0f );
		for ( int i = 0; i < aiVerts.size(); ++i )
			MakeVertex( &objData.verts[i], aiVerts[i], vTex );

		objData.geometry = tris;

		// 
		pValue->Assign( &objData, false );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CEngineInfo::Invalidate()
	{
		pMesh = 0;
		pMaterial = 0;
		bNeedUpdate = true;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ************************************************************************************************************************ //
	// * CDebugSegment
	// ************************************************************************************************************************ //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const float CDebugSegment::F_MIN_AI_THICKNESS = float(AI_TILE_SIZE) / 8.0f;
	const float CDebugSegment::F_AI_SPLICE_LEN = float(AI_TILE_SIZE) / 10;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugSegment::Init( const CSegment &aiSegment, const int nThickness )
	{
		fThick = F_MIN_AI_THICKNESS * nThickness / 2.0f;	
		SetPosition( aiSegment );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugSegment::SetPosition( const CSegment &aiSegment )
	{ 
		SetNeedUpdate( true );

		CVec2 vDir( aiSegment.dir );
		Normalize( &vDir ); 

		// calculate perpendicular
		vDirPerp.x = -vDir.y * fThick;
		vDirPerp.y = vDir.x * fThick;

		// calculate splice step
		vDir *= F_AI_SPLICE_LEN;
		const float fTotalLen = fabs( aiSegment.p2 - aiSegment.p1 );

		// splice segment
		segmentsSplice.reserve( fTotalLen / F_AI_SPLICE_LEN + 1 );
		float fLen = 0.0f;
		CVec2 vCurPoint( aiSegment.p1 );
		while ( fLen < fTotalLen )
		{
			CVec2 vNextPoint;
			if ( fLen + F_AI_SPLICE_LEN < fTotalLen )
			{
				vNextPoint = vCurPoint + vDir;
				fLen += F_AI_SPLICE_LEN;
			}
			else
			{
				vNextPoint = aiSegment.p2;
				fLen = fTotalLen;
			}
			segmentsSplice.push_back( CSegment( vCurPoint, vNextPoint ) );

			vCurPoint = vNextPoint;
		}

		vector<CVec2> aiVerts( segmentsSplice.size() * 4 );
		vector<STriangle> tris( segmentsSplice.size() * 2 );

		for ( int i = 0; i < segmentsSplice.size(); ++i )
		{
			// calculate ai vertices
			aiVerts[4*i + 0] = segmentsSplice[i].p1 - vDirPerp;
			aiVerts[4*i + 1] = segmentsSplice[i].p2 - vDirPerp;
			aiVerts[4*i + 2] = segmentsSplice[i].p2 + vDirPerp;
			aiVerts[4*i + 3] = segmentsSplice[i].p1 + vDirPerp;

			// calculate triangles
			tris[2*i + 0].i1 = 4*i + 0;
			tris[2*i + 0].i2 = 4*i + 1;
			tris[2*i + 0].i3 = 4*i + 2;
			tris[2*i + 1].i1 = 4*i + 0;
			tris[2*i + 1].i2 = 4*i + 2;
			tris[2*i + 1].i3 = 4*i + 3;
		}

		SetVerts( aiVerts );
		SetTris( tris );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ************************************************************************************************************************ //
	// * CDebugCircle
	// ************************************************************************************************************************ //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const int CDebugCircle::N_SPLICE = 32;
	const float CDebugCircle::F_AI_TURN_X = cos( 2 * PI / N_SPLICE );
	const float CDebugCircle::F_AI_TURN_Y = sin( 2 * PI / N_SPLICE );
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugCircle::Init( const CCircle &aiCircle )
	{
		SetPosition( aiCircle );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugCircle::SetPosition( const CCircle &aiCircle )
	{
		SetNeedUpdate( true );
		
		static CVec2 vTurn( F_AI_TURN_X, F_AI_TURN_Y );
		CVec2 vDir( aiCircle.r, 0.0f );

		vector<CVec2> aiVerts( N_SPLICE * 3 );
		vector<STriangle> tris( N_SPLICE );

		for ( int i = 0; i < N_SPLICE; ++i )
		{
			CVec2 vNextDir( vDir ^ vTurn );

			aiVerts[i*3 + 0] = aiCircle.center;
			aiVerts[i*3 + 1] = aiCircle.center + vDir;
			aiVerts[i*3 + 2] = aiCircle.center + vNextDir;

			tris[i].i1 = i*3 + 0;
			tris[i].i2 = i*3 + 1;
			tris[i].i3 = i*3 + 2;

			vDir = vNextDir;
		}

		SetVerts( aiVerts );
		SetTris( tris );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ************************************************************************************************************************ //
	// * CDebugMarker
	// ************************************************************************************************************************ //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugMarker::Init( const vector<SVector> &tiles, const int nStart, const int nEnd )
	{
		SetPosition( tiles, nStart, nEnd );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugMarker::SetPosition( const vector<SVector> &tiles, const int nStart, const int nEnd )
	{
		SetNeedUpdate( true );

		vector<CVec2> aiVerts( (nEnd - nStart) * 4 );
		vector<STriangle> tris( (nEnd - nStart) * 2 );

		int cnt = 0;
		for ( int i = nStart; i < nEnd; ++i )
		{
			const CVec2 vPoint( tiles[i].x * AI_TILE_SIZE, tiles[i].y * AI_TILE_SIZE );

			aiVerts[cnt*4 + 0] = vPoint;
			aiVerts[cnt*4 + 1] = vPoint + CVec2( AI_TILE_SIZE, 0 );
			aiVerts[cnt*4 + 2] = vPoint + CVec2( AI_TILE_SIZE, AI_TILE_SIZE );
			aiVerts[cnt*4 + 3] = vPoint + CVec2( 0, AI_TILE_SIZE );

			tris[cnt*2].i1 = cnt*4 + 0;
			tris[cnt*2].i2 = cnt*4 + 1;
			tris[cnt*2].i3 = cnt*4 + 2;

			tris[cnt*2 + 1].i1 = cnt*4 + 0;
			tris[cnt*2 + 1].i2 = cnt*4 + 2;
			tris[cnt*2 + 1].i3 = cnt*4 + 3;

			++cnt;
		}

		SetVerts( aiVerts );
		SetTris( tris );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ************************************************************************************************************************ //
	// * CDebugMarkers
	// ************************************************************************************************************************ //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugMarkers::Init( const vector<SVector> &tiles )
	{
		SetPosition( tiles );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugMarkers::SetPosition( const vector<SVector> &tiles )
	{
		markers.clear();

		const int nPatchX = MAXIMUM_MAP_SIZE / AI_TILE_SIZE;
		const int nPatchY = MAXIMUM_MAP_SIZE / AI_TILE_SIZE;
		CArray2D< vector<SVector> > patches;
		patches.SetSizes( nPatchX, nPatchY );

		for ( vector<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
		{
			const int nTileX = it->x / AI_TILE_SIZE;
			const int nTileY = it->y / AI_TILE_SIZE;
			patches[nTileY][nTileX].push_back( *it );
		}

		for ( int nX = 0; nX < nPatchX; ++nX )	
		{
			for ( int nY = 0; nY < nPatchY; ++nY )
			{
				CPtr<CDebugMarker> pMarker = new NAIVisInfo::CDebugMarker();
				pMarker->Init( patches[nY][nX] );
				markers.push_back( pMarker );
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CDebugMarkers::SetColor( const NDb::SMaterial *pColor )
	{
		for ( TMarkers::iterator it = markers.begin(); it != markers.end(); ++it )
		{
			CPtr<CDebugMarker> pMarker = *it;
			pMarker->SetColor( pColor );
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}


