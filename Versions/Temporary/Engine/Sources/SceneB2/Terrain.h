#pragma once

#include "../System/DG.h"
#include "../B2_M1_Terrain/PatchHolder.h"
#include "TerraGen.h"

using namespace NMeshData;

namespace NGScene
{
	class IGameView;
	class IMaterial;
}

namespace NDb
{
	struct SWater;
}

// precipices

class CMeshDataPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	NMeshData::SMeshData *pInfo;
protected:
	void Recalc();
public:
	CMeshDataPatch() {}
	CMeshDataPatch( NMeshData::SMeshData *_pInfo ): pInfo( _pInfo ) {}
};
typedef SPatchHolder<CMeshDataPatch> SPrecipicePatchHolder;

class CPrecipice
{
	SPrecipiceGFXInfo *pInfo; // graphics data (meshes for patches)
	vector<SPrecipicePatchHolder> patches; // engine's ready graphics data
	int nID;
public:
	CPrecipice(): pInfo( 0 ) {}
	CPrecipice( SPrecipiceGFXInfo *_pInfo, const int _nID ): pInfo( _pInfo ), nID( _nID ) {}
	//
	void CreatePatches( NGScene::IGameView *pGView );
	int GetID() const { return nID; }
	//
	void UpdatePrecipiceGfxInfoPtr( SPrecipiceGFXInfo *_pInfo ) { pInfo = _pInfo; }
};

// peaks

class CPeakPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	NMeshData::SMeshDataTex2 *pInfo;
protected:
	void Recalc();
public:
	CPeakPatch() {}
	CPeakPatch( NMeshData::SMeshDataTex2 *_pInfo ): pInfo( _pInfo ) {}
};
typedef SPatchHolder<CPeakPatch> SPeakPatchHolder;

class CPeak
{
	SPeakGFXInfo *pInfo;						// graphics data (meshes for patches)
	vector<vector<SPeakPatchHolder> > patches;			// engine's ready graphics data
	int nID;
public:
	CPeak(): pInfo( 0 ) {}
	CPeak( SPeakGFXInfo *_pInfo, const int _nID ): pInfo( _pInfo ), nID( _nID ) {}
	//
	void CreatePatches( NGScene::IGameView *pGView, const NDb::STerrain *pTerraDesc );
	int GetID() const { return nID; }
	//
	void UpdatePeakGfxInfoPtr( SPeakGFXInfo *_pInfo ) { pInfo = _pInfo; }
};

// foots

typedef SPatchHolder<CMeshDataPatch> SFootPatchHolder;

class CFoot
{
	SFootGFXInfo *pInfo;						// graphics data (meshes for patches)
	vector<SFootPatchHolder> patches;			// engine's ready graphics data
	int nID;
public:
	CFoot(): pInfo( 0 ) {}
	CFoot( SFootGFXInfo *_pInfo, const int _nID ): pInfo( _pInfo ), nID( _nID ) {}
	//
	void CreatePatches( NGScene::IGameView *pGView, const NDb::STerrain *pTerraDesc );
	int GetID() const { return nID; }
	//
	void UpdateFootGfxInfoPtr( SFootGFXInfo *_pInfo ) { pInfo = _pInfo; }
};

// river

class CRiverPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	const NMeshData::SMeshData *pInfo;
	CDGPtr< CFuncBase<STime> > pTimer;
	float fStreamSpeed;
protected:
	bool NeedUpdate() { return pTimer == 0 ? false : pTimer.Refresh(); }
	void Recalc();
public:
	CRiverPatch(): pInfo( 0 ), fStreamSpeed( 0 ) {}
	CRiverPatch( const NMeshData::SMeshData *_pInfo, float _fStreamSpeed, CFuncBase<STime> *_pTimer )
		: pInfo( _pInfo ), fStreamSpeed( _fStreamSpeed ), pTimer( _pTimer ) {}
};
typedef SPatchHolder<CRiverPatch> SRiverPatchHolder;

class CRiver
{
	const SRiverGFXInfo *pInfo;						// graphics data (meshes for patches)
	int nID;
	vector<SRiverPatchHolder> waterPatches;
	vector<SRiverPatchHolder> water2Patches;
	vector<SRiverPatchHolder> bottomPatches;
	CDGPtr< CFuncBase<STime> > pTimer;
public:
	CRiver(): pInfo( 0 ) {}
	CRiver( const SRiverGFXInfo *_pInfo, const int _nID, CFuncBase<STime> *_pTimer )
		: pInfo( _pInfo ), nID( _nID ), pTimer( _pTimer ) {}
	//
	void CreatePatches( NGScene::IGameView *pGView );
	int GetID() const { return nID; }
	//
	void UpdateRiverGfxInfoPtr( const SRiverGFXInfo *_pInfo ) { pInfo = _pInfo; }
};

// road

typedef SPatchHolder<CMeshDataPatch> SRoadPatchHolder;

class CRoad
{
	SRoadGFXInfo *pInfo;						// graphics data (meshes for patches)
	vector<SRoadPatchHolder> patches;			// engine's ready graphics data
	int nID;
public:
	CRoad(): pInfo( 0 ) {}
	CRoad( SRoadGFXInfo *_pInfo, const int _nID ): pInfo( _pInfo ), nID( _nID ) {}
	//
	void CreatePatches( NGScene::IGameView *pGView );
	int GetID() const { return nID; }
	//
	void UpdateRoadGfxInfoPtr( SRoadGFXInfo *_pInfo ) { pInfo = _pInfo; }
};

// terraspot

typedef SPatchHolder<CMeshDataPatch> STerraSpotPatchHolder;

class CTerraSpot
{
	STerraSpotGFXInfo *pInfo;						// graphics data (meshes for patches)
	STerraSpotPatchHolder patch;			// engine's ready graphics data
	int nID;
public:
	CTerraSpot(): pInfo( 0 ) {}
	CTerraSpot( STerraSpotGFXInfo *_pInfo, const int _nID ): pInfo( _pInfo ), nID( _nID ) {}
	//
	void CreatePatches( NGScene::IGameView *pGView );
	int GetID() const { return nID; }
	//
	void UpdateTerraSpotGfxInfoPtr( STerraSpotGFXInfo *_pInfo ) { pInfo = _pInfo; }
};

// terrain

typedef SPatchHolder<CMeshDataPatch> STerrainPatchHolder;

class CSceneTerrain : public ITerraGfxObserver
{
	OBJECT_NOCOPY_METHODS( CSceneTerrain )
	//
	CPtr<NGScene::IGameView> pGameView;
	CDBPtr<NDb::STerrain> pDBDesc;
	CPtr< CFuncBase<STime> > pAbsTimer;
	vector<vector<STerrainPatchHolder> > patches;
	vector<vector<STerrainPatchHolder> > terraBorders;
	//vector<CCrag> crags;
	vector<CRiver> rivers;
	vector<CRoad> roads;
	vector<CTerraSpot> terraspots;
	vector<CPrecipice> precipices;
	vector<CPeak> peaks;
	vector<CFoot> foots;
//	CObj<CWater> pWater;
	//
	const NDb::SVSOInstance* FindCragDescInstance( int nID ) const;
	const NDb::SVSOInstance* FindRiverDescInstance( int nID ) const;
	const NDb::SVSOInstance* FindRoadDescInstance( int nID ) const;
	void UpdateCragGfxInfoPtr();
	void UpdateRiverGfxInfoPtr();
	void UpdateRoadGfxInfoPtr();
	//
	// terrain itself
	void UpdatePatchGeometry( vector<NMeshData::SMeshData> *pMeshData, const int nPatchInd );
	void UpdateBorderGeometry( vector<NMeshData::SMeshData> *pMeshData, const int nBorderID );
	// road
	void AddRoad( SRoadGFXInfo *pRoadGfxInfo );
	bool UpdateRoad( const int nVSOID );
	void RemoveRoad( const int nVSOID );
	void RemoveAllRoads();
	// crag
	//void AddCrag( const SCragGFXInfo *pCragGfxInfo );
	//bool UpdateCrag( const int nVSOID );
	//void RemoveCrag( const int nVSOID );
	// peak
	void AddPeak( SPeakGFXInfo *pPeakGfxInfo );
	bool UpdatePeak( const int nVSOID );
	void RemovePeak( const int nVSOID );
	void RemoveAllPeaks();
	// peak
	void AddFoot( SFootGFXInfo *pFootGfxInfo );
	bool UpdateFoot( const int nVSOID );
	void RemoveFoot( const int nVSOID );
	void RemoveAllFoots();
	// river
	void AddRiver( const SRiverGFXInfo *pRiverGfxInfo );
	bool UpdateRiver( const int nVSOID );
	void RemoveRiver( const int nVSOID );
	void RemoveAllRivers();
	// terraspots
	void AddTerraSpot( STerraSpotGFXInfo *pTerraSpotGfxInfo );
	bool UpdateTerraSpot( const int nVSOID );
	void RemoveTerraSpot( const int nVSOID );
	void RemoveAllTerraSpots();
	// precipice
	void AddPrecipice( SPrecipiceGFXInfo *pPrecipiceGfxInfo );
	bool UpdatePrecipice( const int nVSOID );
	void RemovePrecipice( const int nVSOID );
	void RemoveAllPrecipices();
	//
public:
	CSceneTerrain() {}
	CSceneTerrain( const NDb::STerrain *_pDBDesc, NGScene::IGameView *_pGameView, CFuncBase<STime> *_pAbsTimer );
	//
	void HideTerrain( bool bHide );
	//
	int operator&( IBinSaver &saver );
};

