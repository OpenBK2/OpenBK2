#include "stdafx.h"

#include "B2_M1_Terrain/DBPreLight.h"
#include "3Dmotor/GMaterial.hpp"
#include "3Dmotor/GMaterial.h"
#include "3Dmotor/GfxRender.h"
#include "Misc/Win32Random.h"
#include "System/FastMath.h"
#include "Image/Targa.h"
#include "GenTerrain.h"
#include "TerraHeight.h"
#include "Scene.h"
#include "WaterController.h"
#include "System/VFSOperations.h"

#include <algorithm>
#include <cstdint>

#include <fmt/format.h>

#define DEF_WATER_PATCH_SIZE_X 6
#define DEF_WATER_PATCH_SIZE_Y 6

#define DEF_WAVES_NUM 9
#define DEF_WAVES_AMPLITUDE 1.4f
#define DEF_WAVES_DEEP_WAVE_NUMBER 0.1f
#define DEF_WAVES_PERIOD 0.3f
#define DEF_WAVES_PERIOD_VARIATION -0.15f

#define DEF_WAVE_MAX_AMPL 1.5f
#define DEF_WAVE_MIN_AMPL 0.0f
#define DEF_WAVES_SCALE_FACTOR 0.6f
#define DEF_WAVE_DISTR_STEP 0.5f
#define DEF_WAVE_LEN 5.0f
#define DEF_WAVE_LEN_VARIATION 0.0f/*0.3f*/
#define DEF_NOWAVE_LEN /*7.5f*/10.0f
#define DEF_NOWAVE_LEN_VARIATION 0.0f/*0.3f*/

#define DEF_WATER_DEPTH 5.0f
#define DEF_WATER_TIME_STEP /*0.000014f*/0.00003f

#define DEF_WATER_HOR_DEFORM_MIN_RAD 0.0f
#define DEF_WATER_HOR_DEFORM_MAX_RAD 0.5f
#define DEF_NEW_TIME ( 1.0f / 50.0f )
#define DEF_WATER_HOR_DEFORM_MIN_DANG ( 2.5f * DEF_NEW_TIME * 0.0175f )
#define DEF_WATER_HOR_DEFORM_VAR_DANG ( 7.5f * DEF_NEW_TIME * 0.0175f )
#define DEF_WATER_HOR_DEFORM_DRAD ( 0.05f * 0.5f * DEF_NEW_TIME )

#define DEF_WATER_TEX_TILES_PER_TEX 6
#define DEF_WATER_TEX_TILE_COEFF ( 1.0f / ( DEF_WATER_TEX_TILES_PER_TEX - 1 ) )
//#define DEF_WATER_TEX_ANIM_NUM 4
//#define DEF_WATER_TEX_COORD_OFFSET (1.0f / DEF_WATER_TEX_ANIM_NUM)
//#define DEF_WATER_TEX_COORD_OFFSET2 (1.0f / DEF_WATER_TEX_ANIM_NUM - DEF_TEX_ERROR * 2.0f)

//#define DEF_ANIM_MAX_FRAMES 16
#define DEF_WATER_ANIM_TIME DEF_NEW_TIME

//#define DEF_MESH2_TILES_PER_TEX 2
//#define DEF_MESH2_TILE_COEFF ( 1.0f / DEF_MESH2_TILES_PER_TEX )
#define DEF_FOAM_Y_ANIM ( /*0.0125f*/0.015f * DEF_NEW_TIME * 0.5f )

#define DEF_TEX_ERROR 0.001f

#define DEF_MIN_HEIGHT 0.3f/*0.575f*/
#define DEF_MAX_HEIGHT 1.0f/*1.15f*/
#define DEF_HEIGHT_COEFF ( 1.0f / ( DEF_MAX_HEIGHT - DEF_MIN_HEIGHT ) )

#define DEF_LIGHT_SCALING ( 255.0f * 4.0f )
#define DEF_INV_255 ( 1.0f / 255.0f )

#define DEF_BLENDING_DIST ( DEF_TILE_SIZE * 3 )
#define DEF_BLENDING_ZERO ( DEF_TILE_SIZE * FP_SQRT_2 / DEF_BLENDING_DIST )
#define DEF_BLENDING_NON_ZERO_COEFF ( 1.0f / ( 1.0f - DEF_BLENDING_ZERO ) )

static float fWaterTexTileCoeff = DEF_WATER_TEX_TILE_COEFF;
static float fFoamYAnim = DEF_FOAM_Y_ANIM;

inline float GetFracPart( const float z )
{
	return z - (int)z;
}

struct SAnimWaterMaterialKey
{
	CPtrFuncBase<NGfx::CTexture> *pTex;
	CPtrFuncBase<NGfx::CTexture> *pSecondTex;
	int nPriority;
	CFuncBase<STime> *pTime;
	bool bProjectOnTerrain;
	int nNumFramesX;
	int nNumFramesY;
	bool bApplyFog;
	bool bAddPlaced;
	bool bDrawHorses;

	SAnimWaterMaterialKey() {}
	SAnimWaterMaterialKey( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pSecondTex, int _nPriority,
		CFuncBase<STime> *_pTime, bool _bProjectOnTerrain, int _nNumFramesX, int _nNumFramesY, bool _bApplyFog, bool _bAddPlaced, bool _bDrawHorses ) :
	pTex(_pTex), pSecondTex(_pSecondTex), nPriority(_nPriority), pTime(_pTime), bProjectOnTerrain(_bProjectOnTerrain), nNumFramesX(_nNumFramesX), nNumFramesY(_nNumFramesY), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced), bDrawHorses(_bDrawHorses)
	{
	}

	bool operator == ( const SAnimWaterMaterialKey &animWaterKey )
	{
		return ( pTex == animWaterKey.pTex ) &&	
			( pSecondTex == animWaterKey.pSecondTex) &&
			( nPriority == animWaterKey.nPriority) &&
			( pTime == animWaterKey.pTime) &&
			( bProjectOnTerrain == animWaterKey.bProjectOnTerrain) &&
			( nNumFramesX == animWaterKey.nNumFramesX) &&
			( nNumFramesY == animWaterKey.nNumFramesY) &&
			( bApplyFog == animWaterKey.bApplyFog) &&
			( bAddPlaced == animWaterKey.bAddPlaced) &&
			( bDrawHorses == animWaterKey.bDrawHorses);
	}
};
static NGScene::CAnimWaterMaterial * GetAnimatedMaterial( const SAnimWaterMaterialKey &animWaterKey )
{
	static std::vector< std::pair<SAnimWaterMaterialKey, CPtr<NGScene::CAnimWaterMaterial> > > animMaterials;
	for ( int iMaterial = 0; iMaterial < animMaterials.size(); ++iMaterial )
	{
		if ( animMaterials[iMaterial].first == animWaterKey  && IsValid( animMaterials[iMaterial].second ) )
			return animMaterials[iMaterial].second;
	}

	animMaterials.push_back( std::pair<SAnimWaterMaterialKey, CPtr<NGScene::CAnimWaterMaterial> >() );
	animMaterials.back().first = animWaterKey;
	animMaterials.back().second = new NGScene::CAnimWaterMaterial( animWaterKey.pTex, animWaterKey.pSecondTex, animWaterKey.nPriority, animWaterKey.pTime, 
		animWaterKey.bProjectOnTerrain, animWaterKey.nNumFramesX, animWaterKey.nNumFramesY, animWaterKey.bApplyFog, animWaterKey.bAddPlaced, animWaterKey.bDrawHorses );

	return animMaterials.back().second;
}

void CVisWaterPatch::UpdateGeomData()
{
}

void CVisWaterPatch::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;

	UpdateGeomData();

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	//objData.geometry.SetTriangles( data.triangles );
	objData.geometry = data.triangles;
	objData.secondTex = data.secondTex;

	pValue->AssignFast( &objData );
	bUpdate = false;
}

CVisWaterPatch::CVisWaterPatch( const CWaterController *pContr,
																CFuncBase<STime> *_pTimer,
																const bool _bUseWaves,
																const float fMinRad, const float fMaxRad,
																const NDb::STwoSidedLight *_pLight,
																const int nFramesX, const int nFramesY,
																const bool _bUseNoise,
																const float _fNoiseCoeff,
																const uint8_t _nParamInd )
	: bUpdate( true ),
	pController( pContr ),
	pTimer( _pTimer ),
	bUseWaves( _bUseWaves ),
	fHorDeformMinRadius( fMinRad ),
	fHorDeformMaxRadius( fMaxRad ),
	pLight( _pLight ),
	bUseNoise( _bUseNoise ),
	fNoiseCoeff( _fNoiseCoeff ),
	nParamInd( _nParamInd )
{
	if ( _pLight )
	{
		const float fHor = sin( ToRadian( _pLight->fPitch ) );
		vSunDir.z = -cos( ToRadian( _pLight->fPitch ) );
		vSunDir.x = fHor * cos( ToRadian( _pLight->fYaw ) );
		vSunDir.y = fHor * sin( ToRadian( _pLight->fYaw ) );
		vSunDir = -vSunDir;
	}
	else
		vSunDir.Set( 0, 0, 0 );

	nAnimMaxFrames = (std::max)( nFramesX * nFramesY, 1 );
	nNumFramesX = (std::max)( nFramesX, 1 );
	nNumFramesY = (std::max)( nFramesY, 1 );
	fTexCoordOffsetX = 1.0f / nNumFramesX;
	fTexCoordOffsetY = 1.0f / nNumFramesY;
	fTexCoord2OffsetX = 1.0f / nNumFramesX - DEF_TEX_ERROR * 2.0f;
	fTexCoord2OffsetY = 1.0f / nNumFramesY - DEF_TEX_ERROR * 2.0f;
}

inline void RotateVector( CVec2 *vRotPos, const CVec2 &vPos, const CVec2 &vCenter, const float fCosAng, const float fSinAng )
{
	vRotPos->x = ( vPos.x - vCenter.x ) * fCosAng - ( vPos.y - vCenter.y ) * fSinAng + vCenter.x;
	vRotPos->y = ( vPos.x - vCenter.x ) * fSinAng + ( vPos.y - vCenter.y ) * fCosAng + vCenter.y;
}

inline int FindRealIndex( const CVec2i &val, const std::vector<CVec2i> &arr )
{
	for ( int i = 0; i < arr.size(); ++i )
	{
		if ( arr[i] == val )
			return i;
	}
	NI_ASSERT( false, "Can't find value in array" );
	return 0;
}

inline float GetTanHyperbolic( const float z )
{
	return ( exp( z ) - exp( -z ) ) / ( exp( z ) + exp( -z ) );
}

inline float GetPhase( const int x, const int y, const float fDeepWaveNumber )
{
	return fDeepWaveNumber / GetTanHyperbolic( fDeepWaveNumber * DEF_WATER_DEPTH );
}

void CWaterController::ProcessWavesDistribution( CArray2D<CPtr<SWaterNode> > *pWaterNodes )
{
	CArray2D<CPtr<SWaterNode> > &waterNodes = *pWaterNodes;

	float fDefPhase, fCurOffset, fCurWaveStart, fCurWaveLen;
	bool bIsCurWave;

	for ( int k = 0; k < waves.size(); ++k )
	{
		waves[k].fPhaseOffset = NWin32Random::Random( 0.0f, 10.0f );

		fCurOffset = 0.0f;

		const int nWaveType = k % 3;

		if ( nWaveType > 0 ) // create non-wave
		{
			bIsCurWave = false;
			fCurWaveStart = /*0.0f;*/-( NWin32Random::Random( 0.0f, 1.0f ) * 0.5f + 0.5f * ( nWaveType - 1 ) ) * DEF_NOWAVE_LEN;// + DEF_NOWAVE_LEN;
			fCurWaveLen = fCurWaveStart + DEF_NOWAVE_LEN * ( 1.0f - DEF_NOWAVE_LEN_VARIATION ) +
				NWin32Random::Random( 0.0f, 1.0f ) * DEF_NOWAVE_LEN * DEF_NOWAVE_LEN_VARIATION;
		}
		else // create wave
		{
			bIsCurWave = true;
			fCurWaveStart = /*0.0f;*/-NWin32Random::Random( 0.0f, 1.0f ) * DEF_WAVE_LEN;// + DEF_WAVE_LEN * 0.0f;
			fCurWaveLen = fCurWaveStart + DEF_WAVE_LEN * ( 1.0f - DEF_WAVE_LEN_VARIATION ) +
				NWin32Random::Random( 0.0f, 1.0f ) * DEF_WAVE_LEN * DEF_WAVE_LEN_VARIATION;
		}

		for ( int i = 0; i < waterNodes.GetSizeX(); ++i )
		{
			float fAmpl;
			if ( bIsCurWave )
			{
				fAmpl = Clamp( static_cast<float>(fabs( ( fCurWaveStart + fCurWaveLen ) - fCurOffset * 2.0f ) / ( fCurWaveLen - fCurWaveStart )), 0.0f, 1.0f );
				fAmpl = 1.0f - fAmpl * fAmpl;
				fAmpl = DEF_WAVE_MIN_AMPL + fAmpl * DEF_WAVE_MAX_AMPL;
			}
			else
				fAmpl = DEF_WAVE_MIN_AMPL;

			fDefPhase = waves[k].fPhaseOffset;

			const int nStages = 3;
			for ( int p = 0; p < nStages; ++p )
			{
				const float fCoeff = (float)p / ( nStages - 1 );
				fDefPhase += sin( (float)i * ( 0.3f + 0.9f * fCoeff ) ) * ( 0.01f + 0.04f * fCoeff );
			}

			float fPhase = fDefPhase * DEF_WAVES_SCALE_FACTOR;
			for ( int g = 0; g < waterNodes.GetSizeY(); ++g )
			{
				fPhase += GetPhase( i, g, waves[k].fDeepWaveNumber ) * DEF_WAVES_SCALE_FACTOR;
				if ( waterNodes[g][i] )
				{
					waterNodes[g][i]->waveParams[k].fAmplitude = fAmpl;
					waterNodes[g][i]->waveParams[k].fPhase = GetFracPart( fPhase );
				}
			}

			fCurOffset += 1.0f;
			if ( fCurOffset >= fCurWaveLen )
			{
				bIsCurWave = !bIsCurWave;
				fCurWaveStart = fCurWaveLen;
				if ( bIsCurWave ) // create wave
				{
					fCurWaveLen = fCurWaveStart + DEF_WAVE_LEN * ( 1.0f - DEF_WAVE_LEN_VARIATION ) +
						NWin32Random::Random( 0.0f, 1.0f ) * DEF_WAVE_LEN * DEF_WAVE_LEN_VARIATION;
				}
				else // create non-wave
				{
					fCurWaveLen = fCurWaveStart + DEF_NOWAVE_LEN * ( 1.0f - DEF_NOWAVE_LEN_VARIATION ) +
						NWin32Random::Random( 0.0f, 1.0f ) * DEF_NOWAVE_LEN * DEF_NOWAVE_LEN_VARIATION;
				}
			}
		}
	}

	// Sort waves by amplitude
	for ( int g = 0; g < waterNodes.GetSizeY(); ++g )
	{
		for ( int i = 0; i < waterNodes.GetSizeX(); ++i )
		{
			if ( !waterNodes[g][i] )
				continue;

			SWaterNode *pCurNode = waterNodes[g][i];
			std::sort( pCurNode->waveParams.begin(), pCurNode->waveParams.end() );
		}
	}
}

inline int FindWaterParamInd( int nInd, const std::vector<NWaterStuff::SWaterParams> &params )
{
	for ( int k = 0; k < params.size(); ++k )
	{
		if ( params[k].nSeaMapIndex == nInd )
			return k;
	}
	return -1;
}

void CWaterController::Init(	const float fAngle,
															const CArray2D<uint8_t> &seaMap,
															const std::vector<NWaterStuff::SWaterParams> &_waterParams,
															const std::vector<NWaterStuff::SSurfBorder> &waterBorders,
															NGScene::IGameView *_pGScene,
															//CArray2D<uint8_t> &waterBottomMap,
															const NDb::SWater *pWater )
{
	waterPatches.clear();
	shaderWaterPatches.clear();

	if ( _waterParams.size() <= 0 )
		return;

	pGScene = _pGScene;

	std::vector<NWaterStuff::SWaterParams> waterParams( _waterParams );
	for ( int k = 0; k < waterParams.size(); ++k )
		waterParams[k].nSeaMapIndex = k + 1;

	std::vector<NWaterStuff::SWaterParams> silentWaterParams, oceanWaterParams;
	for ( std::vector<NWaterStuff::SWaterParams>::const_iterator it = waterParams.begin(); it != waterParams.end(); ++it )
	{
		if ( it->bUseWaves )
			oceanWaterParams.push_back( *it );
		else
			silentWaterParams.push_back( *it );
	}

	if ( _pGScene )
	{
		if ( !oceanWaterParams.empty() )
			InitOceanWater( seaMap, oceanWaterParams, waterBorders, _pGScene, pWater, fAngle );

		if ( !silentWaterParams.empty() )
			InitSilentWater( seaMap, silentWaterParams, waterBorders, _pGScene, pWater );

	}
}

struct SWaterAlphaSmoothProfile
{
	float operator()( const float x ) const
	{
		// TODO: new waterbanks
		//const float fY = x * 2.0f - 1.0f;
		//if ( fY < 0 )
		//	return 0;
		//return sin( FP_PI2 * fY );

		// TODO: old waterbanks
		return x < DEF_WATER_BLENDING_ZERO_COEFF ? 0.0f : ( ( x - DEF_WATER_BLENDING_ZERO_COEFF ) * DEF_WATER_BLENDING_NONZERO_COEFF );
	}
};

static bool IsInnerPoint( const CArray2D<CPtr<SWaterNode> > &rWaterNodes, int nY, int nX )
{
	bool bResult = ( nX > 0 ) && ( nY > 0 ) &&
								 ( nX + 1 < rWaterNodes.GetSizeX() ) &&
								 ( nY + 1 < rWaterNodes.GetSizeY() );

	bResult = bResult && (rWaterNodes[nY][nX + 1] != 0);
	bResult = bResult && (rWaterNodes[nY][nX - 1] != 0);
	bResult = bResult && (rWaterNodes[nY + 1][nX] != 0);
	bResult = bResult && (rWaterNodes[nY - 1][nX] != 0);
	//
	bResult = bResult && (rWaterNodes[nY + 1][nX + 1] != 0);
	bResult = bResult && (rWaterNodes[nY - 1][nX - 1] != 0);
	bResult = bResult && (rWaterNodes[nY + 1][nX - 1] != 0);
	bResult = bResult && (rWaterNodes[nY - 1][nX + 1] != 0);

	return bResult;
}

void CWaterController::SetBorders( CArray2D<CPtr<SWaterNode> > *pWaterNodes, const std::vector<NWaterStuff::SSurfBorder> &waterBorders )
{
	CVec3 vBBMin, vBBMax;
	static std::vector<CVec3fEx> coastPoints( 4096 );
	SWaterAlphaSmoothProfile waterAlphaSmoothProfile;
	float fAlpha;
	int nInd;
	CVec3 vCurNorm;

	CArray2D<uint8_t> singleMask( 128, 128 );
	CArray2D<CPtr<SWaterNode> > &waterNodes = *pWaterNodes;

	for ( int k = 0; k < waterBorders.size(); ++k )
	{
		const std::vector<NDb::SVSOPoint> &curBorder = waterBorders[k].points;

		if ( curBorder.size() < 2 )
			continue;

		const bool bNeedCycle = ( fabs(curBorder[0].vPos.x - curBorder[curBorder.size() - 1].vPos.x) < DEF_EPS ) &&
														( fabs(curBorder[0].vPos.y - curBorder[curBorder.size() - 1].vPos.y) < DEF_EPS );

		if ( bNeedCycle )
			coastPoints.resize( (curBorder.size() - 1) * 4 + 2 );
		else
			coastPoints.resize( (curBorder.size() - 1) * 4 );

		const int nCoastSize = coastPoints.size();
		nInd = 0;
		for ( int i = 0; i < (curBorder.size() - 1); ++i )
		{
			const CVec3 &vCurPos = curBorder[i].vPos;
			const CVec3 &vNextPos = curBorder[i + 1].vPos;
			const CVec3 &vCurNorm = curBorder[i].vNorm;
			const CVec3 &vNextNorm = curBorder[i + 1].vNorm;
			//const float &fCurWidth = AI2Vis( curBorder[i].fWidth );
			//const float &fNextWidth = AI2Vis( curBorder[i + 1].fWidth );
			const float &fCurWidth = DEF_WATER_BLENDING_DIST;
			const float &fNextWidth = DEF_WATER_BLENDING_DIST;
			//
			coastPoints[nInd].Set( AI2Vis(vCurPos.x), AI2Vis(vCurPos.y), 1.0f, 1 );
			coastPoints[nCoastSize - 1 - nInd].Set( coastPoints[nInd].x + vCurNorm.x * fCurWidth,
																							coastPoints[nInd].y + vCurNorm.y * fCurWidth, 1.0f, 1 );
			++nInd;
			coastPoints[nInd].Set( AI2Vis(vNextPos.x), AI2Vis(vNextPos.y), 1.0f, 1 );
			coastPoints[nCoastSize - 1 - nInd].Set( coastPoints[nInd].x + vNextNorm.x * fNextWidth,
																							coastPoints[nInd].y + vNextNorm.y * fNextWidth, 1.0f, 1 );
			++nInd;
		}

		if ( bNeedCycle )
		{
			coastPoints[nInd] = coastPoints[0];
			coastPoints[nCoastSize - 1 - nInd] = coastPoints[nCoastSize - 1];
		}

		vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
		vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
		for ( std::vector<CVec3fEx>::const_iterator it = coastPoints.begin(); it != coastPoints.end(); ++it )
		{
			const CVec3 vPos( it->x, it->y, 0 );
			vBBMin.Minimize( vPos );
			vBBMax.Maximize( vPos );
		}

		for ( int g = 0; g < waterNodes.GetSizeY(); ++g )
		{
			for ( int i = 0; i < waterNodes.GetSizeX(); ++i )
			{
				if ( waterNodes[g][i] )
				{
					const CVec3 &vPos = waterNodes[g][i]->vPos;
					if ( (vPos.x >= vBBMin.x) && (vPos.y >= vBBMin.y) && (vPos.x <= vBBMax.x) && (vPos.y <= vBBMax.y) )
					{
						if ( GetIncRidgeHeightMinLaw(CVec2(vPos.x, vPos.y), coastPoints, &fAlpha, waterAlphaSmoothProfile) )
						{
							fAlpha = Clamp( fAlpha, 0.0f, 1.0f );
							SWaterNode &wn = *waterNodes[g][i];
							if ( wn.bIsLargeAlpha ) // first alpha changing
								wn.SetAlpha( fAlpha );
							else // interference
								wn.SetAlpha( ( wn.GetAlpha() + fAlpha ) * 0.5f );
							wn.bIsLargeAlpha = false;
						}
					}
				}
			}
		}
		/*// set border tiles in mask of bottom visibility
		const int nBorderSize = ( coastPoints.size() >> 1 ) - 1;
		for ( int p = 0; p < nBorderSize; ++p )
		{
			const int nInvInd = coastPoints.size() - 1 - p;
			const CVec2i v1( coastPoints[p].x * DEF_INV_TILE_SIZE, coastPoints[p].y * DEF_INV_TILE_SIZE );
			const CVec2i v2( coastPoints[p + 1].x * DEF_INV_TILE_SIZE, coastPoints[p + 1].y * DEF_INV_TILE_SIZE );
			const CVec2i v3( coastPoints[nInvInd].x * DEF_INV_TILE_SIZE, coastPoints[nInvInd].y * DEF_INV_TILE_SIZE );
			const CVec2i v4( coastPoints[nInvInd - 1].x * DEF_INV_TILE_SIZE, coastPoints[nInvInd - 1].y * DEF_INV_TILE_SIZE );
			const CVec2i vMin( min( min( v1.x, v2.x ), min( v3.x, v4.x ) ) - 1, min( min( v1.y, v2.y ), min( v3.y, v4.y ) ) - 1 );
			const CVec2i vMax( max( max( v1.x, v2.x ), max( v3.x, v4.x ) ), max( max( v1.y, v2.y ), max( v3.y, v4.y ) ) );
			const int nSizeX = vMax.x - vMin.x + 2;
			const int nSizeY = vMax.y - vMin.y + 2;
			if ( ( nSizeX > singleMask.GetSizeX() ) || ( nSizeY > singleMask.GetSizeY() ) )
				singleMask.SetSizes( max( nSizeX, singleMask.GetSizeX() ), max( nSizeY, singleMask.GetSizeY() ) );
			singleMask.FillEvery( 1 );
			DrawLine( singleMask, v1.x - vMin.x, v1.y - vMin.y, v2.x - vMin.x, v2.y - vMin.y, 1 );
			DrawLine( singleMask, v2.x - vMin.x, v2.y - vMin.y, v3.x - vMin.x, v3.y - vMin.y, 1 );
			DrawLine( singleMask, v3.x - vMin.x, v3.y - vMin.y, v4.x - vMin.x, v4.y - vMin.y, 1 );
			DrawLine( singleMask, v4.x - vMin.x, v4.y - vMin.y, v1.x - vMin.x, v1.y - vMin.y, 1 );
			//SimpleFill( singleMask, ( v1.x + v3.x ) / 2, ( v1.y + v3.y ) / 2, 1, 1 );
			ScanFill( singleMask, 1 );
			//WiseFill( singleMask, 1 );
			const int nMinIndX = max( vMin.x, 0 );
			const int nMinIndY = max( vMin.y, 0 );
			const int nMaxIndX = min( vMin.x + nSizeX, waterBottomMap.GetSizeX() );
			const int nMaxIndY = min( vMin.y + nSizeY, waterBottomMap.GetSizeY() );
			for ( int g = nMinIndY; g < nMaxIndY; ++g )
			{
				for ( int i = nMinIndX; i < nMaxIndX; ++i )
				{
					if ( singleMask[g - vMin.y][i - vMin.x] )
						waterBottomMap[g][i] = 0;
				}
			}
		}*/
	}
	//
	for ( int g = 0; g < waterNodes.GetSizeY(); ++g )
	{
		for ( int i = 0; i < waterNodes.GetSizeX(); ++i )
		{
			if ( waterNodes[g][i] != 0 )
			{
				if ( !IsInnerPoint(waterNodes, g, i) )
				{
					waterNodes[g][i]->SetAlpha( 0.0f );
				}
			}
		}
	}
}

void CWaterController::CreatePatches( const std::vector<NWaterStuff::SWaterParams> &waterParams, const CArray2D<CPtr<SWaterNode> > &waterNodes )
{
	// Identity transform
	SFBTransform idPlace;
	Identity( &idPlace.forward );
	Identity( &idPlace.backward );

	waterPatches.resize( 0 );

	std::vector<uint8_t> useParams( 256 );

	// Divide water nodes to patches
	const int nPatchesX = waterNodes.GetSizeX() / DEF_WATER_PATCH_SIZE_X + ( (waterNodes.GetSizeX() % DEF_WATER_PATCH_SIZE_X) ? 1 : 0 );
	const int nPatchesY = waterNodes.GetSizeY() / DEF_WATER_PATCH_SIZE_Y + ( (waterNodes.GetSizeY() % DEF_WATER_PATCH_SIZE_Y) ? 1 : 0 );

	CArray2D<int> vertexInPatch( DEF_WATER_PATCH_SIZE_Y + 1, DEF_WATER_PATCH_SIZE_X + 1);

	for ( int g = 0; g < nPatchesY; ++g )
	{
		for ( int i = 0; i < nPatchesX; ++i )
		{
			// Tiles in the current water patch
			const int nFirstX = (std::max)( i * DEF_WATER_PATCH_SIZE_X, 0 );
			const int nFirstY = (std::max)( g * DEF_WATER_PATCH_SIZE_Y, 0 );
			const int nLastX = (std::min)( (i + 1) * DEF_WATER_PATCH_SIZE_X + 1, waterNodes.GetSizeX() - 1 );
			const int nLastY = (std::min)( (g + 1) * DEF_WATER_PATCH_SIZE_Y + 1, waterNodes.GetSizeY() - 1 );

			// check for used parameters
			useParams.resize( 0 );
			for ( int gg = nFirstY; gg < nLastY; ++gg )
			{
				for ( int ii = nFirstX; ii < nLastX; ++ii )
				{
					if ( waterNodes[gg][ii] )
						PushBackUnique( &useParams, waterNodes[gg][ii]->nParamInd );
				}
			}

			if ( useParams.empty() )
				continue;

			// Create patch for current water param
			for ( std::vector<uint8_t>::const_iterator it = useParams.begin(); it != useParams.end(); ++it )
			{				
				const NWaterStuff::SWaterParams &curParams = waterParams[*it];
				const float fTexCoordOffsX = 1.0f / curParams.nNumFramesX;
				const float fTexCoordOffsY = 1.0f / curParams.nNumFramesY;

				CPtr<CVisWaterPatch> pCurPatch = 0;

				CVec2 vMinBBs( FP_MAX_VALUE, FP_MAX_VALUE );
				CVec2 vMaxBBs( -FP_MAX_VALUE, -FP_MAX_VALUE );				

				// Create vertices for patch
				int nVertexId = 0;
				for ( int gg = nFirstY; gg < nLastY; ++gg )
				{
					for ( int ii = nFirstX; ii < nLastX; ++ii )
					{
						if ( !waterNodes[gg][ii] )
							continue;

						const SWaterNode *pCurNode = waterNodes[gg][ii];

						if ( !pCurPatch )
						{
							// Create patch if it is needed
							pCurPatch = new CVisWaterPatch( this, pTimer, curParams.bUseWaves,
								curParams.fHorDeformMinRadius, curParams.fHorDeformMaxRadius,
								curParams.pLight, curParams.nNumFramesX, curParams.nNumFramesY,
								!(curParams.szNoiseFileName.empty()), curParams.fNoiseCoeff, *it );

							vertexInPatch.FillEvery( -1 );
						}

						// Update mins/maxs
						const CVec3 &vPos = pCurNode->vPos;
						vMinBBs.Minimize( CVec2(vPos.x, vPos.y) );
						vMaxBBs.Maximize( CVec2(vPos.x, vPos.y) );

						// Adding this "vert" to patch
						vertexInPatch[gg - nFirstY][ii - nFirstX] = nVertexId++;

						NGScene::SVertex vertex;
						vertex.pos = vPos;

						// Calculating UV for this vertex
						float fTexX = Clamp((float)(ii - nFirstX)/DEF_WATER_PATCH_SIZE_X, 0.0f, 1.0f );
						float fTexY = Clamp((float)(gg - nFirstY)/DEF_WATER_PATCH_SIZE_Y, 0.0f, 1.0f );
						
						vertex.tex.Set( fTexX * fTexCoordOffsX , fTexY * fTexCoordOffsY );

						float fSecondTexX = GetFracPart( ii * DEF_WATER_TEX_TILE_COEFF * fTexCoordOffsX ) * 2;
						float fSecondTexY = GetFracPart( gg * DEF_WATER_TEX_TILE_COEFF * fTexCoordOffsY ) * 2;
						pCurPatch->data.secondTex.push_back( CVec2( fSecondTexX, fSecondTexY ) );

						// Put wave params to V vec and normal
						CVec3 vPhase(pCurNode->waveParams[0].fPhase, pCurNode->waveParams[1].fPhase, pCurNode->waveParams[2].fPhase);
						CVec3 vAmplitude(pCurNode->waveParams[0].fAmplitude, pCurNode->waveParams[1].fAmplitude, pCurNode->waveParams[2].fAmplitude);

						vPhase =  vPhase * 255.0f;
						vAmplitude = vAmplitude*0.5f*255.0f;

						vertex.texV.x = vAmplitude.x;
						vertex.texV.y = vAmplitude.y;
						vertex.texV.z = vAmplitude.z;

						vertex.normal.x = vPhase.x;
						vertex.normal.y = vPhase.y;
						vertex.normal.z = vPhase.z;

						vertex.normal.w = pCurNode->nAlpha;

						// Find vertex color
						CVec3 vCol( 0x3f, 0x3f, 0x3f );
						int nColor, cr, cg, cb;

						// Light
						if ( curParams.pLight )
						{
							const CVec3 &vSunDirAmbient = curParams.pLight->vAmbientColor;
							const CVec3 &vSunDirDiffuse = curParams.pLight->vLightColor;

							const float fCos = -cos( ToRadian( curParams.pLight->fPitch ) );
							vCol.Lerp( fCos, vSunDirAmbient, vSunDirDiffuse );
							vCol *= 255;
						}

						// Noise
						if ( !curParams.szNoiseFileName.empty() )
						{
							const CArray2D<SColor24> &curNoise = noises[*it];							
							const SColor24 noiseCol = curNoise[gg%curNoise.GetSizeY()][ii%curNoise.GetSizeX()];
							const float fNoiseCoeff = curParams.fNoiseCoeff;

							cr = Clamp( Float2Int( vCol.x*( fNoiseCoeff * noiseCol.r * DEF_INV_255 + 1.0f - fNoiseCoeff ) ), 0, 255 );
							cg = Clamp( Float2Int( vCol.y*( fNoiseCoeff * noiseCol.g * DEF_INV_255 + 1.0f - fNoiseCoeff ) ), 0, 255 );
							cb = Clamp( Float2Int( vCol.z*( fNoiseCoeff * noiseCol.b * DEF_INV_255 + 1.0f - fNoiseCoeff ) ), 0, 255 );
						}
						else
						{
							cr = Clamp( Float2Int( vCol.x ), 0, 255 );
							cg = Clamp( Float2Int( vCol.y ), 0, 255 );
							cb = Clamp( Float2Int( vCol.z ), 0, 255 );
						}

						nColor = ( cr << 16 ) | ( cg << 8 ) | cb;

						vertex.texU.dw = nColor;

						pCurPatch->data.vertices.push_back( vertex );						
					}
				}

				if ( !pCurPatch )
					continue;

				if ( vMaxBBs.x - vMinBBs.x < FP_EPSILON || vMaxBBs.y - vMinBBs.y < FP_EPSILON )
				{
					pCurPatch = 0;
					continue;
				}

				// Initialize triangles
				for ( int gg = 0; gg < nLastY-nFirstY-1; ++gg )
				{
					for ( int ii = 0; ii < nLastX-nFirstX-1; ++ii )
					{
						int nIndex00 = vertexInPatch[gg][ii];
						int nIndex01 = vertexInPatch[gg][ii+1];
						int nIndex10 = vertexInPatch[gg+1][ii];
						int nIndex11 = vertexInPatch[gg+1][ii+1];

						if ( nIndex00 != -1 && nIndex01  != -1 && nIndex11 != -1 &&
							( pCurPatch->data.vertices[nIndex00].normal.w != 0 || pCurPatch->data.vertices[nIndex01].normal.w != 0 || pCurPatch->data.vertices[nIndex11].normal.w != 0) ) 
						{
							pCurPatch->data.triangles.push_back( STriangle( nIndex00, nIndex01, nIndex11 ) );
						}

						if ( nIndex00 != -1 && nIndex11 != -1 && nIndex10 != -1 &&
							( pCurPatch->data.vertices[nIndex00].normal.w != 0 || pCurPatch->data.vertices[nIndex11].normal.w != 0 || pCurPatch->data.vertices[nIndex10].normal.w != 0) ) 
						{
							pCurPatch->data.triangles.push_back( STriangle( nIndex00, nIndex11, nIndex10 ) );
						}
					}
				}

				if ( pCurPatch->data.triangles.empty() )					
				{
					pCurPatch = 0;
					continue;
				}

				// Patch has content so add it to scene
				NGScene::SMaterialCreateInfo waterMatInfo;
				pGScene->CreateMaterialInfo( curParams.pMaterial, &waterMatInfo );
				NGScene::IGameView::SMeshInfo meshInfo;

				meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( pCurPatch,
					GetAnimatedMaterial( SAnimWaterMaterialKey( waterMatInfo.pTexture, waterMatInfo.pBump,
					waterMatInfo.nPriority, pTimer, false, waterParams[*it].nNumFramesX, waterParams[*it].nNumFramesY, waterMatInfo.bApplyFog, waterMatInfo.bAddPlaced, true ) ) ) );

				NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING | NGScene::LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY,	0), 0, 0 );

				waterPatches.push_back( CVisWaterPatchHolder() );
				waterPatches.back().pPatch = pCurPatch;
				waterPatches.back().pHolder = pGScene->CreateMesh( meshInfo, idPlace, 0, 0, room );
			}

			/*
			fill( minBBs.begin(), minBBs.end(), CVec2(FP_MAX_VALUE, FP_MAX_VALUE) );
			fill( maxBBs.begin(), maxBBs.end(), CVec2(-FP_MAX_VALUE, -FP_MAX_VALUE) );

			param2patch.clear();

			for ( vector<uint8_t>::const_iterator it = useParams.begin(); it != useParams.end(); ++it )
			{
				waterPatches.push_back( CVisWaterPatchHolder() );
				const NWaterStuff::SWaterParams &curParams = waterParams[*it];
				waterPatches.back().pPatch = new CVisWaterPatch( this, pTimer, curParams.bUseWaves,
					curParams.fHorDeformMinRadius, curParams.fHorDeformMaxRadius,
					curParams.pLight, curParams.nNumFramesX, curParams.nNumFramesY,
					!(curParams.szNoiseFileName.empty()), curParams.fNoiseCoeff, *it );
				param2patch[*it] = waterPatches.size() - 1;
			}

			for ( int gg = nFirstY; gg <= nLastY; ++gg )
			{
				for ( int ii = nFirstX; ii <= nLastX; ++ii )
				{
					if ( waterNodes[gg][ii] )
					{

						const uint8_t nParam = waterNodes[gg][ii]->nParamInd;
						CVisWaterPatch *pCurPatch = waterPatches[param2patch[nParam]].pPatch;

						pCurPatch->verts.push_back( CVec2i(ii, gg) );

						const CVec3 &vPos = waterNodes[gg][ii]->vPos;
						minBBs[nParam].Minimize( CVec2(vPos.x, vPos.y) );
						maxBBs[nParam].Maximize( CVec2(vPos.x, vPos.y) );
					}
				}
			}

			for ( vector<uint8_t>::const_iterator it = useParams.begin(); it != useParams.end(); ++it )
			{
				const int nCurPatchNum = param2patch[*it];
				CVisWaterPatch *pCurPatch = waterPatches[nCurPatchNum].pPatch;
				// initialize vertices
				pCurPatch->data.vertices.resize( pCurPatch->verts.size() );
				for ( int iVertex = 0; iVertex < pCurPatch->data.vertices.size(); ++iVertex )
				{
					NGScene::SVertex &vertex = pCurPatch->data.vertices[iVertex];
					vertex.pos.Set( 0, 0, 0 );

					float fTexX = GetFracPart( (float)pCurPatch->verts[iVertex].x/DEF_WATER_TEX_TILES_PER_TEX );
					float fTexY = GetFracPart( (float)pCurPatch->verts[iVertex].y/DEF_WATER_TEX_TILES_PER_TEX );

					const NWaterStuff::SWaterParams &curParams = waterParams[*it];
					const float fTexCoordOffsX = 1.0f / curParams.nNumFramesX;
					const float fTexCoordOffsY = 1.0f / curParams.nNumFramesY;

					vertex.tex.Set( fTexX * fTexCoordOffsX , fTexY * fTexCoordOffsY );
					//vertex.tex.Set( 0, 0 );

					CalcCompactVector( &(vertex.normal), CVec3(0, 0, 1) );						
					vertex.texU.dw = 0x00ffffff;
					vertex.texV.dw = 0;
				}
				pCurPatch->data.secondTex.resize( pCurPatch->verts.size() );
				for ( vector<CVec2>::iterator itVert = pCurPatch->data.secondTex.begin(); itVert != pCurPatch->data.secondTex.end(); ++itVert )
				{
					itVert->Set( 0, 0 );
				}

				// initialize triangles
				pCurPatch->data.triangles.reserve( (DEF_WATER_PATCH_SIZE_X + 2) * (DEF_WATER_PATCH_SIZE_Y + 2) * 2 );
				pCurPatch->data.triangles.resize( 0 );
				pCurPatch->realTrgs.reserve( DEF_WATER_PATCH_SIZE_X * DEF_WATER_PATCH_SIZE_Y * 2 );
				pCurPatch->realTrgs.resize( 0 );
				for ( int gg = nFirstY; gg < nLastY; ++gg )
				{
					for ( int ii = nFirstX; ii < nLastX; ++ii )
					{
						if ( waterNodes[gg][ii] && waterNodes[gg][ii + 1] && waterNodes[gg + 1][ii + 1] && waterNodes[gg + 1][ii] &&
							(waterNodes[gg][ii]->nParamInd == *it) && (waterNodes[gg][ii + 1]->nParamInd == *it) &&
							(waterNodes[gg + 1][ii]->nParamInd == *it) && (waterNodes[gg + 1][ii + 1]->nParamInd == *it) )
						{
							const int nInd1 = FindRealIndex( CVec2i(ii, gg), pCurPatch->verts );
							const int nInd2 = FindRealIndex( CVec2i(ii + 1, gg), pCurPatch->verts );
							const int nInd3 = FindRealIndex( CVec2i(ii + 1, gg + 1), pCurPatch->verts );
							const int nInd4 = FindRealIndex( CVec2i(ii, gg + 1), pCurPatch->verts );

							pCurPatch->data.triangles.push_back( STriangle(nInd1, nInd2, nInd3) );
							pCurPatch->data.triangles.push_back( STriangle(nInd3, nInd4, nInd1) );

							if ( (ii > nFirstX) && (ii < (nLastX - 1)) && (gg > nFirstY) && (gg < (nLastY - 1)) )
							{
								pCurPatch->realTrgs.push_back( STriangle(nInd1, nInd2, nInd3) );
								pCurPatch->realTrgs.push_back( STriangle(nInd3, nInd4, nInd1) );
							}
						}
					}
				}

				//bound.BoxInit( CVec3( vBBMin.x, vBBMin.y, 0.0f ), CVec3( vBBMax.x, vBBMax.y, 1.0f ) );
				if ( maxBBs[*it].x - minBBs[*it].x < FP_EPSILON || maxBBs[*it].y - minBBs[*it].y < FP_EPSILON )
					continue;

				SBound bound;
				bound.BoxInit( CVec3(minBBs[*it].x, minBBs[*it].y, 0.0f), CVec3(maxBBs[*it].x, maxBBs[*it].y, 2.0f) );
				waterPatchesBounds.push_back( new CCSBound() );
				waterPatchesBounds.back()->Set( bound );

				NGScene::SFullRoomInfo room( NGScene::SRoomInfo((waterParams[*it].pLight != 0) ? (NGScene::LF_SKIP_LIGHTING | NGScene::LF_NEVER_STATIC) : NGScene::LF_NEVER_STATIC,	-100), 0, 0 );
				//NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_NEVER_STATIC, -100 ), 0, 0 );
				if ( pGScene && waterParams[*it].pMaterial )
				{
					NGScene::SMaterialCreateInfo waterMatInfo;
					pGScene->CreateMaterialInfo( waterParams[*it].pMaterial, &waterMatInfo );
					NGScene::IGameView::SMeshInfo meshInfo;
#pragma message(TODO_sharelky_dlya_materialov)
					meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( pCurPatch,
						new NGScene::CAnimWaterMaterial( waterMatInfo.pTexture, waterMatInfo.pBump,
						waterMatInfo.nPriority, pTimer, false, waterParams[*it].nNumFramesX, waterParams[*it].nNumFramesY, waterMatInfo.bApplyFog, waterMatInfo.bAddPlaced, false ) ) );

					waterPatches[nCurPatchNum].pHolder = pGScene->CreateMesh( meshInfo, idPlace, 0, 0 );
				}
				*/
			
		}
	}
}

static void CreateAlphaMap( CArray2D<uint8_t> *pAlphaMap, const CArray2D<uint8_t> &seaMap, const std::vector<NWaterStuff::SSurfBorder> &borders )
{
	NI_VERIFY( pAlphaMap, "Invalid pointer", return )

	pAlphaMap->SetSizes( seaMap.GetSizeX(), seaMap.GetSizeY() );
	pAlphaMap->FillEvery( 0xff );

	float fAlpha;
	static std::vector<CVec3fEx> coastPoints( 4096 );
	SWaterAlphaSmoothProfile waterAlphaSmoothProfile;

	for ( int k = 0; k < borders.size(); ++k )
	{
		const std::vector<NDb::SVSOPoint> &curBorder = borders[k].points;
		if ( curBorder.size() < 2 )
			continue;
		const bool bNeedCycle = ( fabs(curBorder[0].vPos.x - curBorder[curBorder.size() - 1].vPos.x) < DEF_EPS ) &&
														( fabs(curBorder[0].vPos.y - curBorder[curBorder.size() - 1].vPos.y) < DEF_EPS );

		if ( bNeedCycle )
			coastPoints.resize( (curBorder.size() - 1) * 4 + 2 );
		else
			coastPoints.resize( (curBorder.size() - 1) * 4 );

		const int nCoastSize = coastPoints.size();
		int nInd = 0;
		for ( int i = 0; i < (curBorder.size() - 1); ++i )
		{
			const CVec3 &vCurPos = curBorder[i].vPos;
			const CVec3 &vNextPos = curBorder[i + 1].vPos;
			const CVec3 &vCurNorm = curBorder[i].vNorm;
			const CVec3 &vNextNorm = curBorder[i + 1].vNorm;
			//const float &fCurWidth = AI2Vis( curBorder[i].fWidth );
			//const float &fNextWidth = AI2Vis( curBorder[i + 1].fWidth );
			const float &fCurWidth = DEF_WATER_BLENDING_DIST;
			const float &fNextWidth = DEF_WATER_BLENDING_DIST;
			//
			coastPoints[nInd].Set( AI2Vis(vCurPos.x), AI2Vis(vCurPos.y), 1.0f, 1 );
			coastPoints[nCoastSize - 1 - nInd].Set( coastPoints[nInd].x + vCurNorm.x * fCurWidth,
																							coastPoints[nInd].y + vCurNorm.y * fCurWidth, 1.0f, 1 );
			++nInd;
			coastPoints[nInd].Set( AI2Vis(vNextPos.x), AI2Vis(vNextPos.y), 1.0f, 1 );
			coastPoints[nCoastSize - 1 - nInd].Set( coastPoints[nInd].x + vNextNorm.x * fNextWidth,
																							coastPoints[nInd].y + vNextNorm.y * fNextWidth, 1.0f, 1 );
			++nInd;
		}

		if ( bNeedCycle )
		{
			coastPoints[nInd] = coastPoints[0];
			coastPoints[nCoastSize - 1 - nInd] = coastPoints[nCoastSize - 1];
		}

		CVec3 vBBMin( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
		CVec3 vBBMax( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
		for ( std::vector<CVec3fEx>::const_iterator it = coastPoints.begin(); it != coastPoints.end(); ++it )
		{
			const CVec3 vPos( it->x, it->y, 0 );
			vBBMin.Minimize( vPos );
			vBBMax.Maximize( vPos );
		}

		for ( int g = 0; g < pAlphaMap->GetSizeY(); ++g )
		{
			for ( int i = 0; i < pAlphaMap->GetSizeX(); ++i )
			{
				const CVec2 vPos( (float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE );
				if ( (vPos.x >= vBBMin.x) && (vPos.y >= vBBMin.y) && (vPos.x <= vBBMax.x) && (vPos.y <= vBBMax.y) )
				{
					if ( GetIncRidgeHeightMinLaw(CVec2(vPos.x, vPos.y), coastPoints, &fAlpha, waterAlphaSmoothProfile) )
					{
						const int nAlpha = Clamp( Float2Int(fAlpha * 254.0f), 0, 254 );
						if ( (*pAlphaMap)[g][i] == 0xff )
							(*pAlphaMap)[g][i] = nAlpha;
						else
							(*pAlphaMap)[g][i] = ( int((*pAlphaMap)[g][i]) + nAlpha ) >> 1;
					}
				}
			}
		}
	}
}

class CShaderWater : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_NOCOPY_METHODS(CShaderWater)
	ZDATA
	int nOriginX, nOriginY;
	CArray2D<uint8_t> alphaMap;
	float fSpeed;
	float fRadius;
	int nNumFramesX;
	int nNumFramesY;
	ZEND 
	void Recalc();
protected:
	CShaderWater() {}
public:
	CShaderWater( int _nOriginX, int _nOriginY,
								const CArray2D<uint8_t> &_alphaMap,
								float _fSpeed, float _fRadius, int _nNumFramesX, int _nNumFramesY )
	: nOriginX( _nOriginX ),
	nOriginY( _nOriginY ),
	alphaMap( _alphaMap ),
	fSpeed( _fSpeed ),
	fRadius( _fRadius ),
	nNumFramesX( _nNumFramesX ),
	nNumFramesY( _nNumFramesY )
	{
	}
};

typedef std::unordered_map<int, int> CVertsHash;

static CArray2D<float> randOffsets;

inline float GetRandomOffset( int x, int y )
{
	if ( randOffsets.IsEmpty() )
	{
		randOffsets.SetSizes( 512, 512 );
		for ( int g = 0; g < randOffsets.GetSizeY(); ++g )
		{
			for ( int i = 0; i < randOffsets.GetSizeX(); ++i )
			{
				randOffsets[g][i] = NWin32Random::Random( 0.0f, 1.0f );
			}
		}
	}

	return randOffsets[y & ( randOffsets.GetSizeY() - 1 )][x & ( randOffsets.GetSizeX() - 1 )];
}

inline int AddNewVertex(	NGScene::CObjectInfo::SData *pData,
													int nInd,
													int nOffsX, int nOffsY,
													int x, int y,
													float fTexScaleX, float fTexScaleY,
													uint8_t cFade,
													float fWaterHeight,
													float fRadius, float fSpeed )
{
	const float fOffsX = (float)nOffsX * DEF_TILE_SIZE;
	const float fOffsY = (float)nOffsY * DEF_TILE_SIZE;

	NGScene::SVertex v;
	CalcCompactVector( &(v.texU), CVec3(1, 0, 0) );
	CalcCompactVector( &(v.texV), CVec3(0, 1, 0) );
	CalcCompactVector( &(v.normal), CVec3(0, 0, 1) );
	v.normal.w = cFade;
	v.pos.Set( fOffsX + (float)x * DEF_TILE_SIZE, fOffsY + (float)y * DEF_TILE_SIZE + fRadius, fWaterHeight );
	v.tex.Set( (float)x * fTexScaleX, (float)y * fTexScaleY );
	pData->verts.push_back( v );
	pData->secondTex.push_back( CVec2( GetRandomOffset( (x + nOffsX) << 1, y + nOffsY ), fSpeed ) );
	return pData->verts.size() - 1;
}

inline int AddNewMiddleVertex(	NGScene::CObjectInfo::SData *pData,
																int nInd,
																int nOffsX, int nOffsY,
																int x, int y,
																float fTexScaleX, float fTexScaleY,
																uint8_t cFade,
																float fWaterHeight,
																float fRadius, float fSpeed )
{
	const float fOffsX = (float)nOffsX * DEF_TILE_SIZE;
	const float fOffsY = (float)nOffsY * DEF_TILE_SIZE;

	NGScene::SVertex v;
	CalcCompactVector( &(v.texU), CVec3(1, 0, 0) );
	CalcCompactVector( &(v.texV), CVec3(0, 1, 0) );
	CalcCompactVector( &(v.normal), CVec3(0, 0, 1) );
	v.normal.w = cFade;
	v.pos.Set( fOffsX + ( (float)x + 0.5f ) * DEF_TILE_SIZE, fOffsY + ( (float)y + 0.5f ) * DEF_TILE_SIZE + fRadius, fWaterHeight );
	v.tex.Set( ( (float)x + 0.5f ) * fTexScaleX, ( (float)y + 0.5f ) * fTexScaleY );
	pData->verts.push_back( v );
	pData->secondTex.push_back( CVec2( GetRandomOffset( ( (x + nOffsX) << 1 ) + 1, y + nOffsY ), fSpeed ) );
	return pData->verts.size() - 1;
}

inline int GetVertexIndex(	CVertsHash *pVertsHash,
														NGScene::CObjectInfo::SData *pData,
														int nInd,
														int nOffsX, int nOffsY,
														int x, int y,
														float fTexScaleX, float fTexScaleY,
														uint8_t cFade,
														float fWaterHeight,
														float fRadius, float fSpeed )
{
	CVertsHash::iterator it = pVertsHash->find( nInd );
	if ( it == pVertsHash->end() )
	{
		const int nAddInd = AddNewVertex( pData, nInd, nOffsX, nOffsY, x, y, fTexScaleX, fTexScaleY, cFade, fWaterHeight, fRadius, fSpeed );
		(*pVertsHash)[nInd] = nAddInd;
		return nAddInd;
	}
	else
		return it->second;
}

void CShaderWater::Recalc()
{
	if ( !pValue )
		pValue = new NGScene::CObjectInfo;

	NGScene::CObjectInfo::SData data;
	data.verts.reserve( 256 );
	data.geometry.reserve( 256 );
	data.secondTex.reserve( 256 );

	CVertsHash vertsHash( 256 );

	const int nRaw = alphaMap.GetSizeX();
	int nInd1, nInd2, nInd3, nInd4, nInd5;

	const float fTexScaleX = 1.0f / ( alphaMap.GetSizeX() - 1 ) / nNumFramesX;
	const float fTexScaleY = 1.0f / ( alphaMap.GetSizeY() - 1 ) / nNumFramesY;
	const float fWaterHeight = 0.1f;

	for ( int g = 0; g < ( alphaMap.GetSizeY() - 1 ); ++g )
	{
		for ( int i = 0; i < ( alphaMap.GetSizeX() - 1 ); ++i )
		{
			const uint8_t &cAlp1 = alphaMap[g][i];
			const uint8_t &cAlp2 = alphaMap[g][i + 1];
			const uint8_t &cAlp3 = alphaMap[g + 1][i + 1];
			const uint8_t &cAlp4 = alphaMap[g + 1][i];
			if ( cAlp1 || cAlp2 || cAlp3 || cAlp4 )
			{
				const int nInd = g * nRaw + i;
				nInd1 = GetVertexIndex( &vertsHash, &data, nInd, nOriginX, nOriginY, i, g, fTexScaleX, fTexScaleY, cAlp1, fWaterHeight, fRadius, fSpeed );
				nInd2 = GetVertexIndex( &vertsHash, &data, nInd + 1, nOriginX, nOriginY, i + 1, g, fTexScaleX, fTexScaleY, cAlp2, fWaterHeight, fRadius, fSpeed );
				nInd3 = GetVertexIndex( &vertsHash, &data, nInd + 1 + nRaw, nOriginX, nOriginY, i + 1, g + 1, fTexScaleX, fTexScaleY, cAlp3, fWaterHeight, fRadius, fSpeed );
				nInd4 = GetVertexIndex( &vertsHash, &data, nInd + nRaw, nOriginX, nOriginY, i, g + 1, fTexScaleX, fTexScaleY, cAlp4, fWaterHeight, fRadius, fSpeed );
				nInd5 = AddNewMiddleVertex( &data, nInd, nOriginX, nOriginY, i, g, fTexScaleX, fTexScaleY, ((int)cAlp1 + cAlp2 + cAlp3 + cAlp4) >> 2, fWaterHeight, fRadius, fSpeed );

				data.geometry.push_back( STriangle( nInd5, nInd1, nInd2 ) );
				data.geometry.push_back( STriangle( nInd5, nInd2, nInd3 ) );
				data.geometry.push_back( STriangle( nInd5, nInd3, nInd4 ) );
				data.geometry.push_back( STriangle( nInd5, nInd4, nInd1 ) );
			}
		}
	}

	/*
	if ( 0 && !szNoiseFileName.empty() )
	{	
		CFileStream noiseStream( NVFS::GetMainVFS(), szNoiseFileName );
		if ( noiseStream.IsOk() )
		{
			data.attributes.resize( 1 );
			NGScene::CObjectInfo::SStream &attr = data.attributes[0];

			attr.nID = NGScene::GATTR_VERTEX_COLOR;
			attr.data.resize( data.verts.size() );
			CVec3 vCol( 1.0f, 1.0f, 1.0f );

			CArray2D<SColor24> noise;
			NImage::LoadTGAImage( noise, &noiseStream );

			const float fStep = 3.0f;
			for ( int i = 0; i < data.verts.size(); ++i )
			{
				const int nNoiseU = ((int)(data.verts[i].pos.x/fStep)) & 0xffff;
				const int nNoiseV = ((int)(data.verts[i].pos.y/fStep)) & 0xffff;

				const int nNoiseU1 = (nNoiseU+1) & 0xffff;
				const int nNoiseV1 = (nNoiseV+1) & 0xffff;

				float fFractionX = fmod(data.verts[i].pos.x/fStep, 0xffff) - nNoiseU;
				float fFractionY = fmod(data.verts[i].pos.y/fStep, 0xffff) - nNoiseV;

				SColor24 NoiseCol = noise[ nNoiseV	& ( noise.GetSizeY() - 1 )][ nNoiseU & ( noise.GetSizeX() - 1 )];
				SColor24 NoiseColY = noise[ nNoiseV1	& ( noise.GetSizeY() - 1 )][ nNoiseU & ( noise.GetSizeX() - 1 )];
				SColor24 NoiseColX = noise[ nNoiseV	& ( noise.GetSizeY() - 1 )][ nNoiseU1 & ( noise.GetSizeX() - 1 )];
				SColor24 NoiseColXY = noise[ nNoiseV1	& ( noise.GetSizeY() - 1 )][ nNoiseU1 & ( noise.GetSizeX() - 1 )];

				const uint8_t r = NoiseCol.r*(1-fFractionX)*(1-fFractionY) + NoiseColX.r*fFractionX*(1-fFractionY)+NoiseColY.r*(1-fFractionX)*fFractionY + NoiseColXY.r*fFractionX*fFractionY;
				const uint8_t g = NoiseCol.g*(1-fFractionX)*(1-fFractionY) + NoiseColX.g*fFractionX*(1-fFractionY)+NoiseColY.g*(1-fFractionX)*fFractionY + NoiseColXY.g*fFractionX*fFractionY;
				const uint8_t b = NoiseCol.b*(1-fFractionX)*(1-fFractionY) + NoiseColX.b*fFractionX*(1-fFractionY)+NoiseColY.b*(1-fFractionX)*fFractionY + NoiseColXY.b*fFractionX*fFractionY;

				uint32_t nColor = 0xff000000 | ( r << 16 ) | ( g << 8 ) | b;
				attr.data[i] = nColor;
			}
		}
	}
	*/

	pValue->Assign( data, true );
}

void CWaterController::InitSilentWater( const CArray2D<uint8_t> &seaMap,
																				const std::vector<NWaterStuff::SWaterParams> &waterParams,
																				const std::vector<NWaterStuff::SSurfBorder> &waterBorders,
																				NGScene::IGameView *_pGScene,
																				const NDb::SWater *pWater )
{
	const int nPatchSize = pWater->nTilesNumPerWaterTexture;

	CArray2D<uint8_t> locMap( nPatchSize + 1, nPatchSize + 1 ), alphaMap;
	CreateAlphaMap( &alphaMap, seaMap, waterBorders );

	const int nNumPatchesX = seaMap.GetSizeX() / nPatchSize + ( ( ( seaMap.GetSizeX() % nPatchSize ) == 0 ) ? 0 : 1 );
	const int nNumPatchesY = seaMap.GetSizeY() / nPatchSize + ( ( ( seaMap.GetSizeY() % nPatchSize ) == 0 ) ? 0 : 1 );
	shaderWaterPatches.resize( 0 );

	SFBTransform idPlace;
	Identity( &idPlace.forward );
	Identity( &idPlace.backward );

	bool bFlag;
	for ( int k = 0; k < waterParams.size(); ++k )
	{
		const NWaterStuff::SWaterParams &params = waterParams[k];

		for ( int g = 0; g < nNumPatchesY; ++g )
		{
			for ( int i = 0; i < nNumPatchesX; ++i )
			{
				const int nX1 = i * nPatchSize;
				const int nY1 = g * nPatchSize;
				const int nX2 = (std::min)( ( i + 1 ) * nPatchSize, seaMap.GetSizeX() - 1 );
				const int nY2 = (std::min)( ( g + 1 ) * nPatchSize, seaMap.GetSizeY() - 1 );

				locMap.FillZero();
				bFlag = false;

				for ( int y = nY1; y <= nY2; ++y )
				{
					for ( int x = nX1; x <= nX2; ++x )
					{
						if ( seaMap[y][x] == params.nSeaMapIndex && alphaMap[y][x] )
						{
							locMap[y - nY1][x - nX1] = alphaMap[y][x];
							bFlag = true;
						}
					}
				}

				if ( bFlag )
				{
					NGScene::SMaterialCreateInfo waterMatInfo;
					_pGScene->CreateMaterialInfo( params.pMaterial, &waterMatInfo );
					NGScene::IGameView::SMeshInfo meshInfo;
					meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( new CShaderWater( nX1, nY1, locMap,
						( params.fHorDeformRotationSpeedMin + params.fHorDeformRotationSpeedVariation * 0.5f ) * 4000.0f,
						( params.fHorDeformMinRadius + params.fHorDeformMaxRadius ) * 0.5f, params.nNumFramesX, params.nNumFramesY ),
						GetAnimatedMaterial( SAnimWaterMaterialKey( waterMatInfo.pTexture, waterMatInfo.pBump,
						waterMatInfo.nPriority, pTimer, false, params.nNumFramesX, params.nNumFramesY, waterMatInfo.bApplyFog, waterMatInfo.bAddPlaced, false ) ) ) );
					shaderWaterPatches.push_back( _pGScene->CreateMesh( meshInfo, idPlace, 0, 0 ) );
				}
			}
		}
	}
}

void CWaterController::InitOceanWater( const CArray2D<uint8_t> &seaMap,
																			 const std::vector<NWaterStuff::SWaterParams> &waterParams,
																			 const std::vector<NWaterStuff::SSurfBorder> &waterBorders,
																			 NGScene::IGameView *_pGScene,
																			 const NDb::SWater *pWater,
																			 float fWinterDirection )
{
	// init waves
	waves.resize( DEF_WAVES_NUM );
	for ( std::vector<SWaveType>::iterator it = waves.begin(); it != waves.end(); ++it )
	{
		it->fAmplitude = DEF_WAVES_AMPLITUDE;
		it->fDeepWaveNumber = DEF_WAVES_DEEP_WAVE_NUMBER;
		it->fPeriod = DEF_WAVES_PERIOD;
		it->fInvPeriod = 1.0f / it->fPeriod;
	}

	// Find center for rotating water mesh around it
	const int nSrcHalfX = seaMap.GetSizeX() / 2 + ( (seaMap.GetSizeX() & 1) ? 1 : 0 );
	const int nSrcHalfY = seaMap.GetSizeY() / 2 + ( (seaMap.GetSizeY() & 1) ? 1 : 0 );

	const CVec2 vCenter( (float)nSrcHalfX * DEF_TILE_SIZE, (float)nSrcHalfY * DEF_TILE_SIZE );

	const int nDstHalfX = (float)nSrcHalfX * FP_SQRT_2 + 1;
	const int nDstHalfY = (float)nSrcHalfY * FP_SQRT_2 + 1;

	const CVec2 vOrg( vCenter.x - (float)nDstHalfX * DEF_TILE_SIZE, vCenter.y - (float)nDstHalfY * DEF_TILE_SIZE );

	// Tempary buffer to generate water params
	CArray2D<CPtr<SWaterNode> > waterNodes( nDstHalfX * 2, nDstHalfY * 2 );

	fWaterTexTileCoeff = 1.0f / pWater->nTilesNumPerWaterTexture;
	fFoamYAnim = DEF_FOAM_Y_ANIM * DEF_WATER_TEX_TILES_PER_TEX / pWater->nTilesNumPerWaterTexture;

	CVec2 vRotPos;

	float fTexX;
	float fTexY = 0.0f;
	int nWaterParamsInd;

	// create used nodes
	for ( int g = 0; g < waterNodes.GetSizeY(); ++g )
	{
		fTexX = 0.0f;
		for ( int i = 0; i < waterNodes.GetSizeX(); ++i )
		{
			const CVec2 vPos( vOrg.x + (float)i * DEF_TILE_SIZE, vOrg.y + (float)g * DEF_TILE_SIZE );
			RotateVector( &vRotPos, vPos, vCenter, cos(fWinterDirection), sin(fWinterDirection) );
			const int nIndX = vRotPos.x * DEF_INV_TILE_SIZE;
			const int nIndY = vRotPos.y * DEF_INV_TILE_SIZE;
			if ( (nIndX >= 0) && (nIndX < seaMap.GetSizeX()) && (nIndY >= 0) && (nIndY < seaMap.GetSizeY()) && (seaMap[nIndY][nIndX]) &&
				((nWaterParamsInd = FindWaterParamInd(seaMap[nIndY][nIndX], waterParams)) >= 0) )
			{
				waterNodes[g][i] = new SWaterNode;

				//const int nWaterParamsInd = min( seaMap[nIndY][nIndX] - 1, nonShaderWaterParams.size() - 1 );
				const NWaterStuff::SWaterParams &params = waterParams[nWaterParamsInd];
				waterNodes[g][i]->nParamInd = nWaterParamsInd;
				const float fTexCoordOffsX = 1.0f / params.nNumFramesX;
				const float fTexCoordOffsY = 1.0f / params.nNumFramesY;

				waterNodes[g][i]->vPos.Set( vRotPos.x, vRotPos.y, 0.1f );
				waterNodes[g][i]->vTex.Set( fTexX * fTexCoordOffsX ,
					fTexY * fTexCoordOffsY );
				waterNodes[g][i]->SetAlpha( 2.0f ); // for interference properties
				waterNodes[g][i]->bIsLargeAlpha = true;

				waterNodes[g][i]->bBorderTexX = fabs( waterNodes[g][i]->vTex.x - fTexCoordOffsX ) < DEF_TEX_ERROR;
				waterNodes[g][i]->bBorderTexY = fabs( waterNodes[g][i]->vTex.y - fTexCoordOffsY ) < DEF_TEX_ERROR;

				waterNodes[g][i]->waveParams.resize( waves.size() );
			}
			if ( fTexX > (1.0f - DEF_TEX_ERROR) )
				fTexX = 0.0f;
			fTexX += fWaterTexTileCoeff;
		}
		if ( fTexY > (1.0f - DEF_TEX_ERROR) )
			fTexY = 0.0f;
		fTexY += fWaterTexTileCoeff;
	}

	// load noises
	SColor24 whiteCol;
	whiteCol.r = whiteCol.g = whiteCol.b = 0xff;
	noises.resize( waterParams.size() );
	for ( int i = 0; i < waterParams.size(); ++i )
	{
		if ( !(waterParams[i].szNoiseFileName.empty()) )
		{
			CFileStream noiseStream( NVFS::GetMainVFS(), waterParams[i].szNoiseFileName );
			NI_ASSERT( noiseStream.IsOk(), fmt::format("Can't load water noise {}", waterParams[i].szNoiseFileName) );
			if ( noiseStream.IsOk() )
				NImage::LoadTGAImage( noises[i], &noiseStream );
			else
			{
				noises[i].SetSizes( 1, 1 );
				noises[i][0][0] = whiteCol;
			}
		}
	}

	ProcessWavesDistribution( &waterNodes );
	SetBorders( &waterNodes, waterBorders );
	CreatePatches( waterParams, waterNodes );
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x130C8300, SWaterNode );


