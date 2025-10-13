#pragma once

#include "System/Time.h"
#include "Misc/2Darray.h"
#include "B2_M1_Terrain/PatchHolder.h"

#include <cstdint>

namespace NGScene
{
	class IGameView;
};

namespace NDb
{
	struct SWater;
};

class CCSBound;

inline float GetTanHyperbolic( float z )
{
	const float fExpZ = exp( z );
	const float fExpMinusZ = exp( -z );
	return ( fExpZ - fExpMinusZ ) / ( fExpZ + fExpMinusZ );
}

class CWaterPatch;

class CWaterPatchLayer : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_BASIC_METHODS( CWaterPatchLayer );
	//
	CDGPtr<CWaterPatch, CPtr<CWaterPatch> > pPatch;
protected:
	bool NeedUpdate() { return pPatch.Refresh(); }
	void Recalc() {	}
public:
	CWaterPatchLayer() {}
	CWaterPatchLayer( CWaterPatch *_pPatch ): pPatch( _pPatch ) { pValue = new NGScene::CObjectInfo(); }
	//
	void UpdateMesh( NGScene::CObjectInfo::SData *pData )
	{
		pValue->AssignFast( pData );
		Updated();
	}
	//
	int operator&( IBinSaver &saver );
};

typedef NMeshData::SPatchHolder<CWaterPatchLayer> SWaterPatchLayerHolder;

class CWaterPatch : public CVersioningBase
{
	OBJECT_BASIC_METHODS( CWaterPatch );
	//
	// holders for each layer
	SWaterPatchLayerHolder layerWater;
	//SWaterPatchLayerHolder layerWhiteHorses;
	SWaterPatchLayerHolder layerSurf;
	//
	// geometry for each layer
	NGScene::CObjectInfo::SData objDataWater;
	NGScene::CObjectInfo::SData objDataSurf;
	//
	struct SGridType
	{
		union
		{
			struct
			{
				float x, y, z;
				uint32_t color;
			};
			struct
			{
				CVec3 pos;
				uint32_t color;
			};
		};
		SGridType() {}
	};
	//
	struct SWaveType
	{
		float fAmplitude;
		float fPeriod;
		float fInvPeriod;
		float fDeepWaveNumber;
		CVec2 direction;
		float fPhaseOffset;
	};
	//
	struct SWaterSurfaceType
	{
		float fHeight;
		std::vector<float> phase;
		std::vector<float> amplitude;
	};
	//
	struct SFoamParams
	{
		float fFoamY;
		float fFoamDy;
		float fFoamTexOffs;
		float fFoamTexCoeff;
		uint32_t nFoamColor;
	};
	//
	struct SWaterHorDeform
	{
		float fAngOffs;
		float fDAng;
		float fRadOffs;
		float fDRad;
		float fOffsX;
		float fOffsY;
	};
	//
	CDGPtr<CFuncBase<STime> > pTimer;
	CDBPtr<NDb::SWater> pDesc;
	CDGPtr<CCSBound> pBound;
	//
	std::vector<SGridType> grid;
	std::vector<SWaveType> waves;

	CArray2D<SWaterSurfaceType> waterSurf;

	int nCoastLen;
	CVec2 dGridSize;
	float fBaseHeight;

	float fCurOffset;
	bool bIsCurWave;
	float fCurWaveStart;
	float fCurWaveLen;

	CVec2 windVec;

	// foam parameters
	std::vector<std::vector<SFoamParams> > foamParams;
	std::vector<float> foamXOffsets;

	CArray2D<SWaterHorDeform> waterHorDeform;

	CArray2D<CVec4> noise;
	//
	float fWaterRotationSin, fWaterRotationCos;
	CVec2 vWaterOffset;
	//
protected:
	bool NeedUpdate() { return pTimer.Refresh(); }
	void Recalc();
	//
	void ProcessWaveDistribution( const int nWaveNum );
	float GetPhase( const int x, const int y, const int curw );
	float GetTanHyperbolic( const float z );
	float GetFracPart( const float z );
	float GetWaveProfile( const float z );
public:
	CWaterPatch() {}
	CWaterPatch( const NDb::SWater *_pDesc, CFuncBase<STime> *_pTimer, CCSBound *_pBound );
	//
	int Init( const int nSX, const int nSY, const int nCoast, const CVec2 &dsize, const CVec2 &org, const float _fRotAngle );
	int Process( const long nTime );
	//
	void Create( NGScene::IGameView *pGView );
	//
	int operator&( IBinSaver &saver );
};

typedef NMeshData::SPatchHolder<CWaterPatch> SWaterPatchHolder;

class CWater : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CWater )
	//
	std::vector< CObj<CWaterPatch> > patches;
	CDGPtr< CFuncBase<STime> > pTimer;
	CDBPtr<NDb::SWater> pDesc;
public:
	CWater() {}
	CWater( const NDb::SWater *_pDesc, CFuncBase<STime> *_pTimer ): pDesc( _pDesc ), pTimer( _pTimer ) {}
	void Create( NGScene::IGameView *pGView, const int nPatchesX, const int nPatchesY );
	//
	int operator&( IBinSaver &saver );
};


