#include "StdAfx.h"

#include "GenTerrain.h"
#include "../B2_M1_Terrain/TracksManager.h"

#define DEF_VERTICES_NUM_ALLOC 8
#define DEF_TRIANGLES_NUM_ALLOC 6
#define DEF_TRACK_HEIGHT 0.1f

inline float GetIntersectX( const float x, const CVec2 &p1, const CVec2 &p2, CVec2 *pRes )
{
	pRes->x = x;
	if ( fabs(p2.x - p1.x) < DEF_EPS )
	{
		pRes->y = ( p1.y + p2.y ) * 0.5f;
		return 0.5f;
	}
	const float t = ( x - p1.x ) / ( p2.x - p1.x );
	pRes->y = p1.y + ( p2.y - p1.y ) * t;
	return t;
}

inline float GetIntersectY( const float y, const CVec2 &p1, const CVec2 &p2, CVec2 *pRes )
{
	pRes->y = y;
	if ( fabs(p2.y - p1.y) < DEF_EPS )
	{
		pRes->x = ( p1.x + p2.x ) * 0.5f;
		return 0.5f;
	}
	const float t = ( y - p1.y ) / ( p2.y - p1.y );
	pRes->x = p1.x + ( p2.x - p1.x ) * t;
	return t;
}

float CTerraGen::GetTrackMidHeight( const float x, const float y, const float fMidX, const float fMidY,
																					 const int nTileX, const int nTileY )
{
	float fDx, fDy;
	float fHeight1, fHeight2, fHeight3, fHeight4;
	if ( x > fMidX )
	{
		fDx = ( x - (float)(nTileX + 1) * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
		if ( y > fMidY )
		{
			fDy = ( y - (float)(nTileY + 1) * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
			fHeight1 = GetFullTerraHeight( nTileX + 1, nTileY + 1 );
			fHeight2 = GetFullTerraHeight( nTileX + 2, nTileY + 1 );
			fHeight3 = GetFullTerraHeight( nTileX + 1, nTileY + 2 );
			fHeight4 = GetFullTerraHeight( nTileX + 2, nTileY + 2 );
		}
		else
		{
			fDy = ( y - (float)nTileY * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
			fHeight1 = GetFullTerraHeight( nTileX + 1, nTileY );
			fHeight2 = GetFullTerraHeight( nTileX + 2, nTileY );
			fHeight3 = GetFullTerraHeight( nTileX + 1, nTileY + 1 );
			fHeight4 = GetFullTerraHeight( nTileX + 2, nTileY + 1 );
		}
	}
	else
	{
		fDx = ( x - (float)nTileX * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
		if ( y > fMidY )
		{
			fDy = ( y - (float)(nTileY + 1) * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
			fHeight1 = GetFullTerraHeight( nTileX, nTileY + 1 );
			fHeight2 = GetFullTerraHeight( nTileX + 1, nTileY + 1 );
			fHeight3 = GetFullTerraHeight( nTileX, nTileY + 2 );
			fHeight4 = GetFullTerraHeight( nTileX + 1, nTileY + 2 );
		}
		else
		{
			fDy = ( y - (float)nTileY * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
			fHeight1 = GetFullTerraHeight( nTileX, nTileY );
			fHeight2 = GetFullTerraHeight( nTileX + 1, nTileY );
			fHeight3 = GetFullTerraHeight( nTileX, nTileY + 1 );
			fHeight4 = GetFullTerraHeight( nTileX + 1, nTileY + 1 );
		}
	}
	return ( fHeight1 + (fHeight2 - fHeight1) * fDx ) * ( 1.0f - fDy ) + ( fHeight3 + (fHeight4 - fHeight3) * fDx ) * fDy;
}

void CTerraGen::AddTrack( const int nID, const float fFadingSpeed,
													const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4,
													const CVec2 &vNorm, const float _fWidth, const float fAplha, CTracksManager *pTracksManager )
{
	CVec2 v1 = _v1, v2 = _v2, v3 = _v3, v4 = _v4;
	AI2Vis( &v1 );
	AI2Vis( &v2 );
	AI2Vis( &v3 );
	AI2Vis( &v4 );

	if ( IsPointOnBridge(Vis2Tile(v1.x * 2.0f), Vis2Tile(v1.y * 2.0f)) ||
			 IsPointOnBridge(Vis2Tile(v2.x * 2.0f), Vis2Tile(v2.y * 2.0f)) ||
			 IsPointOnBridge(Vis2Tile(v3.x * 2.0f), Vis2Tile(v3.y * 2.0f)) ||
			 IsPointOnBridge(Vis2Tile(v4.x * 2.0f), Vis2Tile(v4.y * 2.0f)) )
	{
		return;
	}

	const float fWidth = AI2Vis( _fWidth );

	static NMeshData::SMeshData data(8, 6);
	data.vertices.resize( 0 );
	data.triangles.resize( 0 );

	const CVec2 p1( (v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f );
	const CVec2 p2( (v3.x + v4.x) * 0.5f, (v3.y + v4.y) * 0.5f );

	const float fMaxTexX = fabs( p2 - p1 ) / ( fWidth * 2.0f );

	const int nTileX = min( p1.x, p2.x ) * DEF_INV_TILE_SIZE;
	const int nTileY = min( p1.y, p2.y ) * DEF_INV_TILE_SIZE;

	if ( (nTileX < 0) || (nTileX >= (terrainInfo.tiles.GetSizeX() - 1)) ||
			 (nTileY < 0) || (nTileY >= (terrainInfo.tiles.GetSizeY() - 1)) )
		return;

	const float fMidX = (float)( nTileX + 1 ) * DEF_TILE_SIZE;
	const float fMidY = (float)( nTileY + 1 ) * DEF_TILE_SIZE;

	CVec3 vBBMin(FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE);
	CVec3 vBBMax(-FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE);

	static NGScene::SVertex vert;
	CalcCompactVector( &(vert.normal), V3_AXIS_Z );
	vert.texU.dw = Clamp( Float2Int( fAplha * 255.0f ), 0, 255 );
	CalcCompactVector( &(vert.texV), V3_AXIS_Y );

	const float fGH1 = GetFullTerraHeight( v1.x, v1.y, v1.x > fMidX ? nTileX + 1 : nTileX, v1.y > fMidY ? nTileY + 1 : nTileY );
	//const float fAIH1 = AI2Vis( GetAIObserver()->GetZ(v1.x, v1.y) );
	const float fHeight1 = fGH1;
	vert.pos.Set( v1.x, v1.y, fHeight1 + DEF_TRACK_HEIGHT );
	vert.tex.Set( 0.0f, 0.0f );
	data.vertices.push_back( vert );
	vBBMin.Minimize( vert.pos );
	vBBMax.Maximize( vert.pos );
	//
	const float fGH2 = GetFullTerraHeight( v2.x, v2.y, v2.x > fMidX ? nTileX + 1 : nTileX, v2.y > fMidY ? nTileY + 1 : nTileY );
	//const float fAIH2 = AI2Vis( GetAIObserver()->GetZ(v2.x, v2.y) );
	const float fHeight2 = fGH2;
	vert.pos.Set( v2.x, v2.y, fHeight2 + DEF_TRACK_HEIGHT );
	vert.tex.Set( 0.0f, 1.0f );
	data.vertices.push_back( vert );
	vBBMin.Minimize( vert.pos );
	vBBMax.Maximize( vert.pos );
	//
	const float fGH4 = GetFullTerraHeight( v4.x, v4.y, v4.x > fMidX ? nTileX + 1 : nTileX, v4.y > fMidY ? nTileY + 1 : nTileY );
	//const float fAIH4 = AI2Vis( GetAIObserver()->GetZ(v4.x, v4.y) );
	const float fHeight4 = fGH4;
	vert.pos.Set( v4.x, v4.y, fHeight4 + DEF_TRACK_HEIGHT );
	vert.tex.Set( fMaxTexX, 0.0f );
	data.vertices.push_back( vert );
	vBBMin.Minimize( vert.pos );
	vBBMax.Maximize( vert.pos );
	//
	const float fGH3 = GetFullTerraHeight( v3.x, v3.y, v3.x > fMidX ? nTileX + 1 : nTileX, v3.y > fMidY ? nTileY + 1 : nTileY );
	//const float fAIH3 = AI2Vis( GetAIObserver()->GetZ(v3.x, v3.y) );
	const float fHeight3 = fGH3;
	vert.pos.Set( v3.x, v3.y, fHeight3 + DEF_TRACK_HEIGHT );
	vert.tex.Set( fMaxTexX, 1.0f );
	data.vertices.push_back( vert );
	vBBMin.Minimize( vert.pos );
	vBBMax.Maximize( vert.pos );

	CVec2 vInters1, vInters2;
	STriangle trg;

	if ( fMidX < max(p1.x, p2.x) )
	{
		const float t1 = GetIntersectX( fMidX, p1, p2, &vInters1 );
		vert.pos.Set( vInters1.x + vNorm.x * fWidth, vInters1.y + vNorm.y * fWidth, DEF_TRACK_HEIGHT );
		vert.pos.z += GetTrackMidHeight( vert.pos.x, vert.pos.y, fMidX, fMidY, nTileX, nTileY );
		vert.tex.Set( t1 * fMaxTexX, 0.0f );
		data.vertices.push_back( vert );
		vBBMin.Minimize( vert.pos );
		vBBMax.Maximize( vert.pos );
		vert.pos.Set( vInters1.x - vNorm.x * fWidth, vInters1.y - vNorm.y * fWidth, DEF_TRACK_HEIGHT );
		vert.pos.z += GetTrackMidHeight( vert.pos.x, vert.pos.y, fMidX, fMidY, nTileX, nTileY );
		vert.tex.y = 1.0f;
		data.vertices.push_back( vert );
		vBBMin.Minimize( vert.pos );
		vBBMax.Maximize( vert.pos );
		if ( fMidY < max( p1.y, p2.y ) ) // there are two intersections
		{
			const float t2 = GetIntersectY( fMidY, p1, p2, &vInters2 );
			vert.pos.Set( vInters2.x + vNorm.x * fWidth, vInters2.y + vNorm.y * fWidth, DEF_TRACK_HEIGHT );
			vert.pos.z += GetTrackMidHeight( vert.pos.x, vert.pos.y, fMidX, fMidY, nTileX, nTileY );
			vert.tex.Set( t2 * fMaxTexX, 0.0f );
			data.vertices.push_back( vert );
			vBBMin.Minimize( vert.pos );
			vBBMax.Maximize( vert.pos );
			vert.pos.Set( vInters2.x - vNorm.x * fWidth, vInters2.y - vNorm.y * fWidth, DEF_TRACK_HEIGHT );
			vert.pos.z += GetTrackMidHeight( vert.pos.x, vert.pos.y, fMidX, fMidY, nTileX, nTileY );
			vert.tex.y = 1.0f;
			data.vertices.push_back( vert );
			vBBMin.Minimize( vert.pos );
			vBBMax.Maximize( vert.pos );
			if ( t1 < t2 )
			{
				trg.i1 = 0; trg.i2 = 4; trg.i3 = 5;
				data.triangles.push_back( trg );
				trg.i1 = 5; trg.i2 = 1; trg.i3 = 0;
				data.triangles.push_back( trg );
				trg.i1 = 4; trg.i2 = 6; trg.i3 = 7;
				data.triangles.push_back( trg );
				trg.i1 = 7; trg.i2 = 5; trg.i3 = 4;
				data.triangles.push_back( trg );
				trg.i1 = 6; trg.i2 = 2; trg.i3 = 3;
				data.triangles.push_back( trg );
				trg.i1 = 3; trg.i2 = 7; trg.i3 = 6;
				data.triangles.push_back( trg );
			}
			else
			{
				trg.i1 = 0; trg.i2 = 6; trg.i3 = 7;
				data.triangles.push_back( trg );
				trg.i1 = 7; trg.i2 = 1; trg.i3 = 0;
				data.triangles.push_back( trg );
				trg.i1 = 6; trg.i2 = 4; trg.i3 = 5;
				data.triangles.push_back( trg );
				trg.i1 = 5; trg.i2 = 7; trg.i3 = 6;
				data.triangles.push_back( trg );
				trg.i1 = 4; trg.i2 = 2; trg.i3 = 3;
				data.triangles.push_back( trg );
				trg.i1 = 3; trg.i2 = 5; trg.i3 = 4;
				data.triangles.push_back( trg );
			}
		}
		else // there is one intersection with vertical
		{
			trg.i1 = 0; trg.i2 = 4; trg.i3 = 5;
			data.triangles.push_back( trg );
			trg.i1 = 5; trg.i2 = 1; trg.i3 = 0;
			data.triangles.push_back( trg );
			trg.i1 = 4; trg.i2 = 2; trg.i3 = 3;
			data.triangles.push_back( trg );
			trg.i1 = 3; trg.i2 = 5; trg.i3 = 4;
			data.triangles.push_back( trg );
		}
	}
	else
	{
		if ( fMidY < max( p1.y, p2.y ) ) // there is one intersection with horizontal
		{
			const float t2 = GetIntersectY( fMidY, p1, p2, &vInters2 );
			vert.pos.Set( vInters2.x + vNorm.x * fWidth, vInters2.y + vNorm.y * fWidth, DEF_TRACK_HEIGHT );
			vert.pos.z += GetTrackMidHeight( vert.pos.x, vert.pos.y, fMidX, fMidY, nTileX, nTileY );
			vert.tex.Set( t2 * fMaxTexX, 0.0f );
			data.vertices.push_back( vert );
			vBBMin.Minimize( vert.pos );
			vBBMax.Maximize( vert.pos );
			vert.pos.Set( vInters2.x - vNorm.x * fWidth, vInters2.y - vNorm.y * fWidth, DEF_TRACK_HEIGHT );
			vert.pos.z += GetTrackMidHeight( vert.pos.x, vert.pos.y, fMidX, fMidY, nTileX, nTileY );
			vert.tex.y = 1.0f;
			data.vertices.push_back( vert );
			vBBMin.Minimize( vert.pos );
			vBBMax.Maximize( vert.pos );
			trg.i1 = 0; trg.i2 = 4; trg.i3 = 5;
			data.triangles.push_back( trg );
			trg.i1 = 5; trg.i2 = 1; trg.i3 = 0;
			data.triangles.push_back( trg );
			trg.i1 = 4; trg.i2 = 2; trg.i3 = 3;
			data.triangles.push_back( trg );
			trg.i1 = 3; trg.i2 = 5; trg.i3 = 4;
			data.triangles.push_back( trg );
		}
		else //	there are no intersections
		{
			trg.i1 = 0; trg.i2 = 2; trg.i3 = 3;
			data.triangles.push_back( trg );
			trg.i1 = 3; trg.i2 = 1; trg.i3 = 0;
			data.triangles.push_back( trg );
		}
	}

	pTracksManager->AddTrack( nID, fFadingSpeed, data, vBBMin, vBBMax );
}


