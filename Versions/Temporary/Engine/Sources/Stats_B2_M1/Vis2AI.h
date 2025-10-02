#ifndef __VIS2AI_H__
#define __VIS2AI_H__
#pragma once

#define VIS_TILE_SIZE 2.75f
#define AI_TILE_SIZE 32
#define AI_TILES_IN_VIS_TILE 2
#define VIS_TO_AI ( float(AI_TILE_SIZE * AI_TILES_IN_VIS_TILE) / VIS_TILE_SIZE )
#define AI_TO_VIS ( VIS_TILE_SIZE / float(AI_TILE_SIZE * AI_TILES_IN_VIS_TILE) )
#define VIS_TILES_IN_PATCH 16
#define AI_TILES_IN_PATCH (VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE)
#define MAXIMUM_MAP_SIZE 1024
#define MAXIMUM_UNIT_TILE_RADIUS 8

// from AI to Vis coords
// 3D
inline void AI2Vis( CVec3 *pRes, const CVec3 &vPos ) { pRes->Set( vPos.x*AI_TO_VIS, vPos.y*AI_TO_VIS, vPos.z*AI_TO_VIS ); }
inline void AI2Vis( CVec3 *pRes, float x, float y, float z ) { pRes->Set( x*AI_TO_VIS, y*AI_TO_VIS, z*AI_TO_VIS ); }
inline void AI2Vis( CVec3 *pPos ) { pPos->x *= AI_TO_VIS; pPos->y *= AI_TO_VIS; pPos->z *= AI_TO_VIS; }
// 2D
inline void AI2Vis( CVec2 *pRes, const CVec2 &vPos ) { pRes->Set( vPos.x*AI_TO_VIS, vPos.y*AI_TO_VIS ); }
inline void AI2Vis( CVec2 *pRes, float x, float y ) { pRes->Set( x*AI_TO_VIS, y*AI_TO_VIS ); }
inline void AI2Vis( CVec2 *pPos ) { pPos->x *= AI_TO_VIS; pPos->y *= AI_TO_VIS; }
// 1D
inline const float AI2Vis( const float fVal ) { return fVal * AI_TO_VIS; }
inline void AI2Vis( float *pfVal ) { *pfVal *= AI_TO_VIS; }
// from Vis to AI coords
// 3D
inline void Vis2AIFast( CVec3 *pRes, const CVec3 &vPos ) { pRes->Set( vPos.x*VIS_TO_AI, vPos.y*VIS_TO_AI, vPos.z*VIS_TO_AI ); }
inline void Vis2AIFast( CVec3 *pRes, float x, float y, float z ) { pRes->Set( x*VIS_TO_AI, y*VIS_TO_AI, z*VIS_TO_AI ); }
inline void Vis2AIFast( CVec3 *pPos ) { pPos->x *= VIS_TO_AI; pPos->y *= VIS_TO_AI; pPos->z *= VIS_TO_AI; }
inline void Vis2AI( CVec3 *pPos ) { Vis2AIFast( pPos ); pPos->x = int( pPos->x + 0.3f ); pPos->y = int( pPos->y + 0.3f ); }
inline void Vis2AI( CVec3 *pPos, const CVec3 &vPos ) { Vis2AIFast( pPos, vPos ); pPos->x = int( pPos->x + 0.3f ); pPos->y = int( pPos->y + 0.3f ); }
inline void Vis2AI( CVec3 *pPos, float x, float y, float z ) { Vis2AIFast( pPos, x, y, z ); pPos->x = int( pPos->x + 0.3f ); pPos->y = int( pPos->y + 0.3f ); }
// 2D
inline void Vis2AIFast( CVec2 *pRes, const CVec2 &vPos ) { pRes->Set( vPos.x*VIS_TO_AI, vPos.y*VIS_TO_AI ); }
inline void Vis2AIFast( CVec2 *pRes, float x, float y ) { pRes->Set( x*VIS_TO_AI, y*VIS_TO_AI ); }
inline void Vis2AIFast( CVec2 *pPos ) { pPos->x *= VIS_TO_AI; pPos->y *= VIS_TO_AI; }
inline void Vis2AI( CVec2 *pPos ) { Vis2AIFast( pPos ); pPos->x = int( pPos->x + 0.3f ); pPos->y = int( pPos->y + 0.3f ); }
inline void Vis2AI( CVec2 *pPos, const CVec2 &vPos ) { Vis2AIFast( pPos, vPos ); pPos->x = int( pPos->x + 0.3f ); pPos->y = int( pPos->y + 0.3f ); }
inline void Vis2AI( CVec2 *pPos, float x, float y ) { Vis2AIFast( pPos, x, y ); pPos->x = int( pPos->x + 0.3f ); pPos->y = int( pPos->y + 0.3f ); }
// 1D
inline float Vis2AIFast( float fVal ) { return fVal * VIS_TO_AI; }
inline void Vis2AIFast( float *pfVal ) { *pfVal *= VIS_TO_AI; }
inline float Vis2AI( float fVal ) { fVal = Vis2AIFast( fVal ); return int( fVal + 0.3f ); }
inline void Vis2AI( float *pfVal ) { Vis2AIFast( pfVal ); *pfVal = int( *pfVal + 0.3f ); }

// fit to grid
inline void Fit2Grid( CVec3 *pPos, const float fGridCellSize )
{
	*pPos /= fGridCellSize;
	pPos->x = int( pPos->x + 0.5f );
	pPos->y = int( pPos->y + 0.5f );
	*pPos *= fGridCellSize;
}
inline void FitAIOrigin2AIGrid( CVec3 *pPos, const CVec2 &vOrigin )
{
	// сдвинем объект на origin
	pPos->x -= vOrigin.x;
	pPos->y -= vOrigin.y;
	//
	Fit2Grid( pPos, AI_TILE_SIZE );
	//
	pPos->x += vOrigin.x;
	pPos->y += vOrigin.y;
}
inline void FitVisOrigin2AIGrid( CVec3 *pPos, const CVec2 &vOrigin )
{
	// vis => AI with rounding to whole AI points
	Vis2AI( pPos );
	// сдвинем объект на origin
	FitAIOrigin2AIGrid( pPos, vOrigin );
	// AI => Vis
	AI2Vis( pPos );
}
inline void FitVis2AIGrid( CVec3 *pPos ) { FitVisOrigin2AIGrid( pPos, VNULL2 ); }

// AI <=> Vis graduses
inline WORD Vis2AIGrad( float fGrad ) { return WORD( Float2Int( fGrad / 360.0f * 65536.0f ) & 0x0000ffff ); }
inline float AI2VisGrad( WORD wGrad ) { return float( wGrad ) / 65536.0f * 360.0f; }
inline WORD Vis2AIRad( float fRad ) { return WORD( Float2Int( fRad / FP_2PI * 65536.0f ) & 0x0000ffff ); }
inline float AI2VisRad( WORD wGrad ) { return float( wGrad ) / 65536.0f * FP_2PI; }

inline const SVector GetAIMapTile( const float x, const float y, const int nTileSize )
{
	return SVector( x < 0.0f ? 0.0f : x / nTileSize, y < 0.0f ? 0.0f : y / nTileSize );
}

#endif // __VIS2AI_H__

