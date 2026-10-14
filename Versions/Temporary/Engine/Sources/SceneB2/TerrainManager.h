#pragma once

#include "GenTerrain.h"
#include "Terrain.h"

#include <cstdint>

class CTerrainManager : public ITerraManager
{
	OBJECT_NOCOPY_METHODS( CTerrainManager )
	//
	CObj<CTerraGen> pTerraGen;
	CObj<CSceneTerrain> pTerrain;
	//
	CDBPtr<NDb::STerrain> pDesc;
	CPtr<NGScene::IGameView> pGameView;
	CDGPtr<CFuncBase<STime>> pTimer;

	//
	void InitTerragen()
	{
		pTerraGen = new CTerraGen();
		if ( pGameView )
		{
			pTerraGen->AttachGameView( pGameView );
		}
		if ( pTimer )
		{
			pTerraGen->AttachTimer( pTimer );
		}
	}

	//
public:
	CTerrainManager() {}

	//
	void Setup( NGScene::IGameView *pGView, CFuncBase<STime> *pTimer );
	void Load( const NDb::STerrain *pDesc, CDataStream *pStream ) override;

	const NDb::STerrain *GetDesc() const override
	{
		return pDesc;
	}

	void SetAIObserver( struct ITerraAIObserver *pObserver ) override;
	struct ITerraAIObserver *GetAIObserver() override;

	void UpdateAIInfo() override
	{
		pTerraGen->UpdateAIInfo();
	}

	//
	void ReGenerate() override
	{
		NI_ASSERT( pDesc != 0, "Load terrain desc first!" );
		pTerraGen->ReGenerate();
	}

	//
	void Save( CDataStream *pStream ) override;
	// road
	void AddRoad( const NDb::SVSOInstance *pInstance ) override;
	void UpdateRoad( const int nVSOID ) override;
	void RemoveRoad( const int nVSOID ) override;
	// crag
	void AddCrag( const NDb::SVSOInstance *pInstance ) override;
	void UpdateCrag( const int nVSOID ) override;
	void RemoveCrag( const int nVSOID ) override;
	// river
	void AddRiver( const NDb::SVSOInstance *pInstance ) override;
	void UpdateRiver( const int nVSOID ) override;
	void RemoveRiver( const int nVSOID ) override;
	// terra spot
	void AddTerraSpot( const NDb::STerrainSpotInstance *pInstance ) override;
	void UpdateTerraSpot( const int nSpotID ) override;
	void RemoveTerraSpot( const int nSpotID ) override;
	//
	void AddEntrenchment( const std::vector<CVec2> &_ctrlPoints, const float _fWidth ) override;
	//
	void CreateDebris( const std::string &szFileName,
	                   CArray2D<uint8_t> *pImage,
	                   CVec2 *pOrigin,
	                   const NDebrisBuilder::EMaskType maskType,
	                   const int nSmoothRadius,
	                   const NDebrisBuilder::EMaskSmoothType smoothType ) override;
	// selection
	void CreateSelection( NMeshData::SMeshData *pData, const CVec3 &vMin, const CVec3 &vMax );
	// tracks
	void AddTrack( const int nID,
	               const float fFadingSpeed,
	               const CVec2 &_v1,
	               const CVec2 &_v2,
	               const CVec2 &_v3,
	               const CVec2 &_v4,
	               const CVec2 &vNorm,
	               const float _fWidth,
	               const float fAplha,
	               CTracksManager *pTracksManager );
	// explosions
	void AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial );
	//
	void ModifyTerraGeometryByBrush( const int nVisTileX,
	                                 const int nVisTileY,
	                                 bool bCenter,
	                                 const CArray2D<float> &brush,
	                                 const NTerraBrush::ETerraBrushUpdate terraBrushUpdate ) override;
	//
	void GetTerraGeometryUpdateDifferences( int *pOffsX, int *pOffsY, CArray2D<float> *pDiffs ) override;
	void UpdateAllObjectsInGeomModifyingArea() override;
	void UpdateWater() override;
	float GetTerraHeight( const float x, const float y ) const override;
	float GetRealTerraHeight( const float x, const float y ) const override;
	float GetTerraHeightFast( const int nTileX, const int nTileY ) const override;
	float GetRealTerraHeightFast( const int nTileX, const int nTileY ) const override;

	//
	bool GetCragPrecVerts( std::vector<CVec3> *pVerts, int nVSOId ) override
	{
		return pTerraGen->GetCragPrecVerts( pVerts, nVSOId );
	}

	//
	void UpdateRiversDepthes() override;
	//
	void AddDynamicDebris( const CVec2 &vPos,
	                       const CVec2 &vSize,
	                       const float fAngle,
	                       const int nSmoothRad,
	                       const NDb::SMaterial *pMaterial ) override;
	void ApplyBridgeTerraForm( const CVec2 &_p1, const CVec2 &_p2, const float fWidth, const float fHeight ) override;
	void ApplyObjectTerraForm( const CVec2 &_p1, const CVec2 &_p2, const CVec2 &_p3, const CVec2 &_p4 ) override;
	//
	void RestoreFromHistory();
	//
	void HideTerrain( bool bHide );
	//
	void UpdateTileAreaType( const float fXo,
	                         const float fYo,
	                         const CArray2D<uint8_t> &mask,
	                         const NTerraBrush::ETerraBrushUpdate terraBrushUpdate ) override;
	//
	void GetTileTypeUpdateDifferences( float *pOffsX, float *pOffsY, CArray2D<uint8_t> *pDiffs ) override;
	//
	void FinalizeTexModifying() override;
	//
	void ClampUnderRivers( NMeshData::SMeshData *pData ) override;
	//
	void GetAreaTileTypes(
			CArray2D<uint8_t> *pAreaTypes, const int nX1, const int nY1, const int nX2, const int nY2 ) override;
	void
	GetAreaHeights( CArray2D<float> *pAreaHeights, const int nX1, const int nY1, const int nX2, const int nY2 ) override;
	//
	void UpdateAfterTilesModifying() override;
	//
	float GetZ( float x, float y ) const override;
	float GetTileHeight( int nX, int nY ) const override;
	void UpdateZ( CVec3 *pvPos ) override;
	uint32_t GetNormal( const CVec2 &vPoint ) const override;
	bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const override;
	bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const override;
	void InitHeights4Editor( int nSizeX, int nSizeY ) override;
	//
	void SetRainyWaters( CDBPtr<NDb::SWater> pRainyWater );

	//
	int GetTilesCountX() const override
	{
		return pTerraGen->GetTilesCountX();
	}

	int GetTilesCountY() const override
	{
		return pTerraGen->GetTilesCountY();
	}

	const STerrainInfo *const GetTerraInfo() const override
	{
		return pTerraGen->GetTerraInfo();
	}

	//
	int operator&( IBinSaver &saver ) override;
};
