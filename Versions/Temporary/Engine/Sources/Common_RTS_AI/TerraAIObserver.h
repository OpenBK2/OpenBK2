#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "AIMap.h"
#include "B2_M1_Terrain/DBVSO.h"
#include "B2_M1_Terrain/fmtVSO.h"
#include "PassMarkers.h"
#include "StaticMapHeights.h"
#include "Stats_B2_M1/TerraAIObserver.h"
#include "Terrain.h"

#include "Common_RTS_AI_export.h"

#include <cstdint>

struct SSingleSide
{
	bool bSingleSide = false;
	float fOffset1 = 0.0f;
	float fOffset2 = 0.0f;

	SSingleSide()
		: bSingleSide( false )
	{
	}

	SSingleSide( const bool _bSingleSide, const float _fOffset1, const float _fOffset2 )
		: bSingleSide( _bSingleSide )
		, fOffset1( _fOffset1 )
		, fOffset2( _fOffset2 )
	{
	}
};

//
//
//	Common TerraAIObserver class for game and editor
//
//

class COMMON_RTS_AI_EXPORT CTerraAIObserver : public ITerraAIObserver
{
	OBJECT_NOCOPY_METHODS( CTerraAIObserver )

protected:
	ZDATA
	CObj<CAIMap> pAIMap;
	CObj<CTerrain> pTerrain;
	CObj<CStaticMapHeights> pHeights;

	CObj<CPassMarkersDraw> pMarkers;
	bool bShowPassability = false;

public:
	ZEND int operator&( IBinSaver &f ) override;

	virtual void AddVSO( const NDb::SVSOInstance *pVSO );
	virtual void GetTilesUnderVSO( const NDb::SVSOInstance *pVSO,
	                               const int nPoint,
	                               const float fCoeff,
	                               std::list<SVector> *pTiles,
	                               const SSingleSide &singleSide,
	                               bool bInverse = false );

	void InitPassMarkers( CAIMap *_pAIMap );
	void ClearPassMarkers();
	void LockSteepTiles();

public:
	CTerraAIObserver();

	~CTerraAIObserver() override {}

	void AddCrag( const NDb::SVSOInstance *pInstance ) override;
	void AddRoad( const NDb::SVSOInstance *pInstance ) override;
	void AddRiver( const NDb::SVSOInstance *pInstance ) override;

	void AddWaterLine( const NDb::SVSOInstance *pInstance, const bool bIsLake ) override;
	void ClearRiverCliffTiles() override;
	void AddRiverCliffTiles( const std::list<SVector> &tiles ) override;
	//
	void UpdateZ( CVec3 *pvPos ) override;
	void SetTerraTypes( const std::vector<NDb::STerrainAIProperties> &params ) override;
	void
	UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<uint8_t> &types ) override;
	void
	UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &heights ) override;

	float GetZ( float x, float y ) const override;
	float GetTileHeight( int nX, int nY ) const override;
	uint32_t GetNormal( const CVec2 &vPoint ) const override;

	bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const override;
	bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const override;
	void InitHeights4Editor( int nSizeX, int nSizeY ) override;

	void FinalizeUpdates() override;

	void DrawPassabilities() const override;
	void ToggleShowPassability() override;
	void SetPassMarkers( const int color, const int aiClass, const int freeClass, const int nBoundTileRadius ) override;
	void DumpMaxes( const std::string &szFileName, const int aiClass ) override;

	bool IsPassabilityOn() const override
	{
		return bShowPassability;
	}

	bool IsBridge( const int nX, const int nY ) const override;
};
