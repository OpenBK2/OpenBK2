#pragma once

#include "Terrain.h"
#include "GenTerrain.h"

class CTerrainManager : public ITerraManager
{
	OBJECT_NOCOPY_METHODS( CTerrainManager )
	//
	CObj<CTerraGen> pTerraGen;
	CObj<CSceneTerrain> pTerrain;
	//
	CDBPtr<NDb::STerrain> pDesc;
	CPtr<NGScene::IGameView> pGameView;
	CDGPtr< CFuncBase<STime> > pTimer;
	//
	void InitTerragen()
	{
		pTerraGen = new CTerraGen();
		if ( pGameView )
			pTerraGen->AttachGameView( pGameView );
		if ( pTimer )
			pTerraGen->AttachTimer( pTimer );
	}
	//
public:
	CTerrainManager() {}
	//
	void Setup( NGScene::IGameView *pGView, CFuncBase<STime> *pTimer );
	void Load( const NDb::STerrain *pDesc, CDataStream *pStream );
	const NDb::STerrain *GetDesc() const { return pDesc; }
	void SetAIObserver( struct ITerraAIObserver *pObserver );
	virtual struct ITerraAIObserver* GetAIObserver();
	void UpdateAIInfo() { pTerraGen->UpdateAIInfo(); }

	//
	void ReGenerate()
	{
		NI_ASSERT( pDesc != 0, "Load terrain desc first!" );
		pTerraGen->ReGenerate();
	}
	//
	void Save( CDataStream *pStream );
	// road
	void AddRoad( const NDb::SVSOInstance *pInstance );
	void UpdateRoad( const int nVSOID );
	void RemoveRoad( const int nVSOID );
	// crag
	void AddCrag( const NDb::SVSOInstance *pInstance );
	void UpdateCrag( const int nVSOID );
	void RemoveCrag( const int nVSOID );
	// river
	void AddRiver( const NDb::SVSOInstance *pInstance );
	void UpdateRiver( const int nVSOID );
	void RemoveRiver( const int nVSOID );
	// terra spot
	void AddTerraSpot( const NDb::STerrainSpotInstance *pInstance );
	void UpdateTerraSpot( const int nSpotID );
	void RemoveTerraSpot( const int nSpotID );
	//
	void AddEntrenchment( const std::vector<CVec2> &_ctrlPoints, const float _fWidth );
	//
	void CreateDebris( const std::string &szFileName, CArray2D<BYTE> *pImage, CVec2 *pOrigin,
										 const NDebrisBuilder::EMaskType maskType, const int nSmoothRadius,
										 const NDebrisBuilder::EMaskSmoothType smoothType );
	// selection
	void CreateSelection( NMeshData::SMeshData *pData, const CVec3 &vMin, const CVec3 &vMax );
	// tracks
	void AddTrack( const int nID, const float fFadingSpeed,
								 const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4,
								 const CVec2 &vNorm, const float _fWidth, const float fAplha, CTracksManager *pTracksManager );
	// explosions
	void AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial );
	//
	void ModifyTerraGeometryByBrush( const int nVisTileX, const int nVisTileY, bool bCenter,
																	 const CArray2D<float> &brush,
																	 const NTerraBrush::ETerraBrushUpdate terraBrushUpdate );
	//
	void GetTerraGeometryUpdateDifferences( int *pOffsX, int *pOffsY, CArray2D<float> *pDiffs );
	void UpdateAllObjectsInGeomModifyingArea();
	void UpdateWater();
	float GetTerraHeight( const float x, const float y ) const;
	float GetRealTerraHeight( const float x, const float y ) const;
	float GetTerraHeightFast( const int nTileX, const int nTileY ) const;
	float GetRealTerraHeightFast( const int nTileX, const int nTileY ) const;
	//
	bool GetCragPrecVerts( std::vector<CVec3> *pVerts, int nVSOId ) { return pTerraGen->GetCragPrecVerts( pVerts, nVSOId ); }
	//
	void UpdateRiversDepthes();
	//
	void AddDynamicDebris(	const CVec2 &vPos, const CVec2 &vSize,
													const float fAngle, const int nSmoothRad,
													const NDb::SMaterial *pMaterial );
	void ApplyBridgeTerraForm(	const CVec2 &_p1, const CVec2 &_p2, const float fWidth, const float fHeight );
	void ApplyObjectTerraForm( const CVec2 &_p1, const CVec2 &_p2, const CVec2 &_p3, const CVec2 &_p4 );
	//
	void RestoreFromHistory();
	//
	void HideTerrain( bool bHide );
	//
	void UpdateTileAreaType(	const float fXo, const float fYo,
														const CArray2D<BYTE> &mask,
														const NTerraBrush::ETerraBrushUpdate terraBrushUpdate );
	//
	void GetTileTypeUpdateDifferences( float *pOffsX, float *pOffsY, CArray2D<BYTE> *pDiffs );
	//
	void FinalizeTexModifying();
	//
	void ClampUnderRivers( NMeshData::SMeshData *pData );
	//
	void GetAreaTileTypes( CArray2D<BYTE> *pAreaTypes, const int nX1, const int nY1, const int nX2, const int nY2 );
	void GetAreaHeights( CArray2D<float> *pAreaHeights, const int nX1, const int nY1, const int nX2, const int nY2 );
	//
	void UpdateAfterTilesModifying();
	//
	virtual float GetZ( float x, float y ) const;
	float GetTileHeight( int nX, int nY ) const;
	virtual void UpdateZ( CVec3 *pvPos );
	virtual DWORD GetNormal( const CVec2 &vPoint ) const;
	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual void InitHeights4Editor( int nSizeX, int nSizeY );
	//	
	void SetRainyWaters( CDBPtr<NDb::SWater> pRainyWater );
	//
	virtual int GetTilesCountX() const { return pTerraGen->GetTilesCountX(); }
	virtual int GetTilesCountY() const { return pTerraGen->GetTilesCountY(); }
	virtual const STerrainInfo* const GetTerraInfo() const { return pTerraGen->GetTerraInfo(); }
	//
	int operator&( IBinSaver &saver );
};


