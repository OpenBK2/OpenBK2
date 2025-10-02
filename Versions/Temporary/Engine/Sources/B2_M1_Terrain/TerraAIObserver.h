#pragma once
//#pragma once
//
//#include "../B2_M1_World/TerraAIObserver.h"
//#include "../Common_RTS_AI/StaticMapHeights.h"
//#include "../Common_RTS_AI/AIMap.h"
//#include "../Common_RTS_AI/Terrain.h"
//#include "../B2_M1_World/fmtVSO.h"
//#include "../b2_m1_terrain/dbvso.h"

//struct SSingleSide
//{
//	bool bSingleSide;
//	float fOffset1;
//	float fOffset2;
//
//	SSingleSide() : bSingleSide( false ) {}
//	SSingleSide( const bool _bSingleSide, const float _fOffset1, const float _fOffset2 )
//		: bSingleSide( _bSingleSide ), fOffset1( _fOffset1 ), fOffset2( _fOffset2 ) 
//	{ }
//};



////	Common TerraAIObserver class for game and editor



//class CTerraAIObserver :	public ITerraAIObserver
//{
//protected:
//	CObj<CAIMap> pAIMap;
//	CObj<CTerrain> pTerrain;
//	CObj<CStaticMapHeights> pHeights;
//
//	void AddVSO( const NDb::SVSOInstance *pVSO );
//	void GetTilesUnderVSO( const NDb::SVSOInstance *pVSO, const int j, const float fCoeff, list<SVector> *pTiles,
//												 const SSingleSide &singleSide, bool bInverse = false );
//
//public:
//	void AddCrag( const NDb::SVSOInstance *pInstance );
//	void AddRoad( const NDb::SVSOInstance *pInstance );
//	void AddRiver( const NDb::SVSOInstance *pInstance );
//
//	void AddWaterLine( const NDb::SVSOInstance *pInstance );
//	//
//	virtual void UpdateZ( CVec3 *pvPos );
//	void SetTerraTypes( const vector<NDb::STerrainAIProperties> &params );
//	void UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<BYTE> &types );
//
//	virtual float GetZ( float x, float y ) const;
//	virtual float GetTileHeight( int nX, int nY ) const;
//	virtual DWORD GetNormal( const CVec2 &vPoint ) const;
//
//	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
//	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
//	virtual void InitHeights4Editor( int nSizeX, int nSizeY );
//
//};


