#include "StdAfx.h"

#include "TerrainManager.h"
#include "../Stats_B2_M1/TerraAIObserver.h"
#include "../System/BinaryResources.h"
#include "../System/VFSOperations.h"

//TODO: divide LOAD and CREATE methods in TerraManager

namespace NScene
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const string SZ_TERRA_BIN_FILE_NAME = "map.b2m";
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CreateTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc, const string &szMapFilePath )
	{
		const string szTerrainBinFileName = szMapFilePath + "/" + SZ_TERRA_BIN_FILE_NAME;
		//
		//CFileStream stream( NVFS::GetMainVFS(), szTerrainBinFileName );
		pTerraManager->Load( pDesc, 0 );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool LoadTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc, const string &szMapFilePath )
	{
		const string szTerrainBinFileName = szMapFilePath + "/" + SZ_TERRA_BIN_FILE_NAME;
		//
		CFileStream stream( NVFS::GetMainVFS(), szTerrainBinFileName );
		if ( stream.IsOk() )
		{
			pTerraManager->Load( pDesc, &stream );
			return true;
		}
		//
		return false;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SaveTerrain( ITerraManager *pTerraManager, const string &szMapFilePath )
	{
		try
		{
			if ( const NDb::STerrain *pDesc = pTerraManager->GetDesc() ) 
			{
				const string szTerrainBinFileName = szMapFilePath + "/" + SZ_TERRA_BIN_FILE_NAME;
				//
				CFileStream stream( NVFS::GetMainFileCreator(), szTerrainBinFileName );
				if ( stream.IsOk() )
				{
					pTerraManager->Save( &stream );
					//
					return true;
				}
				//
				return true;
			}
		}
		catch ( ... ) 
		{
		}
		//
		return false;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};


void CTerrainManager::Setup( NGScene::IGameView *pGView, CFuncBase<STime> *_pTimer )
{
	pGameView = pGView;
	pTimer = _pTimer;
	if ( pTerraGen == 0 )
		pTerraGen = new CTerraGen();
	pTerraGen->AttachGameView( pGView );
	pTerraGen->AttachTimer( pTimer );
}


void CTerrainManager::SetAIObserver( interface ITerraAIObserver *pObserver )
{
	if ( pTerraGen == 0 )
		InitTerragen();
	pTerraGen->SetAIObserver( pObserver );
}


ITerraAIObserver* CTerrainManager::GetAIObserver()
{
	return pTerraGen->GetAIObserver();
}


void CTerrainManager::Load( const NDb::STerrain *_pDesc, CDataStream *pStream )
{
	//CheckPathes();
	pDesc = _pDesc;
	//
	if ( !pTerraGen )
		InitTerragen();
	//
	if ( pGameView )
	{
		pTerrain = new CSceneTerrain( pDesc, pGameView, pTimer );
		pTerraGen->SetGfxObserver( pTerrain );
	}
	pTerraGen->Load( pDesc, pStream );
}


void CTerrainManager::Save( CDataStream *pStream )
{
	//CheckPathes();
	pTerraGen->Save( pStream );
}


void CTerrainManager::AddRoad( const NDb::SVSOInstance *pInstance )
{
	DebugTrace( "CTerrainManager::AddRoad() ID: 0x%X", pInstance->nVSOID );
	pTerraGen->AddRoad( pInstance );
}


void CTerrainManager::UpdateRoad( const int nVSOID )
{
	DebugTrace( "CTerrainManager::UpdateRoad() ID: 0x%X", nVSOID );
	pTerraGen->UpdateRoad( nVSOID );
}


void CTerrainManager::RemoveRoad( const int nVSOID )
{
	DebugTrace( "CTerrainManager::RemoveRoad() ID: 0x%X", nVSOID );
	pTerraGen->RemoveRoad( nVSOID );
}


void CTerrainManager::AddCrag( const NDb::SVSOInstance *pInstance )
{
	DebugTrace( "CTerrainManager::AddCrag() ID: 0x%X", pInstance->nVSOID );
	pTerraGen->AddCrag( pInstance );
}


void CTerrainManager::UpdateCrag( const int nVSOID )
{
	DebugTrace( "CTerrainManager::UpdateCrag() ID: 0x%X", nVSOID );
	pTerraGen->UpdateCrag( nVSOID );
}


void CTerrainManager::RemoveCrag( const int nVSOID )
{
	DebugTrace( "CTerrainManager::RemoveCrag() ID: 0x%X", nVSOID );
	pTerraGen->RemoveCrag( nVSOID );
}


void CTerrainManager::AddRiver( const NDb::SVSOInstance *pInstance )
{
	DebugTrace( "CTerrainManager::AddRiver() ID: 0x%X", pInstance->nVSOID );
	pTerraGen->AddRiver( pInstance );
}


void CTerrainManager::UpdateRiver( const int nVSOID )
{
	DebugTrace( "CTerrainManager::UpdateRiver() ID: 0x%X", nVSOID );
	pTerraGen->UpdateRiver( nVSOID );
}


void CTerrainManager::RemoveRiver( const int nVSOID )
{
	DebugTrace( "CTerrainManager::RemoveRiver() ID: 0x%X", nVSOID );
	pTerraGen->RemoveRiver( nVSOID );
}


void CTerrainManager::AddTerraSpot( const NDb::STerrainSpotInstance *pInstance )
{
	pTerraGen->AddTerraSpot( pInstance );
}


void CTerrainManager::UpdateTerraSpot( const int nSpotID )
{
	pTerraGen->UpdateTerraSpot( nSpotID );
}


void CTerrainManager::RemoveTerraSpot( const int nSpotID )
{
	pTerraGen->RemoveTerraSpot( nSpotID );
}


void CTerrainManager::AddEntrenchment( const vector<CVec2> &_ctrlPoints, const float _fWidth )
{
	pTerraGen->AddEntrenchment( _ctrlPoints, _fWidth );
}


void CTerrainManager::RestoreFromHistory()
{
	//pTerraGen->SetStreamPathes( szDstPath, szSrcPath );
	//bPathesSet = true;
	//
	//Load( pDesc );
	pTerraGen->RestoreFromHistory();
}


int CTerrainManager::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pTerraGen );
	//saver.Add( 2, &szSrcPath );
	//saver.Add( 3, &szDstPath );
	saver.Add( 4, &pDesc );
	/*
	saver.Add( 1, &szSrcPath );
	saver.Add( 2, &szDstPath );
	saver.Add( 3, &pDesc );
	saver.Add( 4, &pGameView );
	saver.Add( 5, &pTimer );
	if ( saver.IsReading() ) 
	{
		pTerraGen = new CTerraGen();
		pTerraGen->SetStreamPathes(szDstPath, szSrcPath); 
		bPathesSet = true; 
		if ( pGameView ) 
		{
			pTerrain = new CSceneTerrain( pDesc, pGameView, pTimer );
			pTerraGen->SetGfxObserver( pTerrain );
		}
		pTerraGen->Load( pDesc );
	}
	*/
	return 0;
}


float CTerrainManager::GetZ( float x, float y ) const
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->GetZ( x, y );
		}
	}

	//NI_ASSERT( 0, "Error getting height from AI Observer" )
	return 0.0f;
}


float CTerrainManager::GetTileHeight( int nX, int nY ) const
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->GetTileHeight( nX, nY );
		}
	}

	//NI_ASSERT( 0, "Error getting tile height from AI Observer" )
	return 0.0f;
}


void CTerrainManager::UpdateZ( CVec3 *pvPos )
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->UpdateZ( pvPos );
		}
	}

	//NI_ASSERT( 0, "Error updating AI Observer height" )
}


DWORD CTerrainManager::GetNormal( const CVec2 &vPoint ) const
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->GetNormal( vPoint );
		}
	}

	//NI_ASSERT( 0, "Error getting normal from AI Observer" )
	return 0;
}


bool CTerrainManager::GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->GetIntersectionWithTerrain( pvResult, vBegin, vEnd );
		}
	}

	//NI_ASSERT( 0, "Error getting terra intersection from AI Observer" )
	return false;
}


bool CTerrainManager::GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->GetIntersectionWithTerrainForEditor( pvResult, vBegin, vEnd );
		}
	}

	//NI_ASSERT( 0, "Error getting terra intersection from AI Observer" )
	return false;
}


void CTerrainManager::InitHeights4Editor( int nSizeX, int nSizeY )
{
	//if ( pTerraGen )
	{
		ITerraAIObserver *pAIObserver = pTerraGen->GetAIObserver();
		if ( pAIObserver )
		{
			return pAIObserver->InitHeights4Editor( nSizeX, nSizeY );
		}
	}

	//NI_ASSERT( 0, "Error init heights for AI Observer" )
}


void CTerrainManager::CreateDebris(	const string &szFileName, CArray2D<BYTE> *pImage, CVec2 *pOrigin,
																		const NDebrisBuilder::EMaskType maskType, const int nSmoothRadius,
																		const NDebrisBuilder::EMaskSmoothType smoothType )
{
	pTerraGen->CreateDebris( szFileName, pImage, pOrigin, maskType, nSmoothRadius, smoothType );
}


void CTerrainManager::CreateSelection( NMeshData::SMeshData *pData, const CVec3 &vMin, const CVec3 &vMax )
{
	pTerraGen->CreateSelection( pData, vMin, vMax, this );
}


void CTerrainManager::AddTrack(	const int nID, const float fFadingSpeed,
																const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4,
																const CVec2 &vNorm, const float _fWidth, const float fAplha, CTracksManager *pTracksManager )
{
	pTerraGen->AddTrack( nID, fFadingSpeed, _v1, _v2, _v3, _v4, vNorm, _fWidth, fAplha, pTracksManager );
}


void CTerrainManager::AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial )
{
	pTerraGen->AddExplosion( _vMin, _vMax, pMaterial );
}


void CTerrainManager::ModifyTerraGeometryByBrush(	const int nVisTileX, const int nVisTileY, bool bCenter,
																									const CArray2D<float> &brush,
																									const NTerraBrush::ETerraBrushUpdate terraBrushUpdate )
{
	pTerraGen->ModifyTerraGeometryByBrush( nVisTileX, nVisTileY, bCenter, brush, terraBrushUpdate );
}


void CTerrainManager::GetTerraGeometryUpdateDifferences( int *pOffsX, int *pOffsY, CArray2D<float> *pDiffs )
{
	pTerraGen->GetTerraGeometryUpdateDifferences( pOffsX, pOffsY, pDiffs );
}


void CTerrainManager::UpdateAllObjectsInGeomModifyingArea()
{
	pTerraGen->UpdateAllObjectsInGeomModifyingArea();
}


void CTerrainManager::UpdateWater()
{
	pTerraGen->UpdateWater();
}


float CTerrainManager::GetTerraHeight( const float x, const float y ) const
{
	return pTerraGen->GetTerraHeight( x, y );
}


float CTerrainManager::GetRealTerraHeight( const float x, const float y ) const
{
	return pTerraGen->GetRealTerraHeight( x, y );
}


float CTerrainManager::GetTerraHeightFast( const int nTileX, const int nTileY ) const
{
	return pTerraGen->GetTerraHeightFast( nTileX, nTileY );
}


float CTerrainManager::GetRealTerraHeightFast( const int nTileX, const int nTileY ) const
{
	return pTerraGen->GetRealTerraHeightFast( nTileX, nTileY );
}


void CTerrainManager::UpdateRiversDepthes()
{
	pTerraGen->UpdateRiversDepthes();
}


void CTerrainManager::AddDynamicDebris( const CVec2 &vPos, const CVec2 &vSize,
																				const float fAngle, const int nSmoothRad,
																				const NDb::SMaterial *pMaterial )
{
	pTerraGen->AddDynamicDebris( vPos, vSize, fAngle, nSmoothRad, pMaterial );
}


void CTerrainManager::ApplyBridgeTerraForm( const CVec2 &_p1, const CVec2 &_p2, const float fWidth, const float fHeight )
{
	pTerraGen->ApplyBridgeTerraForm( _p1, _p2, fWidth, fHeight );
}


void CTerrainManager::ApplyObjectTerraForm( const CVec2 &_p1, const CVec2 &_p2, const CVec2 &_p3, const CVec2 &_p4 )
{
	pTerraGen->ApplyObjectTerraForm( _p1, _p2, _p3, _p4 );
}


void CTerrainManager::HideTerrain( bool bHide )
{
	if ( pTerrain )
	{
		pTerrain->HideTerrain( bHide );
	}
}


void CTerrainManager::UpdateTileAreaType( const float fXo, const float fYo, const CArray2D<BYTE> &mask,
																					const NTerraBrush::ETerraBrushUpdate terraBrushUpdate )
{
	pTerraGen->UpdateTileAreaType( fXo, fYo, mask, terraBrushUpdate );
}


void CTerrainManager::GetTileTypeUpdateDifferences( float *pOffsX, float *pOffsY, CArray2D<BYTE> *pDiffs )
{
	pTerraGen->GetTileTypeUpdateDifferences( pOffsX, pOffsY, pDiffs );
}


void CTerrainManager::FinalizeTexModifying()
{
	pTerraGen->FinalizeTexModifying();
}


void CTerrainManager::ClampUnderRivers( NMeshData::SMeshData *pData )
{
	pTerraGen->ClampUnderRivers( pData );
}


void CTerrainManager::GetAreaTileTypes( CArray2D<BYTE> *pAreaTypes, const int nX1, const int nY1, const int nX2, const int nY2 )
{
	pTerraGen->GetAreaTileTypes( pAreaTypes, nX1, nY1, nX2, nY2 );
}


void CTerrainManager::GetAreaHeights(	CArray2D<float> *pAreaHeights, const int nX1, const int nY1, const int nX2, const int nY2 )
{
	pTerraGen->GetAreaHeights( pAreaHeights, nX1, nY1, nX2, nY2 );
}


void CTerrainManager::UpdateAfterTilesModifying()
{
	pTerraGen->UpdateAfterTilesModifying();
}


void CTerrainManager::SetRainyWaters( CDBPtr<NDb::SWater> pRainyWater )
{
	pTerraGen->InitRainyWater( pRainyWater );
}


BASIC_REGISTER_CLASS( ITerraManager );
REGISTER_SAVELOAD_CLASS( 0x10096401, CTerrainManager )


