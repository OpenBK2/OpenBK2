#include "StdAfx.h"

#include "../Misc/Win32Helper.h"
#include "../System/Commands.h"
#include "GenTerrain.h"

bool g_bDontUpdateRoads = false;
static bool s_bMakeHolesForEntrenchment = true;

void CTerraGen::MakeHoleOnTile( const vector<CVec3dEx> &samples, const int nTileX, const int nTileY, const bool bLeftHeighten )
{
	vector<CVec3dEx> newVerts( 512 );
	vector<CVec3dEx> tileVertices( 512 );
	vector<STriangle> tileTriangles( 512 );

	tileVertices.resize( 0 );
	tileTriangles.resize( 0 );
	CTriangleEx curTrg;
	CVec2 vBary;
	for ( vector<STriangle>::const_iterator it = terrainInfo.tiles[nTileY][nTileX].triangles.begin(); it != terrainInfo.tiles[nTileY][nTileX].triangles.end(); ++it )
	{
		newVerts.resize( 0 );
		curTrg.Set( terrainInfo.tiles[nTileY][nTileX].vertices[it->i1],
								terrainInfo.tiles[nTileY][nTileX].vertices[it->i2],
								terrainInfo.tiles[nTileY][nTileX].vertices[it->i3] );

		// add triangles vertices, which are placed outside sample curve
		if ( IsOutside(samples, curTrg.points[0]) )
			newVerts.push_back( curTrg.points[0] );
		if ( IsOutside(samples, curTrg.points[1]) )
			newVerts.push_back( curTrg.points[1] );
		if ( IsOutside(samples, curTrg.points[2]) )
			newVerts.push_back( curTrg.points[2] );

		// add intersections between sample curve and triangle
		GetIntersection( &newVerts, samples, curTrg.points[0], curTrg.points[1] );
		GetIntersection( &newVerts, samples, curTrg.points[1], curTrg.points[2] );
		GetIntersection( &newVerts, samples, curTrg.points[2], curTrg.points[0] );

		if ( newVerts.size() < 3 ) // triangle is placed completely inside sample curve
			continue;

		// add sample curve's vertices, which are placed inside triangle
		for ( vector<CVec3dEx>::const_iterator itPoint = samples.begin(); itPoint != samples.end(); ++itPoint )
		{
			if ( IsInside(curTrg, *itPoint) )
				newVerts.push_back( CVec3dEx(itPoint->x, itPoint->y, itPoint->z, itPoint->flag) );
		}

		// create triangles
		for ( int i1 = 0; i1 < newVerts.size(); ++i1 )
		{
			for ( int i2 = i1 + 1; i2 < newVerts.size(); ++i2 )
			{
				for ( int i3 = i2 + 1; i3 < newVerts.size(); ++i3 )
				{
					// checks, that new triangle not intersect triangles, which was previously collected
					if ( (!IsIntersect(tileTriangles, tileVertices, newVerts[i1], newVerts[i2])) &&
							 (!IsIntersect(tileTriangles, tileVertices, newVerts[i2], newVerts[i3])) &&
							 (!IsIntersect(tileTriangles, tileVertices, newVerts[i3], newVerts[i1])) )
					{
						// checks, that new triangle not intersect sample curve
						if ( (!IsIntersect(samples, newVerts[i1], newVerts[i2])) &&
								 (!IsIntersect(samples, newVerts[i2], newVerts[i3])) &&
								 (!IsIntersect(samples, newVerts[i3], newVerts[i1])) )
						{
							// checks, that new triangle is not placed on the hole
							if ( (!IsInside( samples, CVec3dEx((newVerts[i1].x + newVerts[i2].x) * 0.5,
									 (newVerts[i1].y + newVerts[i2].y) * 0.5,
									 (newVerts[i1].z + newVerts[i2].z) * 0.5, 0))) &&
									 (!IsInside( samples, CVec3dEx((newVerts[i2].x + newVerts[i3].x) * 0.5,
									 (newVerts[i2].y + newVerts[i3].y) * 0.5,
									 (newVerts[i2].z + newVerts[i3].z) * 0.5, 0))) &&
									 (!IsInside( samples, CVec3dEx((newVerts[i3].x + newVerts[i1].x) * 0.5,
									 (newVerts[i3].y + newVerts[i1].y) * 0.5,
									 (newVerts[i3].z + newVerts[i1].z) * 0.5, 0))) )
							{
								if ( !IsTrgSingular(newVerts[i1], newVerts[i2], newVerts[i3]) )
								{
									const int nInd1 = AddUnique( &tileVertices, CVec3dEx(newVerts[i1].x, newVerts[i1].y, newVerts[i1].z, newVerts[i1].flag) );
									const int nInd2 = AddUnique( &tileVertices, CVec3dEx(newVerts[i2].x, newVerts[i2].y, newVerts[i2].z, newVerts[i2].flag) );
									const int nInd3 = AddUnique( &tileVertices, CVec3dEx(newVerts[i3].x, newVerts[i3].y, newVerts[i3].z, newVerts[i3].flag) );
									if ( IsCCW(tileVertices[nInd1], tileVertices[nInd2], tileVertices[nInd3]) )
										tileTriangles.push_back( STriangle(nInd1, nInd2, nInd3) );
									else
										tileTriangles.push_back( STriangle(nInd1, nInd3, nInd2) );
								}
							}
						}
					}
				}
			}
		}
	}

	terrainInfo.tiles[nTileY][nTileX].vertices.resize( tileVertices.size() );
	for ( int i = 0; i < tileVertices.size(); ++i )
	{
		const CVec3dEx &vertex = tileVertices[i];
		terrainInfo.tiles[nTileY][nTileX].vertices[i] = CVec3fEx( vertex.x, vertex.y, vertex.z, vertex.flag );
	}

	terrainInfo.tiles[nTileY][nTileX].addHeights.resize( tileVertices.size() );
	fill( terrainInfo.tiles[nTileY][nTileX].addHeights.begin(), terrainInfo.tiles[nTileY][nTileX].addHeights.end(), 0.0f );
	terrainInfo.tiles[nTileY][nTileX].triangles = tileTriangles;
}

void CTerraGen::MakeHole( const vector<CVec3fEx> &samples, const int nTileX1, const int nTileY1,
													const int nTileX2, const int nTileY2, const bool bLeftHeighten /*= true*/ )
{
	if ( samples.empty() )
		return;

	NWin32Helper::CPrecisionControl precControl( NWin32Helper::CPrecisionControl::PCM_HIGH );

	const int nSamplesNum = samples.size() >> 1;
	vector<CVec3dEx> newSamples( 4 );
	for ( int k = 0; k < (nSamplesNum - 1); ++k )
	{
		newSamples[0] = samples[k];
		newSamples[1] = samples[k + 1];
		newSamples[2] = samples[samples.size() - 2 - k];
		newSamples[3] = samples[samples.size() - 1 - k];

		if ( IsTrgSingular( newSamples[0], newSamples[1], newSamples[2] ) && IsTrgSingular( newSamples[0], newSamples[1], newSamples[3] ) )
			continue;

		const double fMinX = min( min(newSamples[0].x, newSamples[1].x), min(newSamples[2].x, newSamples[3].x) );
		const double fMinY = min( min(newSamples[0].y, newSamples[1].y), min(newSamples[2].y, newSamples[3].y) );
		const double fMaxX = max( max(newSamples[0].x, newSamples[1].x), max(newSamples[2].x, newSamples[3].x) );
		const double fMaxY = max( max(newSamples[0].y, newSamples[1].y), max(newSamples[2].y, newSamples[3].y) );

		const int nx1 = max( nTileX1, int(fMinX * DEF_INV_TILE_SIZE) );
		const int ny1 = max( nTileY1, int(fMinY * DEF_INV_TILE_SIZE) );
		const int nx2 = min( nTileX2, int(fMaxX * DEF_INV_TILE_SIZE) );
		const int ny2 = min( nTileY2, int(fMaxY * DEF_INV_TILE_SIZE) );

		//const int nx1 = Clamp( int( fMinX * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
		//const int ny1 = Clamp( int( fMinY * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );
		//const int nx2 = Clamp( int( fMaxX * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
		//const int ny2 = Clamp( int( fMaxY * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );

		for ( int g = ny1; g <= ny2; ++g )
		{
			for ( int i = nx1; i <= nx2; ++i )
			{
				MakeHoleOnTile( newSamples, i, g, bLeftHeighten );
			}
		}
	}
}

void CTerraGen::AddEntrenchment( const vector<CVec2> &_ctrlPoints, const float _fWidth, const bool bWriteHistory/* = true*/ )
{
	if ( !s_bMakeHolesForEntrenchment || _ctrlPoints.empty() )
		return;

	if ( bWriteHistory )
	{
		SEntrenchmentHistory history;
		history.ctrlPoints = _ctrlPoints;
		history.fWidth = _fWidth;
		entrenchmentsHistory.push_back( history );
	}

	vector<CVec2> norms( _ctrlPoints.size() );
	fill( norms.begin(), norms.end(), VNULL2 );
	CVec2 vNorm;
	for ( int i = 0; i < (_ctrlPoints.size() - 1); ++i )
	{
		vNorm.x = _ctrlPoints[i+1].y - _ctrlPoints[i].y;
		vNorm.y = _ctrlPoints[i].x - _ctrlPoints[i+1].x;
		Normalize( &vNorm );
		norms[i] += vNorm;
		norms[i+1] += vNorm;
	}
	for ( int i = 0; i < norms.size(); ++i )
	{
		CVec2 &vCurNorm = norms[i];
		Normalize( &vCurNorm );
	}

	const float fWidth = AI2Vis( _fWidth );

	vector<CVec3fEx> samples( _ctrlPoints.size() * 2 );
	CVec2 vMin(FP_MAX_VALUE, FP_MAX_VALUE), vMax(-FP_MAX_VALUE, -FP_MAX_VALUE);
	for ( int i = 0; i < _ctrlPoints.size(); ++i )
	{
		samples[i].Set( AI2Vis(_ctrlPoints[i].x) - norms[i].x * fWidth, AI2Vis(_ctrlPoints[i].y) - norms[i].y * fWidth, 0.0f, 0 );
		samples[samples.size()-1-i].Set( AI2Vis(_ctrlPoints[i].x) + norms[i].x * fWidth, AI2Vis(_ctrlPoints[i].y) + norms[i].y * fWidth, 0.0f, 0 );
		vMin.Minimize( CVec2(samples[i].x, samples[i].y) );
		vMin.Minimize( CVec2(samples[samples.size()-1-i].x, samples[samples.size()-1-i].y) );
		vMax.Maximize( CVec2(samples[i].x, samples[i].y) );
		vMax.Maximize( CVec2(samples[samples.size()-1-i].x, samples[samples.size()-1-i].y) );
	}
	const CVec2i vMinTile( Clamp(int(vMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1),
												 Clamp(int(vMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1) );
	const CVec2i vMaxTile( Clamp(int(vMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1),
												 Clamp(int(vMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1) );
	MakeHole( samples, vMinTile.x, vMinTile.y, vMaxTile.x, vMaxTile.y );

	UpdateArea( vMinTile.x, vMinTile.y, vMaxTile.x + 1, vMaxTile.y + 1, TERRAIN_UPDATE_GFX );
	g_bDontUpdateRoads = true;
	UpdateAllOnTerrainObjectsInArea( vMinTile, vMaxTile );
	g_bDontUpdateRoads = false;
}

START_REGISTER(GView)
REGISTER_VAR_EX( "misc_holes_for_entrenchment", NGlobal::VarBoolHandler, &s_bMakeHolesForEntrenchment, true, STORAGE_USER )
FINISH_REGISTER

