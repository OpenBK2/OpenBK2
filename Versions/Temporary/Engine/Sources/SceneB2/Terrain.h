#pragma once

#include "B2_M1_Terrain/PatchHolder.h"
#include "System/Dg.h"
#include "TerraGen.h"

namespace NGScene
{
class IGameView;
class IMaterial;
} // namespace NGScene

namespace NDb
{
struct SWater;
}

// precipices

class CMeshDataPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	NMeshData::SMeshData *pInfo;

protected:
	void Recalc() override;

public:
	CMeshDataPatch() {}

	CMeshDataPatch( NMeshData::SMeshData *_pInfo )
		: pInfo( _pInfo )
	{
	}
};

typedef NMeshData::SPatchHolder<CMeshDataPatch> SPrecipicePatchHolder;

class CPrecipice
{
	SPrecipiceGFXInfo *pInfo;                   // graphics data (meshes for patches)
	std::vector<SPrecipicePatchHolder> patches; // engine's ready graphics data
	int nID;

public:
	CPrecipice()
		: pInfo( 0 )
	{
	}

	CPrecipice( SPrecipiceGFXInfo *_pInfo, const int _nID )
		: pInfo( _pInfo )
		, nID( _nID )
	{
	}

	//
	void CreatePatches( NGScene::IGameView *pGView );

	int GetID() const
	{
		return nID;
	}

	//
	void UpdatePrecipiceGfxInfoPtr( SPrecipiceGFXInfo *_pInfo )
	{
		pInfo = _pInfo;
	}
};

// peaks

class CPeakPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	NMeshData::SMeshDataTex2 *pInfo;

protected:
	void Recalc() override;

public:
	CPeakPatch() {}

	CPeakPatch( NMeshData::SMeshDataTex2 *_pInfo )
		: pInfo( _pInfo )
	{
	}
};

typedef NMeshData::SPatchHolder<CPeakPatch> SPeakPatchHolder;

class CPeak
{
	SPeakGFXInfo *pInfo;                                // graphics data (meshes for patches)
	std::vector<std::vector<SPeakPatchHolder>> patches; // engine's ready graphics data
	int nID;

public:
	CPeak()
		: pInfo( 0 )
	{
	}

	CPeak( SPeakGFXInfo *_pInfo, const int _nID )
		: pInfo( _pInfo )
		, nID( _nID )
	{
	}

	//
	void CreatePatches( NGScene::IGameView *pGView, const NDb::STerrain *pTerraDesc );

	int GetID() const
	{
		return nID;
	}

	//
	void UpdatePeakGfxInfoPtr( SPeakGFXInfo *_pInfo )
	{
		pInfo = _pInfo;
	}
};

// foots

typedef NMeshData::SPatchHolder<CMeshDataPatch> SFootPatchHolder;

class CFoot
{
	SFootGFXInfo *pInfo;                   // graphics data (meshes for patches)
	std::vector<SFootPatchHolder> patches; // engine's ready graphics data
	int nID;

public:
	CFoot()
		: pInfo( 0 )
	{
	}

	CFoot( SFootGFXInfo *_pInfo, const int _nID )
		: pInfo( _pInfo )
		, nID( _nID )
	{
	}

	//
	void CreatePatches( NGScene::IGameView *pGView, const NDb::STerrain *pTerraDesc );

	int GetID() const
	{
		return nID;
	}

	//
	void UpdateFootGfxInfoPtr( SFootGFXInfo *_pInfo )
	{
		pInfo = _pInfo;
	}
};

// river

class CRiverPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	const NMeshData::SMeshData *pInfo;
	CDGPtr<CFuncBase<STime>> pTimer;
	float fStreamSpeed;

protected:
	bool NeedUpdate() override
	{
		return pTimer == 0 ? false : pTimer.Refresh();
	}

	void Recalc() override;

public:
	CRiverPatch()
		: pInfo( 0 )
		, fStreamSpeed( 0 )
	{
	}

	CRiverPatch( const NMeshData::SMeshData *_pInfo, float _fStreamSpeed, CFuncBase<STime> *_pTimer )
		: pInfo( _pInfo )
		, fStreamSpeed( _fStreamSpeed )
		, pTimer( _pTimer )
	{
	}
};

typedef NMeshData::SPatchHolder<CRiverPatch> SRiverPatchHolder;

class CRiver
{
	const SRiverGFXInfo *pInfo; // graphics data (meshes for patches)
	int nID;
	std::vector<SRiverPatchHolder> waterPatches;
	std::vector<SRiverPatchHolder> water2Patches;
	std::vector<SRiverPatchHolder> bottomPatches;
	CDGPtr<CFuncBase<STime>> pTimer;

public:
	CRiver()
		: pInfo( 0 )
	{
	}

	CRiver( const SRiverGFXInfo *_pInfo, const int _nID, CFuncBase<STime> *_pTimer )
		: pInfo( _pInfo )
		, nID( _nID )
		, pTimer( _pTimer )
	{
	}

	//
	void CreatePatches( NGScene::IGameView *pGView );

	int GetID() const
	{
		return nID;
	}

	//
	void UpdateRiverGfxInfoPtr( const SRiverGFXInfo *_pInfo )
	{
		pInfo = _pInfo;
	}
};

// road

typedef NMeshData::SPatchHolder<CMeshDataPatch> SRoadPatchHolder;

class CRoad
{
	SRoadGFXInfo *pInfo;                   // graphics data (meshes for patches)
	std::vector<SRoadPatchHolder> patches; // engine's ready graphics data
	int nID;

public:
	CRoad()
		: pInfo( 0 )
	{
	}

	CRoad( SRoadGFXInfo *_pInfo, const int _nID )
		: pInfo( _pInfo )
		, nID( _nID )
	{
	}

	//
	void CreatePatches( NGScene::IGameView *pGView );

	int GetID() const
	{
		return nID;
	}

	//
	void UpdateRoadGfxInfoPtr( SRoadGFXInfo *_pInfo )
	{
		pInfo = _pInfo;
	}
};

// terraspot

typedef NMeshData::SPatchHolder<CMeshDataPatch> STerraSpotPatchHolder;

class CTerraSpot
{
	STerraSpotGFXInfo *pInfo;    // graphics data (meshes for patches)
	STerraSpotPatchHolder patch; // engine's ready graphics data
	int nID;

public:
	CTerraSpot()
		: pInfo( 0 )
	{
	}

	CTerraSpot( STerraSpotGFXInfo *_pInfo, const int _nID )
		: pInfo( _pInfo )
		, nID( _nID )
	{
	}

	//
	void CreatePatches( NGScene::IGameView *pGView );

	int GetID() const
	{
		return nID;
	}

	//
	void UpdateTerraSpotGfxInfoPtr( STerraSpotGFXInfo *_pInfo )
	{
		pInfo = _pInfo;
	}
};

// terrain

typedef NMeshData::SPatchHolder<CMeshDataPatch> STerrainPatchHolder;

class CSceneTerrain : public ITerraGfxObserver
{
	OBJECT_NOCOPY_METHODS( CSceneTerrain )
	//
	CPtr<NGScene::IGameView> pGameView;
	CDBPtr<NDb::STerrain> pDBDesc;
	CPtr<CFuncBase<STime>> pAbsTimer;
	std::vector<std::vector<STerrainPatchHolder>> patches;
	std::vector<std::vector<STerrainPatchHolder>> terraBorders;
	//vector<CCrag> crags;
	std::vector<CRiver> rivers;
	std::vector<CRoad> roads;
	std::vector<CTerraSpot> terraspots;
	std::vector<CPrecipice> precipices;
	std::vector<CPeak> peaks;
	std::vector<CFoot> foots;
	//	CObj<CWater> pWater;
	//
	const NDb::SVSOInstance *FindCragDescInstance( int nID ) const;
	const NDb::SVSOInstance *FindRiverDescInstance( int nID ) const;
	const NDb::SVSOInstance *FindRoadDescInstance( int nID ) const;
	void UpdateCragGfxInfoPtr();
	void UpdateRiverGfxInfoPtr();
	void UpdateRoadGfxInfoPtr();
	//
	// terrain itself
	void UpdatePatchGeometry( std::vector<NMeshData::SMeshData> *pMeshData, const int nPatchInd ) override;
	void UpdateBorderGeometry( std::vector<NMeshData::SMeshData> *pMeshData, const int nBorderID ) override;
	// road
	void AddRoad( SRoadGFXInfo *pRoadGfxInfo ) override;
	bool UpdateRoad( const int nVSOID ) override;
	void RemoveRoad( const int nVSOID ) override;
	void RemoveAllRoads() override;
	// crag
	//void AddCrag( const SCragGFXInfo *pCragGfxInfo );
	//bool UpdateCrag( const int nVSOID );
	//void RemoveCrag( const int nVSOID );
	// peak
	void AddPeak( SPeakGFXInfo *pPeakGfxInfo ) override;
	bool UpdatePeak( const int nVSOID ) override;
	void RemovePeak( const int nVSOID ) override;
	void RemoveAllPeaks() override;
	// peak
	void AddFoot( SFootGFXInfo *pFootGfxInfo ) override;
	bool UpdateFoot( const int nVSOID ) override;
	void RemoveFoot( const int nVSOID ) override;
	void RemoveAllFoots() override;
	// river
	void AddRiver( const SRiverGFXInfo *pRiverGfxInfo ) override;
	bool UpdateRiver( const int nVSOID ) override;
	void RemoveRiver( const int nVSOID ) override;
	void RemoveAllRivers() override;
	// terraspots
	void AddTerraSpot( STerraSpotGFXInfo *pTerraSpotGfxInfo ) override;
	bool UpdateTerraSpot( const int nVSOID ) override;
	void RemoveTerraSpot( const int nVSOID ) override;
	void RemoveAllTerraSpots() override;
	// precipice
	void AddPrecipice( SPrecipiceGFXInfo *pPrecipiceGfxInfo ) override;
	bool UpdatePrecipice( const int nVSOID ) override;
	void RemovePrecipice( const int nVSOID ) override;
	void RemoveAllPrecipices() override;
	//
public:
	CSceneTerrain() {}

	CSceneTerrain( const NDb::STerrain *_pDBDesc, NGScene::IGameView *_pGameView, CFuncBase<STime> *_pAbsTimer );
	//
	void HideTerrain( bool bHide );
	//
	int operator&( IBinSaver &saver ) override;
};
