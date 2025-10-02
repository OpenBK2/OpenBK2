#pragma once

#include "Common_RTS_AI_export.h"

#include "../Stats_B2_M1/TerraAIObserver.h"
#include "../B2_M1_Terrain/fmtvso.h"
#include "../b2_m1_terrain/dbvso.h"
#include "StaticMapHeights.h"
#include "AIMap.h"
#include "Terrain.h"
#include "PassMarkers.h"

struct SSingleSide
{
	bool bSingleSide;
	float fOffset1;
	float fOffset2;

	SSingleSide() : bSingleSide( false ) {}
	SSingleSide( const bool _bSingleSide, const float _fOffset1, const float _fOffset2 )
		: bSingleSide( _bSingleSide ), fOffset1( _fOffset1 ), fOffset2( _fOffset2 ) 
	{ }
};

//
//
//	Common TerraAIObserver class for game and editor
//
//

class COMMON_RTS_AI_EXPORT CTerraAIObserver :	public ITerraAIObserver
{
	OBJECT_NOCOPY_METHODS( CTerraAIObserver )

protected:
	ZDATA
	CObj<CAIMap> pAIMap;
	CObj<CTerrain> pTerrain;
	CObj<CStaticMapHeights> pHeights;

	CObj<CPassMarkersDraw> pMarkers;
	bool bShowPassability;
public:
	ZEND int operator&( IBinSaver &f );

	virtual void AddVSO( const NDb::SVSOInstance *pVSO );
	virtual void GetTilesUnderVSO( const NDb::SVSOInstance *pVSO, const int j, const float fCoeff, list<SVector> *pTiles,
												 const SSingleSide &singleSide, bool bInverse = false );

	void InitPassMarkers( CAIMap *_pAIMap );
	void ClearPassMarkers();
	void LockSteepTiles();

public:
	CTerraAIObserver();
	virtual ~CTerraAIObserver() {}

	virtual void AddCrag( const NDb::SVSOInstance *pInstance );
	virtual void AddRoad( const NDb::SVSOInstance *pInstance );
	virtual void AddRiver( const NDb::SVSOInstance *pInstance );

	virtual void AddWaterLine( const NDb::SVSOInstance *pInstance, const bool bIsLake );
	//
	virtual void UpdateZ( CVec3 *pvPos );
	virtual void SetTerraTypes( const vector<NDb::STerrainAIProperties> &params );
	virtual void UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<BYTE> &types );
	virtual void UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &heights );

	virtual float GetZ( float x, float y ) const;
	virtual float GetTileHeight( int nX, int nY ) const;
	virtual DWORD GetNormal( const CVec2 &vPoint ) const;

	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual void InitHeights4Editor( int nSizeX, int nSizeY );

	virtual void FinalizeUpdates();

	virtual void DrawPassabilities() const;
	virtual void ToggleShowPassability();
	virtual void SetPassMarkers( const int color, const int aiClass, const int freeClass, const int nBoundTileRadius );
	virtual void DumpMaxes( const string &szFileName, const int aiClass );

	virtual bool IsPassabilityOn() const { return bShowPassability; }
	virtual bool IsBridge( const int nX, const int nY ) const;
};

