#pragma once

#include "Misc/2DArray.h"
#include "System/DG.h"
#include "B2_M1_Terrain/PatchHolder.h"
#include "WaterStuff.h"
#include "B2_M1_Terrain/TerrUtils.h"
#include "Image/ImageConvertor.h"

class CCSBound;

namespace NDb
{
	struct SWater;
}

namespace NGScene
{
	class IGameView;
}

struct SWaterNode : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SWaterNode )
public:
	struct SWaveParams
	{
		float fAmplitude;
		float fPhase;

		SWaveParams() : fAmplitude(0), fPhase(0) {}
		// Need to sort by decreasing amplitude but our ripped stl doesn't have greater functor.
		friend bool operator < ( const SWaterNode::SWaveParams &a, const SWaterNode::SWaveParams &b ) { return a.fAmplitude > b.fAmplitude; }
	};
	//
public:
	CVec3 vPos;
	CVec2 vTex;
	BYTE nAlpha;//float fAlpha;
	float fAngleOffset;
	float fAngleStep;
	float fRadiusOffset;
	float fRadiusStep;
	bool bBorderTexX;
	bool bBorderTexY;
	bool bIsLargeAlpha;
	//
	BYTE nParamInd;
	//
	vector<SWaveParams> waveParams;

	SWaterNode() : bIsLargeAlpha(false) {}
	void SetAlpha( float _f ) { nAlpha = Clamp( Float2Int( _f * 255.0f ), 0, 255 ); }
	float GetAlpha() const { return nAlpha / 255.0f; }
};

class CWaterController;

class CVisWaterPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	bool bUpdate;
	const CWaterController *pController;
	CDGPtr<CFuncBase<STime> > pTimer;
	//
	bool bUseWaves;
	float fHorDeformMinRadius;
	float fHorDeformMaxRadius;
	CDBPtr<NDb::STwoSidedLight> pLight;
	CVec3 vSunDir;
	int nAnimMaxFrames;
	int nNumFramesX;
	int nNumFramesY;
	float fTexCoordOffsetX;
	float fTexCoordOffsetY;
	float fTexCoord2OffsetX;
	float fTexCoord2OffsetY;
	bool bUseNoise;
	float fNoiseCoeff;
	BYTE nParamInd;
protected:
	void UpdateGeomData();
	void Recalc();
	//bool NeedUpdate() { return false; }
public:
	NMeshData::SMeshDataTex2 data;
	vector<STriangle> realTrgs;
	//
	CVisWaterPatch( const CWaterController *pContr, CFuncBase<STime> *_pTimer,
		const bool _bUseWaves, const float fMinRad, const float fMaxRad, const NDb::STwoSidedLight *_pLight,
		const int nFramesX, const int nFramesY, const bool _bUseNoise, const float _fNoiseCoeff, const BYTE _nParamInd );
	~CVisWaterPatch() {};
};

typedef NMeshData::SPatchHolder<CVisWaterPatch> CVisWaterPatchHolder;

class CWaterController
{
	struct SWaveType
	{
		float fAmplitude;
		float fPeriod;
		float fInvPeriod;
		float fDeepWaveNumber;
		float fPhaseOffset;
	};
	//
	CObj<NGScene::IGameView> pGScene;
	vector<CVisWaterPatchHolder> waterPatches;
	//
	CPtr<CFuncBase<STime> > pTimer;
	vector<CObj<CObjectBase> > shaderWaterPatches;

protected:
	void ProcessWavesDistribution( CArray2D<CPtr<SWaterNode> > *pWaterNodes );
	void SetBorders( CArray2D<CPtr<SWaterNode> > *pWaterNodes, const vector<NWaterStuff::SSurfBorder> &waterBorders );
	void CreatePatches( const vector<NWaterStuff::SWaterParams> &waterParams, const CArray2D<CPtr<SWaterNode> > &waterNodes );

	void InitSilentWater( const CArray2D<BYTE> &seaMap,
												const vector<NWaterStuff::SWaterParams> &_waterParams,
												const vector<NWaterStuff::SSurfBorder> &waterBorders,
												NGScene::IGameView *_pGScene,
												const NDb::SWater *pWater );

	void InitOceanWater( const CArray2D<BYTE> &seaMap,
												const vector<NWaterStuff::SWaterParams> &_waterParams,
												const vector<NWaterStuff::SSurfBorder> &waterBorders,
												NGScene::IGameView *_pGScene,
												const NDb::SWater *pWater,
												float fWinterDirection );

public:
	//CDBPtr<NDb::SWater> pDesc;
	vector<SWaveType> waves;
	vector<CArray2D<SColor24> > noises;

	//
	CWaterController() {}
	void Init(	const float fAngle, const CArray2D<BYTE> &seaMap,
							const vector<NWaterStuff::SWaterParams> &waterParams,
							const vector<NWaterStuff::SSurfBorder> &waterBorders,
							NGScene::IGameView *_pGScene,
							//CArray2D<BYTE> &waterBottomMap,
							const NDb::SWater *pWater );
	//const CArray2D<CPtr<SWaterNode> > &GetWaterNodes() const { return waterNodes; }
	void AttachTimer( CFuncBase<STime> *_pTimer ) { pTimer = _pTimer; }
};


