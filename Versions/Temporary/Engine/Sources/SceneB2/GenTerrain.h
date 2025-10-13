#pragma once

#include "NoiseManager.h"
#include "TerraGen.h"
#include "TerrainAIInfo.h"
#include "TerrainInfo.h"
#include "TerrainGfxInfo.h"
#include "ExplosionsManager.h"
#include "WaterController.h"
#include "SurfController.h"
#include "DynamicDebrisManager.h"
#include "Misc/HPTimer.h"
#include "Stats_B2_M1/Vis2AI.h"

#include <cstdint>

#define DEF_CRAG_HOLE_WIDTH 0.05f
#define DEF_RIVER_DEPTH 4.0f

#define DEF_TERRA_HEIGHT 4.0f
#define DEF_TERRA_HEIGHT_AI Vis2AI( DEF_TERRA_HEIGHT )

#define DEF_DYNAMIC_DEBRIS_TEX_SIZE 128

#define TERRAIN_UPDATE_GFX	0x00000001
#define TERRAIN_UPDATE_AI		0x00000002
#define TERRAIN_UPDATE_ALL	TERRAIN_UPDATE_AI | TERRAIN_UPDATE_GFX

#define TIME_STAT_START( FuncName )															\
	//NHPTimer::STime tStart##FuncName;															\
	//NHPTimer::GetTime( &tStart##FuncName );												\
	//for ( int i = 0; i < g_nTimeStatLevel; ++i )									\
	//	OutputDebugString( "\t" );																	\
	//OutputDebugString( StrFmt("%s() started...\n", #FuncName) );	\
	//++g_nTimeStatLevel;																						\

#define TIME_STAT_FINISH( FuncName )																														\
	//--g_nTimeStatLevel;																																						\
	//const float fTimePassed##FuncName = NHPTimer::GetTimePassed( &tStart##FuncName );							\
	//for ( int i = 0; i < g_nTimeStatLevel; ++i )																									\
	//	OutputDebugString( "\t" );																																	\
	//OutputDebugString( StrFmt("%s() finished in: %f sec\n", #FuncName, fTimePassed##FuncName) );	\

extern int g_nTimeStatLevel;

//
//
//		class for terrain generation
//
//

class CTerrainManager;
class CTerraGen : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTerraGen )
	//
	const NDb::STerrain *pDesc;						// Terrain DB descriptor
	//
	CDGPtr<CFuncBase<STime> > pTimer;
	CObj<NGScene::IGameView> pGScene;
	CPtr<ITerraGfxObserver> pGfxObserver;
	CPtr<ITerraAIObserver> pAIObserver;
	//
	STerrainInfo terrainInfo;							// working terrain structure
	STerrainGfxInfo terrainGfxInfo;				// result vertices and indices
	STerrainAIInfo terrainAIInfo;					// result height map and terrain types
	CArray2D<CVec3> terrainNorms;					// normals for terrain	[N + 1]x[M + 1]
	//
	CWaterController waterController;
	CSurfController surfController;
	//
	CNoiseManager noiseManager;
	CExplosionsManager explosionsManager;
	CDynamicDebrisManager dynamicDebrisManager;
	//
	CArray2D<std::vector<int> > terraExplosionsHash;
	CArray2D<uint8_t> terraExplosionsHeights;
	//
	std::vector<int> needAddFoots;
	std::vector<int> updatedPrecipices;
	std::vector<int> updatedPrecNodes;

	struct SExplosionHistory
	{
		ZDATA
			CVec2 vMin;
		CVec2 vMax;
		CDBPtr<NDb::SMaterial> pMaterial;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&vMin); f.Add(3,&vMax); f.Add(4,&pMaterial); return 0; }
	};
	std::vector<SExplosionHistory> explosionsHistory;
	//
	struct SEntrenchmentHistory
	{
		ZDATA
		std::vector<CVec2> ctrlPoints;
		float fWidth;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&ctrlPoints); f.Add(3,&fWidth); return 0; }
	};
	std::vector<SEntrenchmentHistory> entrenchmentsHistory;
	//
	std::vector<CArray2D<uint8_t> > tileTerraMasks;
	CVec3 vPreLightDir;
	CVec2i vTexModMin, vTexModMax;
	CArray2D<uint8_t> texModCheck;

	CArray2D<uint8_t> needTexExportAfterGeomModifying;
	CVec2i vGeomModAreaMin, vGeomModAreaMax;

	std::vector<NWaterStuff::SWaterParams> waterParams;
	//
	struct STileOrder
	{
		int nPrevNum;
		int nPriority;
		//
		bool operator < ( const STileOrder &v ) const { return nPriority < v.nPriority; }
	};
	std::vector<STileOrder> tilesOrder;

	bool LoadTerrainInfo( CDataStream *pStream );
	void SaveTerrainInfo( CDataStream *pStream );
	//
	void OptimizeTerrainInfo();
	void RestoreTerrainInfoAfterOptimizing();

	inline float Tile2Vis( const int nGridValue ) const { return (float)nGridValue * DEF_TILE_SIZE; }
	inline int Vis2Tile( const float fVisValue ) const { return fVisValue / DEF_TILE_SIZE; }
	inline bool IsPointOnMap( const CVec2 &vPoint ) const
	{
		const float f1 = Tile2Vis(GetTilesCountX());
		const float f2 = Tile2Vis(GetTilesCountY());

		return ( (vPoint.x >= 0) && (vPoint.y >= 0) &&
						 (vPoint.x <= Tile2Vis(GetTilesCountX())) &&
						 (vPoint.y <= Tile2Vis(GetTilesCountY())) );
	}

	// HEIGHTS METHODS
	float GetTerraHeightWOWaters( const int nx, const int ny ) const;
	float GetTerraHeightWORivers( const int nx, const int ny ) const;
	float GetTerraHeight( const int nx, const int ny ) const;
	float GetFullTerraHeight( const int nx, const int ny ) const;
	float GetTerraHeight( const float x, const float y, const int nTileX, const int nTileY ) const;
	float GetTerraHeightNative( const float x, const float y ) const;
	float GetFullTerraHeight( const float x, const float y, const int nTileX, const int nTileY ) const;
	float GetFullTerraHeight( const float x, const float y ) const;
	//
	float GetUpperHeight( const STerrainInfo::SVSOPoint &point ) const;	// used for crags only
	float GetLowerHeight( const STerrainInfo::SVSOPoint &point ) const;	// used for crags only
	//
	float GetMaxCragHeight( const CVec2 &v, const int nExcludeID ) const;
	float GetMaxCragHeight2( const CVec2 &v, const int nExcludeID1, const int nExcludeID2 ) const;
	bool  GetMaxCragHeightEx( const CVec2 &v, float *pHeight ) const;
	//
	float GetMaxRiverHeight( const CVec2 &v ) const;
	//
	float GetTrackMidHeight( const float x, const float y, const float fMidX, const float fMidY, const int nTileX, const int nTileY );

	// TERRAINFO GEOMETRY + MODIFIERS
	void GenerateTerrain();
	void UpdateTileHeights( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 );
	void ResetTile( const int nTileX, const int nTileY );
	void ResetTerrainTiles( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 );
	void CreateNormals();
	void UpdateNormals( const int nX1, const int nY1, const int nX2, const int nY2 );
	CVec3 GetTerraNorm( const CVec3 &vPos ) const;
	CVec3 GetTerraNormFast( const float x, const float y, const int nTileX, const int nTileY ) const;
	CVec3 FindNormalInVertex( const CVec3 &vVert, const int nTileX, const int nTileY ) const;
	//
	void MakeHole(	const std::vector<CVec3fEx> &samples, const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2, const bool bLeftHeighten = true );
	void MakeHoleOnTile(	const std::vector<CVec3dEx> &samples, const int nTileX, const int nTileY, const bool bLeftHeighten );
	//
	void UpdateRiverHeights( STerrainInfo::SRiver *pRiver, SRiverGFXInfo *pGfxInfo, const NDb::SVSOInstance *pInstance );
	//
	void UpdateHeightsAfterCrags( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 );
	void UpdateHeightsAfterRivers( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 );
	//
	void UpdateArea( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2, uint32_t dwUpdateFlags );
	void ReCreateAllWaterZones();
	//
	void UpdateVectorAreaInfo( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2, uint32_t dwUpdateFlags );
	void UpdateAllObjectsInArea( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 );

	// TILE MASKS
	void InitTileMasks();
	void UpdateTileMasks( const int nX1, const int nY1, const int nX2, const int nY2, const bool bNeedGeomUpdate );

	// GFX GEOMETRY
	void PutAllFeaturesToGfx();
	void UpdateGfxInfo( const int nPatchX1, const int nPatchY1, const int nPatchX2, const int nPatchY2 );
	void UpdateGfxInfo( const int nPatchX, const int nPatchY );
	void UpdateAllGfxInfo();
	void ReCreateAllFeaturesGfx();
	void PutAllGfxToObserver();
	void UpdateTerraBorders();
	//
	void CalcTerraVertexData( NGScene::SVertex *pVert, const CVec3 &vPos, const int nTileX, const int nTileY, const int nPatchX, const int nPatchY, const CVec3 *pRealPos, const CVec3 *pRealNorm, const bool bNeedFaster ) const;
	int AddUniqueVertex(	NMeshData::SMeshData *pMeshData, NMeshData::SMeshDataTex2 *pTexData, const CVec2 &vSecTex, const uint8_t cAlpha, NGScene::SVertex *pVert, const bool bNeedFaster ) const;
	void AddTileTriangle( std::vector<NMeshData::SMeshData> *pMeshData, std::vector<NMeshData::SMeshDataTex2> *pTexData, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec2 &vSecTex1, const CVec2 &vSecTex2, const CVec2 &vSecTex3, const int nTileX, const int nTileY, const int nPatchX, const int nPatchY, const CVec3 *pRealPos1, const CVec3 *pRealPos2, const CVec3 *pRealPos3, const CVec3 *pRealNorm1, const CVec3 *pRealNorm2, const CVec3 *pRealNorm3,	const bool bNeedFaster ) const;
	//
	void UpdateAlphaByPosition( NGScene::SVertex *pVertex );
	void RemoveInvisibleTriangles( NMeshData::SMeshData *pPatch );
	//void ClampRiverGfxByMap( SRiverGFXInfo *pRiverGfx );

	// AI CONNECTION
	void RegenerateAIInfo();
	void UpdateAIInfo( const int nX1, const int nY1, const int nX2, const int nY2 ) const;
	void UpdateAllAIInfo();
	void PutCragToAI( const NDb::SVSOInstance *pCragInstance ) const;
	void PutAllCragsToAI() const;
	void PutRiverToAI( const NDb::SVSOInstance *pRiverInstance ) const;
	void PutRiverCliffsToAI( const STerrainInfo::SRiver &river ) const;
	void PutAllRiversToAI() const;
	void PutRoadToAI( const NDb::SVSOInstance *pRoadInstance ) const;
	void PutAllRoadsToAI() const;
	void PutAllFeaturesToAI() const;
	void UpdateAITerraTypes( const bool bForceUpdateAll );
	bool IsPointOnBridge( float x, float y ) const;
	void SetTerraTypesToAI( const std::vector<NDb::STerrainAIProperties> &params ) const;
	void PutAllWaterToAI() const;

	// TERRAINFO VSOs
	void CollectAllCragsAndRiversInArea( std::vector<int> *pColCrags, std::vector<int> *pColRivers, const CVec2i &vBBMin, const CVec2i &vBBMax, const int nExcludeCragID, const int nExcludeRiverID );
	void UpdateAllOnTerrainObjectsInArea( const CVec2i &vMin, const CVec2i &vMax );
	void UpdateCragsAndRiversInArea( const std::vector<int> &updCrags, const std::vector<int> &updRivers, const CVec2i &vBBMin, const CVec2i &vBBMax );
	//
	void ConvertVSOPointsFromAIToVisAndPutOnTerrain( std::vector<NDb::SVSOPoint> *pDstPoints, const std::vector<NDb::SVSOPoint> &srcPoints );

	// CRAGS
	const NDb::SVSOInstance* FindCrag( int nID ) const;
	STerrainInfo::SCrag* FindCragInfo( int nID );
	bool AddCrag( const NDb::SVSOInstance *pInstance, const int nRandSeed );
	void AddAllCrags();
	void PutCragOnTerrain( STerrainInfo::SCrag *pCrag, const CVec2i &vMinTile, const CVec2i &vMaxTile );
	void PutAllCragsOnTerrain();
	void RemoveCragInfo( const int nVSOID );
	void CragManipulator( const STerrainInfo::SCrag *pCrag, const bool bRemove );
	void CragsPushPointLR( std::vector<CVec3fEx> *pLeft, std::vector<CVec3fEx> *pRight, const STerrainInfo::SVSOPoint &point, CVec3 *pVMin, CVec3 *pVMax, const int nID );
	void CragsPushExactPoint( std::vector<STerrainInfo::SVSOPoint> *pArray, const STerrainInfo::SVSOPoint &p1, const STerrainInfo::SVSOPoint &p2, const float t1, const float t2, const int nID, const bool bStart );

	// RIVERS
	const NDb::SVSOInstance* FindRiver( int nID ) const;
	STerrainInfo::SRiver* FindRiverInfo( int nID );
	SRiverGFXInfo *FindRiverGfxInfo( const int nID );
	bool AddRiver( const NDb::SVSOInstance *pInstance, const int nRandSeed );
	void AddAllRivers();
	void PutRiverOnTerrain( STerrainInfo::SRiver *pRiver, const CVec2i &vMinTile, const CVec2i &vMaxTile );
	void PutAllRiversOnTerrain();
	void RemoveRiverInfo( const int nVSOID );
	void RemoveRiverGfxInfo( const int nVSOID );
	void CreateRiverGfx( STerrainInfo::SRiver *pRiver, const NDb::SVSOInstance *pInstance, const bool bNeedUpdateHeights );
	void RiverManipulator( STerrainInfo::SRiver *pRiver, const bool bRemove );
	//
	bool IsPointInsideRivers( const CVec3 &v, const int nExcludeID );
	
	// ROADS
	const NDb::SVSOInstance* FindRoad( int nID ) const;
	void AddRoad( NMeshData::SMeshData *pPatch, const std::vector<NGScene::SVertex> &verts, const std::vector<STriangle> &trgs );
	void AddAllRoads();
	void RemoveAllRoads();
	void ProjectQuadOnTerrain(	const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec3 &v4, const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3, const CVec2 &vTex4, const float fAlpha1, const float fAlpha2, const float fAlpha3, const float fAlpha4, NMeshData::SMeshData *pData );
	void ProjectTrgOnTerrain( const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3, const float fAlpha1, const float fAlpha2, const float fAlpha3, NMeshData::SMeshData *pData );
	int CalcTexAndAddRoadVertex(	const CVec3 &vert, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec3 &v4, const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3, const CVec2 &vTex4, const float fAlpha1, const float fAlpha2, const float fAlpha3, const float fAlpha4, const int nTileX, const int nTileY, std::vector<NGScene::SVertex> *pVerts );
	void ProjectRoadLayer(	const NDb::SVSOPoint &point1, const NDb::SVSOPoint &point2, const float fMinTexX, const float fMaxTexX, const float fPrevDist, const float fNextTexY, const float fTexY, NMeshData::SMeshData *pCurPatch, const float fOrgTexX, const float fTexXD );
	void CreateRoadGfx( const NDb::SVSOInstance *pInstance, const std::vector<NDb::SVSOPoint> &points );

	// PEAKS
	void AddPeak( const STerrainInfo::SPeak &peak );
	void AddAllPeaks();
	void RemovePeakInfo( const int nVSOID );
	void RemovePeakGfxInfo( const int nVSOID );
	void UpdateNeededPeaks();
	void PeaksProjectTrgOnTiles( std::vector<NMeshData::SMeshDataTex2> *pDataArr, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec2 &vSecTex1, const CVec2 &vSecTex2, const CVec2 &vSecTex3, const CVec3 &vRealPos1, const CVec3 &vRealPos2, const CVec3 &vRealPos3, const CVec3 &vNorm1, const CVec3 &vNorm2, const CVec3 &vNorm3, const CVec3 &vBase1, const CVec3 &vBase2, const CVec3 &vBase3 );
	void PeaksGetArrayOfFirstPoints( std::vector<SIntersectPoint> *firstArrPos, const CVec3 &v1, const CVec3 &v2 );
	
	// FOOTS
	void AddFoot( const STerrainInfo::SFoot &foot );
	void RemoveFoot( const int nVSOID );
	void RemoveFootInfo( const int nVSOID );
	void RemoveFootGfxInfo( const int nVSOID );
	void AddAllNeededFoots();

	// PRECIPICES
	STerrainInfo::SPrecipice* FindPrecipice( int nID );
	void CreateVerticesInPrecipiceNode( STerrainInfo::SPrecipiceNode *pNode, const int nNodeInd );
	void CreatePrecipiceMesh( STerrainInfo::SPrecipice *pCurPrec, const bool bNeedRiversClamping );
	void RemovePrecipiceFromCollector( const int nID, const bool bFast );
	CVec3 GetSmoothPrecipiceNorm( const CVec2 &vPos );
	void AddPrecipiceToCollector( const int nID, const std::vector<CVec3> &posArr, const std::vector<float> &heightsArr, const std::vector<CVec3> &normsArr, const NDb::SMaterial *pMaterial, const float fTexGeomScale, const uint8_t bStayedOnTerrain, const int nExcludeID, const NDb::SMaterial *pFootMaterial, const float fDepth, const float fDepthRand, const float fRandX, const float fRandY, const bool bHasPeak );
	//
	void CheckPrecipiceIntersectionWithRivers( STerrainInfo::SPrecipice *pPrec );
	void AddToPrecipiceUpdateQueue( const int nID );
	void UpdateAllNeededPrecipices();
	void RemapPrecipices();
	//
	int PrecAddUniqueNode( const CVec2 &vPos, const CVec3 &vNorm, const float fMin, const float fMax, const int nID, const float fDepth, const float fDepthRand, const float fRandX, const float fRandY );
	int PrecAddUniqueNode2( const CVec2 &vPos, const CVec3 &vNorm1, const CVec3 &vNorm2, const float fMin, const float fMax, const int nID1, const int nID2, const float fDepth1, const float fDepth2, const float fDepthRand1, const float fDepthRand2, const float fRandX1, const float fRandX2, const float fRandY1, const float fRandY2 );

	// SPOTS
	const NDb::STerrainSpotInstance* FindTerraSpot( int nID ) const;
	void AddAllTerraSpots();
	void RemoveAllTerraSpots();
	void CreateTerraSpotGfx( STerrainInfo::STerraSpot *pSpot, const NDb::STerrainSpotInstance *pInstance );

	// EXPLOSIONS
	void CreateExplosionData( NMeshData::SMeshData *pData, const CVec2 &vMin, const CVec2 &vMax, const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2, const int nIncHeight );


public:
	CTerraGen();
	virtual ~CTerraGen() {}

	void SetGfxObserver( ITerraGfxObserver *_pGfxObserver );
	void SetAIObserver( ITerraAIObserver *_pAIObserver );

	ITerraAIObserver* GetAIObserver();

	void AttachGameView( NGScene::IGameView *_pGScene );
	void AttachTimer( CFuncBase<STime> *_pTimer );
	//
	void Load( const NDb::STerrain *_pDesc, CDataStream *pStream );
	void Save( CDataStream *pStream );
	void ReGenerate();

	// HEIGHTS
	float GetTerraHeight( const float x, const float y ) const;
	float GetRealTerraHeight( const float x, const float y ) const;
	float GetTerraHeightFast( const int nTileX, const int nTileY ) const;
	float GetRealTerraHeightFast( const int nTileX, const int nTileY ) const;

	// MODIFIERS
	void ModifyTerraGeometryByBrush( const int nVisTileX, const int nVisTileY, bool bCenter, const CArray2D<float> &brush, const NTerraBrush::ETerraBrushUpdate terraBrushUpdate );
	void GetTerraGeometryUpdateDifferences( int *pOffsX, int *pOffsY, CArray2D<float> *pDiffs );
	void UpdateAllObjectsInGeomModifyingArea();
	void UpdateRiversDepthes();
	//
	void UpdateTileAreaType( const float fXo, const float fYo, const CArray2D<uint8_t> &mask, const NTerraBrush::ETerraBrushUpdate terraBrushUpdate );
	void GetTileTypeUpdateDifferences( float *pOffsX, float *pOffsY, CArray2D<uint8_t> *pDiffs );
	void FinalizeTexModifying();
	void UpdateAfterTilesModifying();
	//
	void GetAreaTileTypes( CArray2D<uint8_t> *pAreaTypes, const int nX1, const int nY1, const int nX2, const int nY2 );
	void GetAreaHeights( CArray2D<float> *pAreaHeights, const int nX1, const int nY1, const int nX2, const int nY2 );

	// ROADS
	void AddRoad( const NDb::SVSOInstance *pInstance );
	void UpdateRoad( const int nVSOID );
	void RemoveRoad( const int nVSOID );

	// CRAGS
	void AddCrag( const NDb::SVSOInstance *pInstance );
	void UpdateCrag( const int nVSOID );
	void RemoveCrag( const int nVSOID );
	bool GetCragPrecVerts( std::vector<CVec3> *pVerts, int nVSOId );

	// RIVERS
	void AddRiver( const NDb::SVSOInstance *pInstance );
	void UpdateRiver( const int nVSOID );
	void RemoveRiver( const int nVSOID );

	// TERRA SPOTS
	void AddTerraSpot( const NDb::STerrainSpotInstance *pInstance );
	void UpdateTerraSpot( const int nSpotID );
	void RemoveTerraSpot( const int nSpotID );

	// OTHERS
	void AddEntrenchment( const std::vector<CVec2> &_ctrlPoints, const float _fWidth, const bool bWriteHistory = true );
	void AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial, const bool bWriteHistory = true );
	void AddDynamicDebris( const CVec2 &vPos, const CVec2 &vSize, const float fAngle, const int nSmoothRad, const NDb::SMaterial *pMaterial );
	void AddTrack( const int nID, const float fFadingSpeed, const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4, const CVec2 &vNorm, const float _fWidth, const float fAplha, CTracksManager *pTracksManager );
	void CreateDebris(	const std::string &szFileName, CArray2D<uint8_t> *pImage, CVec2 *pOrigin, const NDebrisBuilder::EMaskType maskType, const int nSmoothRadius, const NDebrisBuilder::EMaskSmoothType smoothType );

	// AI
  void UpdateAIInfo();

	// WATER
	void InitWater();
	void InitRainyWater( CDBPtr<NDb::SWater> pWater );
	void UpdateWater();
	void ClampUnderRivers( NMeshData::SMeshData *pData );

	// BRIDGES
	void ApplyBridgeTerraForm( const CVec2 &_p1, const CVec2 &_p2, const float fWidth, const float fHeight );
	void ApplyObjectTerraForm( const CVec2 &_p1, const CVec2 &_p2, const CVec2 &_p3, const CVec2 &_p4 );
	//
	void RestoreFromHistory();
	void CreateSelection( NMeshData::SMeshData *pData, const CVec3 &vMin, const CVec3 &vMax, CTerrainManager *pManager );

	//
	int GetNumPatchesX() const { return terrainInfo.nNumPatchesX; }
	int GetNumPatchesY() const { return terrainInfo.nNumPatchesY; }
	//
	int GetTilesCountX() const { return terrainInfo.tiles.GetSizeX(); }
	int GetTilesCountY() const { return terrainInfo.tiles.GetSizeY(); }

	const STerrainInfo* const GetTerraInfo() const { return &terrainInfo; }

	int operator&( IBinSaver &saver );
};


