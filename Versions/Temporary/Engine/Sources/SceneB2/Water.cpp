#include "stdafx.h"

#include "Water.h"
#include "B2_M1_Terrain/DBPreLight.h"
#include "B2_M1_Terrain/DBWater.h"
#include "Image/Targa.h"
#include "System/FastMath.h"
#include "Misc/Win32Random.h"
#include "TerrainInfo.h"
#include "Scene.h"
#include "System/VFSOperations.h"

#include <boost/config.hpp>

#define DEF_INV_255 ( 1.0f / 255.0f )
#define DEF_HEIGHT_BIAS 0.1f

#define DEF_NEW_TIME ( 1.0f / 50.0f )

#define DEF_TEX_ERROR 0.001f

#define DEF_COASTES_NUM 2

#define DEF_WATER_TEX_TILES_PER_TEX 2
#define DEF_WATER_TEX_TILE_COEFF (1.0f / DEF_WATER_TEX_TILES_PER_TEX)
#define DEF_WATER_TEX_TILE_COEFF2 0.1f

#define DEF_WATER_TEX_ANIM_NUM 4
#define DEF_WATER_TEX_COORD_OFFSET (1.0f / DEF_WATER_TEX_ANIM_NUM)
#define DEF_WATER_TEX_COORD_OFFSET2 (1.0f / DEF_WATER_TEX_ANIM_NUM - DEF_TEX_ERROR * 2.0f)

#define DEF_WAVES_NUM 10
#define DEF_WAVES_AMPLITUDE 1.4f
#define DEF_WAVES_DEEP_WAVE_NUMBER 0.1f
#define DEF_WAVES_PERIOD 0.3f
#define DEF_WAVES_PERIOD_VARIATION -0.15f
#define DEF_WAVES_DIRECTION_VARIATION 0.0f

#define DEF_WAVES_SCALE_FACTOR 0.6f
#define DEF_WAVES_SINGLE_WAVE_LEN 25

#define DEF_WAVE_DISTR_STEP 0.5f
#define DEF_WAVE_LEN ( DEF_WAVE_DISTR_STEP * 10.0f )
#define DEF_WAVE_LEN_VARIATION 0.3f
#define DEF_NOWAVE_LEN ( DEF_WAVE_DISTR_STEP * 20.0f )
#define DEF_NOWAVE_LEN_VARIATION 0.3f
#define DEF_WAVE_MAX_AMPL 1.5f
#define DEF_WAVE_MIN_AMPL 0.0f

#define DEF_FOAM_DY ( /*0.0075f*/0.08f * DEF_NEW_TIME * 12.0f )
#define DEF_FOAM_SCALE 0.2f
//#define DEF_FOAM_BACK_DIST 0.6f
#define DEF_FOAM_BACK_DIST 0.5f
#define DEF_FOAM_BACK_DIST2 ( 1.0f - ( 1.0f - DEF_FOAM_BACK_DIST ) * ( 1.0f - DEF_FOAM_BACK_DIST ))
#define DEF_INV_ONE_MINUS_FOAM_BACK_DIST2 ( 1.0f / ( 1.0f - DEF_FOAM_BACK_DIST2 ) )

#define DEF_MESH2_TILES_PER_TEX 2
#define DEF_MESH2_TILE_COEFF ( 1.0f / DEF_MESH2_TILES_PER_TEX )

#define DEF_FOAM_Y_ANIM_OFFS 0.0f
#define DEF_FOAM_Y_ANIM ( 0.015f * DEF_NEW_TIME )

#define DEF_WATER_HOR_DEFORM_MIN_DANG ( 2.5f * DEF_NEW_TIME )
#define DEF_WATER_HOR_DEFORM_VAR_DANG ( 7.5f * DEF_NEW_TIME )
#define DEF_WATER_HOR_DEFORM_DRAD ( 0.05f * 0.5f * DEF_NEW_TIME )
#define DEF_WATER_HOR_DEFORM_MIN_RAD 0.0f
#define DEF_WATER_HOR_DEFORM_MAX_RAD ( 1.0f * 0.5f )

#define DEF_ANIM_MAX_FRAMES 16

#define DEF_WATER_TIME_STEP ( 0.0007f * DEF_NEW_TIME )
#define DEF_WATER_ANIM_TIME DEF_NEW_TIME

#define DEF_COAST_HOR_DEFORM_COEFF 0.5f

#define DEF_WATER_TILE_SIZE DEF_TILE_SIZE

#define DEF_FOAM_SPEED_QUAD 0.1f
#define DEF_FOAM_SPEED_QUAD_X 0.31622776f
#define DEF_FOAM_SPEED_QUAD_SCALE (1.0f / DEF_FOAM_SPEED_QUAD)

//static int nLastRandSeed = 0;

// normal.w - transparency
// texU.rgb - dwColor (texU.w is occupied by fog)
// texV.rgb - cScndTex (texV.w is occupied by fog)
static BOOST_FORCEINLINE void SetupColors( NGScene::SVertex *pRes, const DWORD dwColor, const unsigned char cScndTex,
																			 const unsigned char cTransparency )
{
	CalcCompactVector( &pRes->normal, CVec3(0, 0, 1) );
	pRes->normal.w = cTransparency;
	pRes->texU.dw = dwColor;
	int nSndTex = cScndTex;
	nSndTex |= nSndTex << 8;
	nSndTex |= nSndTex << 16;
	pRes->texV.dw = nSndTex;
}

CWaterPatch::CWaterPatch( const NDb::SWater *_pDesc, CFuncBase<STime> *_pTimer, CCSBound *_pBound )
: pDesc( _pDesc ), pTimer( _pTimer ), pBound( _pBound )
{
	fBaseHeight = 0.5f;

	fCurOffset = 0.0f;
	bIsCurWave = false;
	fCurWaveStart = 0.0f;
	fCurWaveLen = 0.0f;

	layerWater.pPatch = new CWaterPatchLayer( this );
	layerSurf.pPatch = new CWaterPatchLayer( this );
}

void CWaterPatch::Create( NGScene::IGameView *pGView )
{
	NGScene::SFullRoomInfo room( NGScene::SRoomInfo(NGScene::LF_SKIP_LIGHTING, -100), 0, 0 );
	layerWater.pHolder = pGView->CreateDynamicMesh( pGView->MakeMeshInfo(layerWater.pPatch, pDesc->pWaterSet->water.pMaterial), 0, pBound, NGScene::MakeLargeHintBound(), room );
	layerSurf.pHolder = pGView->CreateDynamicMesh( pGView->MakeMeshInfo(layerSurf.pPatch, pDesc->pWaterSet->pSurf), 0, pBound, NGScene::MakeLargeHintBound(), room );
}

//static vector<CVec3> lastPositions( 128 );
int CWaterPatch::Process( const long nTime )
{
	const float fCurTime = (float)nTime * DEF_WATER_TIME_STEP;
	unsigned long nCount = 0;
	for ( int g = 0; g < waterSurf.GetSizeY(); ++g )
	{
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			grid[nCount].z = fBaseHeight;
			for ( int k = 0; k < waves.size(); ++k )
				grid[nCount].z += waves[k].fAmplitude * waterSurf[g][i].amplitude[k] *
													GetWaveProfile( GetFracPart(waterSurf[g][i].phase[k] + fCurTime * waves[k].fInvPeriod) );
			++nCount;
		}
	}

	float fTexX = 0.0f;
	float fTexY = 0.0f;
	float fTexX2 = 0.0f;
	float fTexY2 = GetFracPart( DEF_FOAM_Y_ANIM_OFFS + (float)nTime * DEF_FOAM_Y_ANIM );
	int nTexX = 0;
	int nTexY = 0;
	int nTexX2 = 0;

	const int nCurFrame = int( (float)nTime * DEF_WATER_ANIM_TIME ) % DEF_ANIM_MAX_FRAMES;
	const int nTexOffsY = nCurFrame >> 2;
	const int nTexOffsX = nCurFrame - nTexOffsY * 4;
	const float fTexOffsX = 0.25f * nTexOffsX + DEF_TEX_ERROR;
	const float fTexOffsY = 0.25f * nTexOffsY + DEF_TEX_ERROR;

	const int nTexOffsY2 = nCurFrame >> 2;
	const int nTexOffsX2 = nCurFrame - nTexOffsY2 * 4;
	const float fTexOffsX2 = 0.25f * nTexOffsX2 + DEF_TEX_ERROR;
	const float fTexOffsY2 = 0.25f * nTexOffsY2 + DEF_TEX_ERROR;

	nCount = 0;
	for ( int g = 0; g < (waterSurf.GetSizeY() - 1); ++g )
	{
		for ( int i = 0; i < (waterSurf.GetSizeX() - 1); ++i )
		{
			const unsigned int nGridInd = g * waterSurf.GetSizeX() + i;
			{
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount];
				const CVec3 &vNode = grid[nGridInd].pos;
				curWaterVert.pos.Set(
					vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g][i].fOffsX + vWaterOffset.x,
					vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g][i].fOffsY + vWaterOffset.y,
					vNode.z );
				curWaterVert.tex.Set( fTexX * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX,
															fTexY * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY );
				objDataWater.secondTex[nCount].Set( fTexX2 * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX2,
																						fTexY2 * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY2 );
			}
			{
				const unsigned int nCurGridInd = nGridInd + 1;
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount+1];
				const CVec3 &vNode = grid[nCurGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g][i+1].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g][i+1].fOffsY + vWaterOffset.y,
															vNode.z );
				curWaterVert.tex.Set( (fTexX + DEF_WATER_TEX_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX,
															 fTexY * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY);
				objDataWater.secondTex[nCount+1].Set( (fTexX2 + DEF_MESH2_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX2,
																							 fTexY2 * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY2 );
			}
			{
				const unsigned int nCurGridInd = nGridInd + 1 + waterSurf.GetSizeX();
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount+2];
				const CVec3 &vNode = grid[nCurGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g+1][i+1].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g+1][i+1].fOffsY + vWaterOffset.y,
															vNode.z );
				curWaterVert.tex.Set( (fTexX + DEF_WATER_TEX_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX,
															(fTexY + DEF_WATER_TEX_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY );
				objDataWater.secondTex[nCount+2].Set( (fTexX2 + DEF_MESH2_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX2,
																							(fTexY2 + DEF_MESH2_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY2 );
			}
			{
				const unsigned int nCurGridInd = nGridInd + waterSurf.GetSizeX();
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount+3];
				const CVec3 &vNode = grid[nCurGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g+1][i].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g+1][i].fOffsY + vWaterOffset.y,
															vNode.z );
				curWaterVert.tex.Set(	fTexX * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX,
															(fTexY + DEF_WATER_TEX_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY );
				objDataWater.secondTex[nCount+3].Set(	fTexX2 * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsX2,
																							(fTexY2 + DEF_MESH2_TILE_COEFF) * DEF_WATER_TEX_COORD_OFFSET2 + fTexOffsY2 );
			}
			++nTexX;
			if ( nTexX >= DEF_WATER_TEX_TILES_PER_TEX )
				nTexX = 0;
			fTexX = (float)nTexX * DEF_WATER_TEX_TILE_COEFF;

			++nTexX2;
			if ( nTexX2 >= DEF_MESH2_TILES_PER_TEX )
				nTexX2 = 0;
			fTexX2 = (float)nTexX2 * DEF_MESH2_TILE_COEFF;

			nCount += 4;
		}

		++nTexY;
		if ( nTexY >= DEF_WATER_TEX_TILES_PER_TEX )
			nTexY = 0;
		fTexY = (float)nTexY * DEF_WATER_TEX_TILE_COEFF;
		nTexX = 0;
		fTexX = 0.0f;

		fTexY2 += DEF_MESH2_TILE_COEFF;
		if ( fTexY2 >= 1.0f )
			fTexY2 -= 1.0f;
		nTexX2 = 0;
		fTexX2 = 0.0f;
	}

	/*// fill first column for storing
	if ( lastPositions.size() > 0 )
	{
		NI_ASSERT( lastPositions.size() == waterSurf.GetSizeY(), "LastPositions error" );
		nCount = 0;
		vector<CVec3>::iterator itLastPos = lastPositions.begin();
		for ( ; itLastPos != lastPositions.end(); nCount += waterSurf.GetSizeX(), ++itLastPos )
		{
			objDataWater.verts[nCount].pos = *itLastPos;
		}
	}

	{
		// store last column
		lastPositions.resize( waterSurf.GetSizeY() );
		nCount = waterSurf.GetSizeX() - 1;
		vector<CVec3>::iterator itLastPos = lastPositions.begin();
		for ( ; itLastPos != lastPositions.end(); nCount += waterSurf.GetSizeX(), ++itLastPos )
		{
			*itLastPos = objDataWater.verts[nCount].pos;
		}
	}*/

	// find maximum and minimum heights
	float fMaxHeight = -FP_MAX_VALUE;
	float fMinHeight = FP_MAX_VALUE;
	for ( std::vector<SGridType>::const_iterator it = grid.begin(); it != grid.end(); ++it )
	{
		if ( it->z < fMinHeight )
			fMinHeight = it->z;
		if ( it->z > fMaxHeight )
			fMaxHeight = it->z;
	}

	// create coast
	const float d = 1.0f / nCoastLen;
	for ( int g = nCoastLen - 1; g >= 0; --g )
	{
		const float d1 = d * g;
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			nCount = (long)g * waterSurf.GetSizeX() + i;
			grid[nCount].y = grid[nCount + waterSurf.GetSizeX()].y - dGridSize.y;
			if ( grid[nCount].z > 0.0f )
				grid[nCount].y -= grid[nCount].z * ( 1.0f - d1 ) * 1.5f;
			else
				grid[nCount].y -= grid[nCount].z * ( 1.0f - d1 ) * 1.5f;
			grid[nCount].z *= d1;
			grid[nCount].z += 0.15f * ( 1.0f - d1 );
		}
	}

	nCount = 0;
	for ( int g = 0; g < nCoastLen; ++g )
	{
		for ( int i = 0; i < (waterSurf.GetSizeX() - 1); ++i )
		{
			const unsigned int nGridInd = g * waterSurf.GetSizeX() + i;
			{
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount];
				const CVec3 &vNode = grid[nGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g][i].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g][i].fOffsY + vWaterOffset.y,
															vNode.z );
				SetupColors( &curWaterVert, grid[nGridInd].color & 0xffffff, grid[nGridInd].color >> 24, 0xff );
			}
			{
				const unsigned int nCurGridInd = nGridInd + 1;
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount+1];
				const CVec3 &vNode = grid[nCurGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g][i+1].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g][i+1].fOffsY + vWaterOffset.y,
															vNode.z );
				SetupColors( &curWaterVert, grid[nGridInd].color & 0xffffff, grid[nGridInd].color >> 24, 0xff );
			}
			{
				const unsigned int nCurGridInd = nGridInd + 1 + waterSurf.GetSizeX();
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount+2];
				const CVec3 &vNode = grid[nCurGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g+1][i+1].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g+1][i+1].fOffsY + vWaterOffset.y,
															vNode.z );
				SetupColors( &curWaterVert, grid[nGridInd].color & 0xffffff, grid[nGridInd].color >> 24, 0xff );
			}
			{
				const unsigned int nCurGridInd = nGridInd + waterSurf.GetSizeX();
				NGScene::SVertex &curWaterVert = objDataWater.verts[nCount+3];
				const CVec3 &vNode = grid[nCurGridInd].pos;
				curWaterVert.pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g+1][i].fOffsX + vWaterOffset.x,
															vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g+1][i].fOffsY + vWaterOffset.y,
															vNode.z );
				SetupColors( &curWaterVert, grid[nGridInd].color & 0xffffff, grid[nGridInd].color >> 24, 0xff );
			}
			nCount += 4;
		}
	}

	const float fInvCoastLen = 1.0f / ( nCoastLen - 1 );

	// fill coast mesh
	for ( int k = 0; k < foamParams.size(); ++k )
	{
		for ( int i = 0; i < foamParams[k].size(); ++i )
		{
			const float fDy = foamParams[k][i].fFoamDy + DEF_FOAM_DY * nTime * DEF_NEW_TIME;
			const int nDy = (int)fDy;
			//if ( foamParams[k][i].bIsFoamMoveForward )
			//if ( !( nDy & 1 ) )
			const int nTale = nDy % 3;
			if ( nTale < 2 )
			{
				const float fFoamDy = ( fDy - nDy + nTale ) * 0.5f;
				//foamParams[k][i].fFoamDy += DEF_FOAM_DY;
				const float t = DEF_FOAM_SPEED_QUAD_X - fFoamDy * DEF_FOAM_SPEED_QUAD_X;
				foamParams[k][i].fFoamY = 1.0f - /*NMath::Sqrt( t * t * t )*/t * t * DEF_FOAM_SPEED_QUAD_SCALE;
				/*if ( foamParams[k][i].fFoamDy >= ( 1.0f - EPS_VALUE ))
				{
					foamParams[k][i].bIsFoamMoveForward = !foamParams[k][i].bIsFoamMoveForward;
					foamParams[k][i].fFoamY = 1.0f;
				}*/
				foamParams[k][i].fFoamTexCoeff = fInvCoastLen / ( 0.01f + foamParams[k][i].fFoamY * 0.99f );
				foamParams[k][i].fFoamTexOffs = 0.0f;
				foamParams[k][i].nFoamColor = /*( (DWORD)*/Clamp( int(foamParams[k][i].fFoamY * 255.0f), 0, 255 )/* << 24 )*/;
			}
			else
			{
				const float fFoamDy = 1.0f - ( fDy - (float)nDy ) * 0.5f;
				//foamParams[k][i].fFoamDy -= DEF_FOAM_DY;
				const float t = DEF_FOAM_SPEED_QUAD_X - fFoamDy * DEF_FOAM_SPEED_QUAD_X;
				foamParams[k][i].fFoamY = 1.0f - /*NMath::Sqrt( t * t * t )*/t * t * DEF_FOAM_SPEED_QUAD_SCALE;
				//foamParams[k][i].fFoamY = fFoamDy;
				/*if ( foamParams[k][i].fFoamDy <= ( DEF_FOAM_BACK_DIST + EPS_VALUE ))
				{
					foamParams[k][i].bIsFoamMoveForward = !foamParams[k][i].bIsFoamMoveForward;
					foamParams[k][i].fFoamDy = 0.0f;
					foamParams[k][i].fFoamY = DEF_FOAM_BACK_DIST2;
				}*/
				foamParams[k][i].fFoamTexCoeff = 1.0f * fInvCoastLen;
				foamParams[k][i].fFoamTexOffs = Clamp( 1.0f - foamParams[k][i].fFoamY, 0.0f, 1.0f );
				foamParams[k][i].nFoamColor = /*( (DWORD)*/Clamp( int(( foamParams[k][i].fFoamY - DEF_FOAM_BACK_DIST2 ) * DEF_INV_ONE_MINUS_FOAM_BACK_DIST2 * 255.0f), 0, 255 )/* << 24 )*/;
			}
		}
	}

	nCount = 0;
	for ( int k = 0; k < DEF_COASTES_NUM; ++k )
	{
		int nCurFoam = 0;
		float fCurTu = foamXOffsets[k];
		for ( int i = 0; i < ( waterSurf.GetSizeX() - 1 ); ++i )
		{
			for ( int g = 0; g < ( nCoastLen - 1 ); ++g )
			{
				const unsigned int nGridInd = g * waterSurf.GetSizeX() + i;
				
				{
					const CVec3 &vNode = grid[nGridInd].pos;
					objDataSurf.verts[nCount].pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g][i].fOffsX * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.x,
																						 vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g][i].fOffsY * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.y,
																						 vNode.z );
					objDataSurf.verts[nCount].texU.dw = /*foamParams[k][nCurFoam].nFoamColor*/0xffffffff;
					objDataSurf.verts[nCount].normal.w = foamParams[k][nCurFoam].nFoamColor;
					objDataSurf.verts[nCount++].tex.Set( fCurTu, Clamp(foamParams[k][nCurFoam].fFoamTexOffs + (float)( nCoastLen - 1 - g ) * foamParams[k][nCurFoam].fFoamTexCoeff,
																														 0.0f, 15.0f) );
				}
				{
					const unsigned int nNextGridInd = nGridInd + 1;
					const CVec3 &vNode = grid[nNextGridInd].pos;
					objDataSurf.verts[nCount].pos.Set(
						vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g][i+1].fOffsX * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.x,
						vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g][i+1].fOffsY * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.y,
						vNode.z );
					objDataSurf.verts[nCount].texU.dw = /*foamParams[k][nCurFoam].nFoamColor*/0xffffffff;
					objDataSurf.verts[nCount].normal.w = foamParams[k][nCurFoam].nFoamColor;
					objDataSurf.verts[nCount++].tex.Set( fCurTu + DEF_FOAM_SCALE, Clamp(foamParams[k][nCurFoam].fFoamTexOffs + (float)(nCoastLen - 1 - g) * foamParams[k][nCurFoam].fFoamTexCoeff, 0.0f, 15.0f) );
				}
				{
					const unsigned int nNextGridInd = nGridInd + 1 + waterSurf.GetSizeX();
					const CVec3 &vNode = grid[nNextGridInd].pos;
					objDataSurf.verts[nCount].pos.Set( vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g+1][i+1].fOffsX * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.x,
																						 vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g+1][i+1].fOffsY * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.y,
																						 vNode.z );
					objDataSurf.verts[nCount].texU.dw = /*foamParams[k][nCurFoam].nFoamColor*/0xffffffff;
					objDataSurf.verts[nCount].normal.w = foamParams[k][nCurFoam].nFoamColor;
					objDataSurf.verts[nCount++].tex.Set( fCurTu + DEF_FOAM_SCALE, Clamp(foamParams[k][nCurFoam].fFoamTexOffs + (float)(nCoastLen - 2 - g) * foamParams[k][nCurFoam].fFoamTexCoeff, 0.0f, 15.0f) );
				}
				{
					const unsigned int nNextGridInd = nGridInd + waterSurf.GetSizeX();
					const CVec3 &vNode = grid[nNextGridInd].pos;
					objDataSurf.verts[nCount].pos.Set(
						vNode.x * fWaterRotationCos - vNode.y * fWaterRotationSin + waterHorDeform[g+1][i].fOffsX * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.x,
						vNode.x * fWaterRotationSin + vNode.y * fWaterRotationCos + waterHorDeform[g+1][i].fOffsY * DEF_COAST_HOR_DEFORM_COEFF + vWaterOffset.y,
						vNode.z );
					objDataSurf.verts[nCount].texU.dw = /*foamParams[k][nCurFoam].nFoamColor*/0xffffffff;
					objDataSurf.verts[nCount].normal.w = foamParams[k][nCurFoam].nFoamColor;
					objDataSurf.verts[nCount++].tex.Set( fCurTu, Clamp(foamParams[k][nCurFoam].fFoamTexOffs + (float)(nCoastLen - 2 - g) * foamParams[k][nCurFoam].fFoamTexCoeff, 0.0f, 15.0f) );
				}
			}

			fCurTu += DEF_FOAM_SCALE;
			if ( fCurTu >= (1.0f - EPS_VALUE) )
			{
				fCurTu = 0.0f;
				++nCurFoam;
				NI_ASSERT( nCurFoam < foamParams[k].size(), "Too much foams count" );
			}
		}
	}

	// regenerate normals
	nCount = 0;
	CVec3 vNorm, vCurNorm;
	unsigned int nNormsCount;
	for ( int g = 0; g < waterSurf.GetSizeY(); ++g )
	{
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			vNorm.Set(0, 0, 0);
			nNormsCount = 0;
			if ( g > 0 )
			{
				if ( i > 0 )
				{
					const CVec3 n( ( grid[nCount-1].pos - grid[nCount].pos ) ^ ( grid[nCount-waterSurf.GetSizeX()].pos - grid[nCount].pos ) );
					NMath::NormalizeFast( &vNorm, n );
					if ( vNorm.z < 0.0f )
						vNorm = -vNorm;
					++nNormsCount;
				}
				if ( i < ( waterSurf.GetSizeX() - 1 ) )
				{
					const CVec3 n( ( grid[nCount+1].pos - grid[nCount].pos ) ^ ( grid[nCount-waterSurf.GetSizeX()].pos - grid[nCount].pos ) );
					NMath::NormalizeFast( &vCurNorm, n );
					if ( vCurNorm.z < 0.0f )
						vCurNorm = -vCurNorm;
					vNorm += vCurNorm;
					++nNormsCount;
				}
			}
			if ( g < ( waterSurf.GetSizeY() - 1 ) )
			{
				if ( i > 0 )
				{
					const CVec3 n( ( grid[nCount-1].pos - grid[nCount].pos ) ^ ( grid[nCount+waterSurf.GetSizeX()].pos - grid[nCount].pos ) );
					NMath::NormalizeFast( &vCurNorm, n );
					if ( vCurNorm.z < 0.0f )
						vCurNorm = -vCurNorm;
					vNorm += vCurNorm;
					++nNormsCount;
				}
				if ( i < ( waterSurf.GetSizeX() - 1 ) )
				{
					const CVec3 n( (grid[nCount+1].pos - grid[nCount].pos) ^ (grid[nCount+waterSurf.GetSizeX()].pos - grid[nCount].pos) );
					NMath::NormalizeFast( &vCurNorm, n );
					if ( vCurNorm.z < 0.0f )
						vCurNorm = -vCurNorm;
					vNorm += vCurNorm;
					++nNormsCount;
				}
			}
			NI_ASSERT( nNormsCount > 0, "There are no avaible normals here" );
			vNorm *= invCoeffs[nNormsCount];

			CVec3 vSunDir;
			const float fHor = sin( ToRadian(pDesc->pLight->fPitch) );
			vSunDir.z = -cos( ToRadian(pDesc->pLight->fPitch) );
			vSunDir.x = fHor * cos( ToRadian(pDesc->pLight->fYaw) );
			vSunDir.y = fHor * sin( ToRadian(pDesc->pLight->fYaw) );
			vSunDir = -vSunDir;

			const CVec3 &vSunDirAmbient = pDesc->pLight->vAmbientColor;
			const CVec3 &vSunDirDiffuse = pDesc->pLight->vLightColor;
			const CVec3 &vSunUnDirAmbient = pDesc->pLight->vShadeAmbientColor;
			const CVec3 &vSunUnDirDiffuse = pDesc->pLight->vShadeColor;

			const float fScaling = 4.0f * 255.0f;

			const float fCos = Clamp( vNorm.x * vSunDir.x + vNorm.y * vSunDir.y + vNorm.z * vSunDir.z, -1.0f, 1.0f );
			if ( fCos >= 0.0f ) // direct lighting
			{
				const CVec3 vCol( (vSunDirAmbient.x * (1.0f - fCos) + vSunDirDiffuse.x * fCos) * fScaling,
													(vSunDirAmbient.y * (1.0f - fCos) + vSunDirDiffuse.y * fCos) * fScaling,
													(vSunDirAmbient.z * (1.0f - fCos) + vSunDirDiffuse.z * fCos) * fScaling );
				const CVec4 &vNoise = noise[g & (noise.GetSizeY() - 1)][i & (noise.GetSizeX() - 1)];
				const unsigned int cr = Clamp( int(vCol.x * (vNoise.x * pDesc->fDepthNoiseCoeff + 1.0f - pDesc->fDepthNoiseCoeff)), 0, 255 );
				const unsigned int cg = Clamp( int(vCol.y * (vNoise.y * pDesc->fDepthNoiseCoeff + 1.0f - pDesc->fDepthNoiseCoeff)), 0, 255 );
				const unsigned int cb = Clamp( int(vCol.z * (vNoise.z * pDesc->fDepthNoiseCoeff + 1.0f - pDesc->fDepthNoiseCoeff)), 0, 255 );
				const unsigned long nResCol = (unsigned long)cr * 65536 + cg * 256 + cb;
				grid[nCount].color = ( grid[nCount].color & 0xff000000 ) | nResCol;
			}
			else // undirect lighting
			{
				const CVec3 vCol( (vSunUnDirAmbient.x + vSunUnDirDiffuse.x * (1.0f + fCos)) * fScaling,
													(vSunUnDirAmbient.y + vSunUnDirDiffuse.y * (1.0f + fCos)) * fScaling,
													(vSunUnDirAmbient.z + vSunUnDirDiffuse.z * (1.0f + fCos)) * fScaling );
				const CVec4 &vNoise = noise[g & ( noise.GetSizeY() - 1 )][i & ( noise.GetSizeX() - 1 )];
				const unsigned int cr = Clamp( int( vCol.x * ( vNoise.x * pDesc->fDepthNoiseCoeff + 1.0f - pDesc->fDepthNoiseCoeff ) ), 0, 255 );
				const unsigned int cg = Clamp( int( vCol.y * ( vNoise.y * pDesc->fDepthNoiseCoeff + 1.0f - pDesc->fDepthNoiseCoeff ) ), 0, 255 );
				const unsigned int cb = Clamp( int( vCol.z * ( vNoise.z * pDesc->fDepthNoiseCoeff + 1.0f - pDesc->fDepthNoiseCoeff ) ), 0, 255 );
				const unsigned long nResCol = (unsigned long)cr * 65536 + cg * 256 + cb;
				grid[nCount].color = ( grid[nCount].color & 0xff000000 ) | nResCol;
			}
			++nCount;
		}
	}

	fMinHeight = fMinHeight * 0.6f + fMaxHeight * 0.4f;

	fMinHeight = 0.9f;
	fMaxHeight = 1.5f;

	const float fMesh2VarInv = 1.0f / ( fMaxHeight - fMinHeight );
	nCount = 0;
	for ( int g = 0; g < (waterSurf.GetSizeY() - 1); ++g )
	{
		for ( int i = 0; i < (waterSurf.GetSizeX() - 1); ++i )
		{
			const unsigned int nGridInd = g * waterSurf.GetSizeX() + i;

			{
				const float fAlp2 = (float)( grid[nGridInd].color >> 24 ) * DEF_INV_255;
				const float fCoeff = fabs2( Clamp( ( grid[nGridInd].z - fMinHeight ) * fMesh2VarInv * fAlp2, 0.0f, 1.0f ) ) * 255.0f;
				const unsigned long nAlp = (unsigned long)(fCoeff) * 16777216;
				DWORD dwGridColor = grid[ nGridInd ].color;
				SetupColors( &objDataWater.verts[nCount], dwGridColor & 0xffffff, (dwGridColor & nAlp) >> 24, dwGridColor >> 24 );
				++nCount;
			}

			{
				const float fAlp2 = (float)( grid[nGridInd+1].color >> 24 ) * DEF_INV_255;
				const float fCoeff = fabs2( Clamp( ( grid[nGridInd+1].z - fMinHeight ) * fMesh2VarInv * fAlp2, 0.0f, 1.0f ) ) * 255.0f;
				const unsigned long nAlp = (unsigned long)(fCoeff) * 16777216;
				DWORD dwGridColor = grid[ nGridInd + 1 ].color;
				SetupColors( &objDataWater.verts[nCount], dwGridColor & 0xffffff, (dwGridColor & nAlp) >> 24, dwGridColor >> 24 );
				++nCount;
			}

			{
				const float fAlp2 = (float)( grid[nGridInd+1+waterSurf.GetSizeX()].color >> 24 ) * DEF_INV_255;
				const float fCoeff = fabs2( Clamp( ( grid[nGridInd+1+waterSurf.GetSizeX()].z - fMinHeight ) * fMesh2VarInv * fAlp2, 0.0f, 1.0f ) ) * 255.0f;
				const unsigned long nAlp = (unsigned long)(fCoeff) * 16777216;
				DWORD dwGridColor = grid[ nGridInd + 1 + waterSurf.GetSizeX() ].color;
				SetupColors( &objDataWater.verts[nCount], dwGridColor & 0xffffff, (dwGridColor & nAlp) >> 24, dwGridColor >> 24 );
				++nCount;
			}

			{
				const float fAlp2 = (float)( grid[nGridInd+waterSurf.GetSizeX()].color >> 24 ) * DEF_INV_255;
				const float fCoeff = fabs2( Clamp( ( grid[nGridInd+waterSurf.GetSizeX()].z - fMinHeight ) * fMesh2VarInv * fAlp2, 0.0f, 1.0f ) ) * 255.0f;
				const unsigned long nAlp = (unsigned long)(fCoeff) * 16777216;
				DWORD dwGridColor = grid[ nGridInd + waterSurf.GetSizeX() ].color;
				SetupColors( &objDataWater.verts[nCount], dwGridColor & 0xffffff, (dwGridColor & nAlp) >> 24, dwGridColor >> 24 );
				++nCount;
			}
		}
	}

	// process horizontal water deformation parameters changing
	for ( int g = 0; g < waterSurf.GetSizeY(); ++g )
	{
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			long nAng = (long)( waterHorDeform[g][i].fAngOffs + waterHorDeform[g][i].fDAng * nTime ) % 360;
			float fAng = nAng * FP_PI / 180.0f;
			float fRad = fmod( waterHorDeform[g][i].fRadOffs + waterHorDeform[g][i].fDRad * nTime,
				(( DEF_WATER_HOR_DEFORM_MAX_RAD - DEF_WATER_HOR_DEFORM_MIN_RAD ) * 2.0f ));
			if ( fRad >= ( DEF_WATER_HOR_DEFORM_MAX_RAD - DEF_WATER_HOR_DEFORM_MIN_RAD ))
				fRad = 2.0f * ( DEF_WATER_HOR_DEFORM_MAX_RAD - DEF_WATER_HOR_DEFORM_MIN_RAD ) - fRad;
			fRad += DEF_WATER_HOR_DEFORM_MIN_RAD;
			waterHorDeform[g][i].fOffsX = cos( fAng ) * fRad;
			waterHorDeform[g][i].fOffsY = sin( fAng ) * fRad;
		}
	}

	return true;
}

inline float CWaterPatch::GetPhase( const int x, const int y, const int curw )
{
	return waves[curw].fDeepWaveNumber / GetTanHyperbolic( waves[curw].fDeepWaveNumber * waterSurf[y][x].fHeight );
}

inline float CWaterPatch::GetTanHyperbolic( const float z )
{
	return ( exp( z ) - exp( -z )) / ( exp( z ) + exp( -z ));
}

inline float CWaterPatch::GetFracPart( const float z )
{
	return z - (int)z;
}

inline float CWaterPatch::GetWaveProfile( const float z )
{
	const float t = z - 0.5f;
	return 24.0f * t * t * t * t * t * t;
}

int CWaterPatch::Init( const int nSX, const int nSY, const int nCoast, const CVec2 &dsize, const CVec2 &org, const float _fRotAngle )
{
	unsigned long nCount = 0;

	nCoastLen = nCoast;
	dGridSize = dsize;

	// init waves
	waves.resize( DEF_WAVES_NUM );
	for ( std::vector<SWaveType>::iterator it = waves.begin(); it != waves.end(); ++it )
	{
		it->fAmplitude = DEF_WAVES_AMPLITUDE;
		it->fDeepWaveNumber = DEF_WAVES_DEEP_WAVE_NUMBER;
		it->fPeriod = DEF_WAVES_PERIOD + NWin32Random::Random( 0.0f, 1.0f ) * DEF_WAVES_PERIOD_VARIATION;
		it->fInvPeriod = 1.0f / it->fPeriod;
	}

	// init water surface properties
	waterSurf.SetSizes( nSX, nSY );
	for ( int g = 0; g < waterSurf.GetSizeY(); ++g )
	{
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			waterSurf[g][i].fHeight = 5.0f;
			waterSurf[g][i].phase.resize( DEF_WAVES_NUM );
			waterSurf[g][i].amplitude.resize( DEF_WAVES_NUM );
			for ( int k = 0; k < DEF_WAVES_NUM; ++k )
			{
				waterSurf[g][i].phase[k] = 0.0f;
				waterSurf[g][i].amplitude[k] = 0.0f;
			}
		}
	}

	// init vertexes
	grid.resize( nSX * nSY );
	fWaterRotationSin = sin( _fRotAngle );
	fWaterRotationCos = cos( _fRotAngle );
	vWaterOffset = org;
	for ( int g = 0; g < nSY; ++g )
	{
		for ( int i = 0; i < nSX; ++i )
		{
			grid[nCount].x = (float)i * dsize.x;
			grid[nCount].y = (float)g * dsize.y;
			grid[nCount].z = fBaseHeight;
			grid[nCount].color = 0xffffffff;
			++nCount;
		}
	}

	// add colors for coast part of vertices
	const float d = 1.0f / nCoastLen;
	const int nAlphaSmooth = nCoastLen * 0.3;
	const float fInvAlphaSmooth = 1.0f / nAlphaSmooth;
	const float fInterpCoeff = 1.0f / ( nCoastLen - nAlphaSmooth - 1 );
	for ( int g = nCoastLen - 1; g >= 0; --g )
	{
		const float fCoeff = g <= nAlphaSmooth ?
			0.5f - ( 1.0f - (float)g * fInvAlphaSmooth ) * ( 1.0f - (float)g * fInvAlphaSmooth ) * 0.5f :
			(float)( g - nAlphaSmooth ) * fInterpCoeff * 0.5f + 0.5f;
		const unsigned long alpha = ( (unsigned long)Clamp( int(fCoeff * 255.0f), 0, 255 ) * 16777216 ) | 0x00ffffff;
		nCount = g * waterSurf.GetSizeX();
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			grid[nCount++].color = alpha;
		}
	}

	// init phase offsets for each wave
	for ( int k = 0; k < waves.size(); ++k )
	{
		if ( k == 0 ) 
			waves[k].fPhaseOffset = 0.0f;
		else
			waves[k].fPhaseOffset = waves[k - 1].fPhaseOffset + GetPhase( 0, 0, k ) * ( 0.5f + NWin32Random::Random( 0.0f, 1.0f ) * 0.5f );

		if (( k % 2 ) == 0 )
		{
			fCurOffset = 0.0f;
			bIsCurWave = true;
			fCurWaveStart = -NWin32Random::Random( 0.0f, 1.0f ) * DEF_WAVE_LEN * 0.0f + DEF_WAVE_LEN * 0.0f;
			fCurWaveLen = fCurWaveStart + DEF_WAVE_LEN * ( 1.0f - DEF_WAVE_LEN_VARIATION ) +
				NWin32Random::Random( 0.0f, 1.0f ) * DEF_WAVE_LEN * DEF_WAVE_LEN_VARIATION;
		}
		else // create non-wave
		{
			fCurOffset = 0.0f;
			bIsCurWave = false;
			fCurWaveStart = -NWin32Random::Random( 0.0f, 1.0f ) * DEF_NOWAVE_LEN * 0.0f + DEF_NOWAVE_LEN * 0.0f;
			fCurWaveLen = fCurWaveStart + DEF_NOWAVE_LEN * ( 1.0f - DEF_NOWAVE_LEN_VARIATION ) +
				NWin32Random::Random( 0.0f, 1.0f ) * DEF_NOWAVE_LEN * DEF_NOWAVE_LEN_VARIATION;
		}

		ProcessWaveDistribution( k );
	}

	// init foam parameters
	foamXOffsets.resize( DEF_COASTES_NUM );
	foamParams.resize( DEF_COASTES_NUM );
	for ( int k = 0; k < foamParams.size(); ++k )
	{
		foamParams[k].resize( waterSurf.GetSizeX() * DEF_FOAM_SCALE + 2 );
		for ( int l = 0; l < foamParams[k].size(); ++l )
		{
			if ( k > 0 )
			{
				if ( foamParams[k-1][l].fFoamDy < 2.0f )
				{
					foamParams[k][l].fFoamDy = foamParams[k-1][l].fFoamDy * 0.5f + 2.0f/*NWin32Random::Random( 2.0f, 2.9f )*/;
					//foamParams[k][l].fFoamY = 1.0f - ( 1.0f - foamParams[k][l].fFoamDy ) * ( 1.0f - foamParams[k][l].fFoamDy );
					foamParams[k][l].fFoamTexOffs = 0.0f;
					foamParams[k][l].fFoamTexCoeff = 0.0f;
				}
				else
				{
					//foamParams[k][l].fFoamDy = DEF_FOAM_BACK_DIST2 + NWin32Random::Random( 1.0f, 2.0f ) * ( 1.0f - DEF_FOAM_BACK_DIST2 );
					foamParams[k][l].fFoamDy = ( foamParams[k-1][l].fFoamDy - 2.0f ) * 2.0f/*NWin32Random::Random( 0.0f, 2.0f )*/;
					//foamParams[k][l].fFoamY = 1.0f - ( 1.0f - foamParams[k][l].fFoamDy ) * ( 1.0f - foamParams[k][l].fFoamDy );
					foamParams[k][l].fFoamTexOffs = 0.0f;
					foamParams[k][l].fFoamTexCoeff = 0.0f;
				}
			}
			else
			{
				foamParams[k][l].fFoamDy = NWin32Random::Random( 0.0f, 3.0f );
				//foamParams[k][l].fFoamY = 1.0f - ( 1.0f - foamParams[k][l].fFoamDy ) * ( 1.0f - foamParams[k][l].fFoamDy );
				foamParams[k][l].fFoamTexOffs = 0.0f;
				foamParams[k][l].fFoamTexCoeff = 0.0f;
			}
		}
		foamXOffsets[k] = ( (float)k + 0.3f + NWin32Random::Random( 0.0f, 0.7f ) ) / foamParams.size();
	}

	// initialize water horizontal deformation parameters
	waterHorDeform.SetSizes( waterSurf.GetSizeX(), waterSurf.GetSizeY() );
	//NWin32Random::Seed( nLastRandSeed );
	for ( int g = 0; g < waterSurf.GetSizeY(); ++g )
	{
		//if ( g == ( waterSurf.GetSizeY() - 1 ) )
		//	nLastRandSeed = NWin32Random::GetSeed();
		for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
		{
			waterHorDeform[g][i].fAngOffs = NWin32Random::Random( 0.0f, 1.0f ) * ( DEF_WATER_HOR_DEFORM_MIN_DANG + DEF_WATER_HOR_DEFORM_VAR_DANG );
			waterHorDeform[g][i].fDAng = NWin32Random::Random( 0.0f, 1.0f ) * DEF_WATER_HOR_DEFORM_VAR_DANG + DEF_WATER_HOR_DEFORM_MIN_DANG;
			if (( NWin32Random::Random( 0.0f, 1.0f ) ) >= 0.5f )
				waterHorDeform[g][i].fDAng = -waterHorDeform[g][i].fDAng;
			float fAng = 0.0f;

			waterHorDeform[g][i].fRadOffs = NWin32Random::Random( 0.0f, 1.0f ) * DEF_WATER_HOR_DEFORM_MAX_RAD;
			waterHorDeform[g][i].fDRad = DEF_WATER_HOR_DEFORM_DRAD * 0.1f + NWin32Random::Random( 0.0f, 1.0f ) * DEF_WATER_HOR_DEFORM_DRAD;

			float fRad = fmod( waterHorDeform[g][i].fRadOffs,
				(( DEF_WATER_HOR_DEFORM_MAX_RAD - DEF_WATER_HOR_DEFORM_MIN_RAD ) * 2.0f ));
			if ( fRad >= ( DEF_WATER_HOR_DEFORM_MAX_RAD - DEF_WATER_HOR_DEFORM_MIN_RAD ))
				fRad = 2.0f * ( DEF_WATER_HOR_DEFORM_MAX_RAD - DEF_WATER_HOR_DEFORM_MIN_RAD ) - fRad;
			fRad += DEF_WATER_HOR_DEFORM_MIN_RAD;

			waterHorDeform[g][i].fOffsX = cos( fAng ) * fRad;
			waterHorDeform[g][i].fOffsY = sin( fAng ) * fRad;
		}
	}

	// init water geometry buffers
	std::vector<STriangle> trgs;
	std::vector<STriangle> trgs2;
	STriangle curTrg;
	for ( int k = 0; k < 2; ++k ) 
	{
		objDataWater.verts.resize( ( nSX - 1 ) * ( nSY - 1 ) * 4 );
		objDataWater.secondTex.resize( ( nSX - 1 ) * ( nSY - 1 ) * 4 );
		trgs.reserve( ( nSX - 1 ) * ( nSY - 1 ) * 2 );
		trgs.resize( 0 );
		nCount = 0;
		for ( int g = 0; g < ( nSY - 1 ); ++g )
		{
			for ( int i = 0; i < ( nSX - 1 ); ++i )
			{
				// init indices
				curTrg.i1 = nCount;
				curTrg.i2 = nCount + 1;
				curTrg.i3 = nCount + 2;
				trgs.push_back( curTrg );

				curTrg.i1 = nCount + 2;
				curTrg.i2 = nCount + 3;
				curTrg.i3 = nCount;
				trgs.push_back( curTrg );

				const unsigned int nGridInd = g * waterSurf.GetSizeX() + i;
				// init vertices
				{
					NGScene::SVertex &curWaterVert = objDataWater.verts[nCount];
					const SGridType &curNode = grid[nGridInd];
					curWaterVert.pos = curNode.pos;
					SetupColors( &curWaterVert, curNode.color & 0xffffff, curNode.color >> 24, curNode.color >> 24 );
					curWaterVert.tex.Set( 0, 0 );
					objDataWater.secondTex[nCount].Set( 0, 0 );
					++nCount;
				}
				{
					NGScene::SVertex &curWaterVert = objDataWater.verts[nCount];
					const SGridType &curNode = grid[nGridInd+1];
					curWaterVert.pos = curNode.pos;
					SetupColors( &curWaterVert, curNode.color & 0xffffff, curNode.color >> 24, curNode.color >> 24 );
					curWaterVert.tex.Set( 0, 0 );
					objDataWater.secondTex[nCount].Set( 0, 0 );
					++nCount;
				}
				{
					NGScene::SVertex &curWaterVert = objDataWater.verts[nCount];
					const SGridType &curNode = grid[nGridInd+1+waterSurf.GetSizeX()];
					curWaterVert.pos = curNode.pos;
					SetupColors( &curWaterVert, curNode.color & 0xffffff, curNode.color >> 24, curNode.color >> 24 );
					curWaterVert.tex.Set( 0, 0 );
					objDataWater.secondTex[nCount].Set( 0, 0 );
					++nCount;
				}
				{
					NGScene::SVertex &curWaterVert = objDataWater.verts[nCount];
					const SGridType &curNode = grid[nGridInd+waterSurf.GetSizeX()];
					curWaterVert.pos = curNode.pos;
					SetupColors( &curWaterVert, curNode.color & 0xffffff, curNode.color >> 24, curNode.color >> 24 );
					curWaterVert.tex.Set( 0, 0 );
					objDataWater.secondTex[nCount].Set( 0, 0 );
					++nCount;
				}
			}
		}
		objDataWater.geometry = trgs;
		layerWater.pPatch->UpdateMesh( &objDataWater );

		// init surf geometry buffers
		const unsigned int nSurfCells = ( nSX - 1 ) * ( nCoastLen - 1 );
		objDataSurf.verts.resize( nSurfCells * 4 * DEF_COASTES_NUM );
		nCount = 0;
		for ( int t = 0; t < DEF_COASTES_NUM; ++t )
		{
			for ( int g = 0; g < ( nCoastLen - 1 ); ++g )
			{
				for ( int i = 0; i < ( nSX - 1 ); ++i )
				{
					const unsigned int nGridInd = g * waterSurf.GetSizeX() + i;
					{
						NGScene::SVertex &curSurfVert = objDataSurf.verts[nCount];
						curSurfVert.pos.Set( grid[nGridInd].x, grid[nGridInd].y, grid[nGridInd].z + DEF_HEIGHT_BIAS );
						SetupColors( &curSurfVert, 0xffffff, 0xff, 0xff );
						curSurfVert.tex.Set( 0, 0 );
						++nCount;
					}
					{
						const unsigned int nNextGridInd = nGridInd + 1;
						NGScene::SVertex &curSurfVert = objDataSurf.verts[nCount];
						curSurfVert.pos.Set( grid[nNextGridInd].x, grid[nNextGridInd].y, grid[nNextGridInd].z + DEF_HEIGHT_BIAS );
						SetupColors( &curSurfVert, 0xffffff, 0xff, 0xff );
						curSurfVert.tex.Set( 0, 0 );
						++nCount;
					}
					{
						const unsigned int nNextGridInd = nGridInd + 1 + waterSurf.GetSizeX();
						NGScene::SVertex &curSurfVert = objDataSurf.verts[nCount];
						curSurfVert.pos.Set( grid[nNextGridInd].x, grid[nNextGridInd].y, grid[nNextGridInd].z + DEF_HEIGHT_BIAS );
						SetupColors( &curSurfVert, 0xffffff, 0xff, 0xff );
						curSurfVert.tex.Set( 0, 0 );
						++nCount;
					}
					{
						const unsigned int nNextGridInd = nGridInd + waterSurf.GetSizeX();
						NGScene::SVertex &curSurfVert = objDataSurf.verts[nCount];
						curSurfVert.pos.Set( grid[nNextGridInd].x, grid[nNextGridInd].y, grid[nNextGridInd].z + DEF_HEIGHT_BIAS );
						SetupColors( &curSurfVert, 0xffffff, 0xff, 0xff );
						curSurfVert.tex.Set( 0, 0 );
						++nCount;
					}
				}
			}
		}

		trgs2.reserve( objDataSurf.verts.size() );
		trgs2.resize( 0 );
		nCount = 0;
		for ( int t = 0; t < DEF_COASTES_NUM; ++t )
		{
			for ( int i = 0; i < nSurfCells; ++i )
			{
				curTrg.i1 = nCount;
				curTrg.i2 = nCount + 1;
				curTrg.i3 = nCount + 2;
				trgs2.push_back( curTrg );

				curTrg.i1 = nCount + 2;
				curTrg.i2 = nCount + 3;
				curTrg.i3 = nCount;
				trgs2.push_back( curTrg );

				nCount += 4;
			}
		}
		objDataSurf.geometry = trgs2;
		layerSurf.pPatch->UpdateMesh( &objDataSurf );
	}

	// load noise
	CFileStream noiseStream( NVFS::GetMainVFS(), pDesc->pDepthNoise->szFileName );
	NI_ASSERT( noiseStream.IsOk(), StrFmt("Can't load %s", pDesc->pDepthNoise->szFileName) );
	NImage::LoadTGAImage( noise, &noiseStream );

	return true;
}

void CWaterPatch::ProcessWaveDistribution( const int nWaveNum )
{
	float fDefPhase;
	for ( int i = 0; i < waterSurf.GetSizeX(); ++i )
	{
		float fAmpl;
		if ( bIsCurWave )
		{
			fAmpl = Clamp( static_cast<float>(fabs((fCurWaveStart + fCurWaveLen) - fCurOffset * 2.0f) / (fCurWaveLen - fCurWaveStart)), 0.0f, 1.0f );
			fAmpl = 1.0f - fAmpl * fAmpl;
			fAmpl = DEF_WAVE_MIN_AMPL + fAmpl * DEF_WAVE_MAX_AMPL;
		}
		else
			fAmpl = DEF_WAVE_MIN_AMPL;

		fDefPhase = 0.0f;

		fDefPhase += waves[nWaveNum].fPhaseOffset;

		const int nStages = 3;
		for ( int p = 0; p < nStages; ++p )
		{
			const float fCoeff = (float)p / ( nStages - 1 );
			fDefPhase += sin( (float)i * (0.3f + 0.9f * fCoeff) ) * ( 0.01f + 0.04f * fCoeff );
		}

		float fPhase = fDefPhase * DEF_WAVES_SCALE_FACTOR;
		for ( int g = 0; g < waterSurf.GetSizeY(); ++g )
		{
			fPhase += GetPhase( i, g, nWaveNum ) * DEF_WAVES_SCALE_FACTOR;
			waterSurf[g][i].phase[nWaveNum] = fPhase;
			waterSurf[g][i].amplitude[nWaveNum] = fAmpl;
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

void CWaterPatch::Recalc()
{
	NTimer::STime time = pTimer->GetValue();
	Process( time );

	// recalculate water
	layerWater.pPatch->UpdateMesh( &objDataWater );
	layerSurf.pPatch->UpdateMesh( &objDataSurf );
}

void CWater::Create( NGScene::IGameView *pGView, const int nPatchesX, const int nPatchesY )
{
	int nWaterWidth = 15;
	int nCoastLen = 4;
	const float fMinPossibleHeight = -10.0f;
	const float fMaxPossibleHeight = 10.0f;

	float fAngle = -FP_PI * 0.5f;


	CVec2 vOffs( (float)( nPatchesX * DEF_PATCH_SIZE - nCoastLen ) * DEF_TILE_SIZE,
							 (float)( nPatchesY * DEF_PATCH_SIZE ) * DEF_TILE_SIZE );

	if ( (nPatchesX == 5) && (nPatchesY == 5) ) // for second playable only
	{
		fAngle = -FP_PI;
		vOffs.x = (float)( nPatchesX * DEF_PATCH_SIZE ) * DEF_TILE_SIZE;
		vOffs.y = ( 2.0f * DEF_PATCH_SIZE - 8.0f ) * DEF_TILE_SIZE;
		nWaterWidth = 2 * DEF_PATCH_SIZE - 6;
	}

	const float fAngleCos = cos( fAngle );
	const float fAngleSin = sin( fAngle );

	patches.reserve( nPatchesX );
	patches.resize( 0 );

	//nLastRandSeed = NWin32Random::GetSeed();

	//for ( int i = nPatchesX - 1; i >= 0; --i )
	//{
	//	const float fStartX = (float)i * DEF_PATCH_SIZE * DEF_TILE_SIZE;
	//	const CVec2 v1( fStartX, 0.0f );
	//	const CVec2 v2( fStartX, (float)nWaterWidth * DEF_TILE_SIZE );
	//	const CVec2 v3( fStartX + (float)DEF_PATCH_SIZE * DEF_TILE_SIZE, (float)nWaterWidth * DEF_TILE_SIZE );
	//	const CVec2 v4( fStartX + (float)DEF_PATCH_SIZE * DEF_TILE_SIZE, 0.0f );

	//	const CVec2 v1Rot( v1.x * fAngleCos - v1.y * fAngleSin + vOffs.x, v1.x * fAngleSin + v1.y * fAngleCos + vOffs.y );
	//	const CVec2 v2Rot( v2.x * fAngleCos - v2.y * fAngleSin + vOffs.x, v2.x * fAngleSin + v2.y * fAngleCos + vOffs.y );
	//	const CVec2 v3Rot( v3.x * fAngleCos - v3.y * fAngleSin + vOffs.x, v3.x * fAngleSin + v3.y * fAngleCos + vOffs.y );
	//	const CVec2 v4Rot( v4.x * fAngleCos - v4.y * fAngleSin + vOffs.x, v4.x * fAngleSin + v4.y * fAngleCos + vOffs.y );

	//	SBound bound;
	//	bound.BoxInit(
	//		CVec3( min( min( v1Rot.x, v2Rot.x ), min( v3Rot.x, v4Rot.x ) ), min( min( v1Rot.y, v2Rot.y ), min( v3Rot.y, v4Rot.y ) ), fMinPossibleHeight ),
	//		CVec3( max( max( v1Rot.x, v2Rot.x ), max( v3Rot.x, v4Rot.x ) ), max( max( v1Rot.y, v2Rot.y ), max( v3Rot.y, v4Rot.y ) ), fMaxPossibleHeight )
	//		);
	//	CCSBound *pBound = new CCSBound();
	//	pBound->Set( bound );
	//	patches.push_back( new CWaterPatch( pDesc, pTimer, pBound ) );
	//	patches.back()->Init( /*( nPatchesX * DEF_PATCH_SIZE + 1 )*/DEF_PATCH_SIZE + 1, nWaterWidth, nCoastLen,
	//		CVec2( DEF_WATER_TILE_SIZE, DEF_WATER_TILE_SIZE ), vOffs, fAngle );
	//	patches.back()->Create( pGView );

	//	if ( ( nPatchesX != 5 ) || ( nPatchesY != 5 ) )
	//		vOffs.y += DEF_PATCH_SIZE * DEF_TILE_SIZE;
	//	else
	//		vOffs.x -= DEF_PATCH_SIZE * DEF_TILE_SIZE;
	//}

	// calculate boundary
	const CVec2 v2( 0.0f, (float)nWaterWidth * DEF_TILE_SIZE );
	const CVec2 v3( (nPatchesX * DEF_PATCH_SIZE + 1) * DEF_TILE_SIZE, (float)nWaterWidth * DEF_TILE_SIZE );
	const CVec2 v4( (nPatchesX * DEF_PATCH_SIZE + 1) * DEF_TILE_SIZE, 0.0f );

	const CVec2 v2Rot( v2.x * fAngleCos - v2.y * fAngleSin + vOffs.x, v2.x * fAngleSin + v2.y * fAngleCos + vOffs.y );
	const CVec2 v3Rot( v3.x * fAngleCos - v3.y * fAngleSin + vOffs.x, v3.x * fAngleSin + v3.y * fAngleCos + vOffs.y );
	const CVec2 v4Rot( v4.x * fAngleCos - v4.y * fAngleSin + vOffs.x, v4.x * fAngleSin + v4.y * fAngleCos + vOffs.y );

	SBound bound;
	bound.BoxInit( CVec3(min(min(vOffs.x, v2Rot.x), min(v3Rot.x, v4Rot.x)), min(min(vOffs.y, v2Rot.y), min(v3Rot.y, v4Rot.y)), fMinPossibleHeight),
								 CVec3(max(max(vOffs.x, v2Rot.x), max(v3Rot.x, v4Rot.x)), max(max(vOffs.y, v2Rot.y), max(v3Rot.y, v4Rot.y)), fMaxPossibleHeight) );
	CCSBound *pBound = new CCSBound();
	pBound->Set( bound );
	patches.push_back( new CWaterPatch(pDesc, pTimer, pBound) );
	patches.back()->Init( (nPatchesX * DEF_PATCH_SIZE + 1), nWaterWidth, nCoastLen,
												CVec2(DEF_WATER_TILE_SIZE, DEF_WATER_TILE_SIZE), vOffs, fAngle );
	patches.back()->Create( pGView );
}

int CWaterPatchLayer::operator&( IBinSaver &saver )
{
	return 0;
}

int CWaterPatch::operator&( IBinSaver &saver )
{
	return 0;
}

int CWater::operator&( IBinSaver &saver )
{
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x100BB400, CWaterPatchLayer );
REGISTER_SAVELOAD_CLASS( 0x100BB401, CWaterPatch );
REGISTER_SAVELOAD_CLASS( 0x100BB402, CWater );


