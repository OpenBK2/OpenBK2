#include "StdAfx.h"

#include "GenTerrain.h"
#include "TerraHeight.h"
#include "VersionInfo.h"

float CTerraGen::GetTerraHeightWOWaters( const int nx, const int ny ) const
{
	return terrainInfo.heights[ny][nx];
}


float CTerraGen::GetTerraHeightWORivers( const int nx, const int ny ) const
{
	// TODO: new wateraplha
	//const float &fWaterCoeff = terrainInfo.waterHeightCoeffs[ny][nx];
	//return GetTerraHeightWOWaters(nx, ny) * fWaterCoeff;

	const float &fWaterCoeff = terrainInfo.waterHeightCoeffs[ny][nx];
	return GetTerraHeightWOWaters(nx, ny) * fWaterCoeff + terrainInfo.waterAddHeights[ny][nx] * ( 1.0f - 2.0f * fWaterCoeff );
}


float CTerraGen::GetTerraHeight( const int nx, const int ny ) const
{

#ifdef VERSION_DEV_M1
	return terrainInfo.heights[ny][nx];
#else // old code
	return max( GetTerraHeightWORivers(nx, ny) + terrainInfo.riverHeights[ny][nx], 0 );
#endif

}

float CTerraGen::GetTerraHeight( const float x, const float y, const int nTileX, const int nTileY ) const
{
	const float fDx = Clamp( (x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float h1 = GetTerraHeight( nTileX, nTileY );
	const float h2 = GetTerraHeight( nTileX + 1, nTileY );
	const float h3 = GetTerraHeight( nTileX, nTileY + 1 );
	const float h4 = GetTerraHeight( nTileX + 1, nTileY + 1 );
	return ( (h1 + (h2 - h1) * fDx) * (1.0f - fDy) + (h3 + (h4 - h3) * fDx) * fDy );
}

float CTerraGen::GetTerraHeight( const float x, const float y ) const
{
	const int nTileX = Clamp( int(x * DEF_INV_TILE_SIZE), 0, terrainInfo.heights.GetSizeX() - 2 );
	const int nTileY = Clamp( int(y * DEF_INV_TILE_SIZE) , 0, terrainInfo.heights.GetSizeY() - 2 );
	const float fDx = Clamp( (x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float h1 = GetTerraHeight( nTileX, nTileY );
	const float h2 = GetTerraHeight( nTileX + 1, nTileY );
	const float h3 = GetTerraHeight( nTileX, nTileY + 1 );
	const float h4 = GetTerraHeight( nTileX + 1, nTileY + 1 );
	return ( h1 + (h2 - h1) * fDx ) * ( 1.0f - fDy ) + ( h3 + (h4 - h3) * fDx ) * fDy;
}

float CTerraGen::GetTerraHeightFast( const int nTileX, const int nTileY ) const
{
	const int nx = Clamp( nTileX, 0, terrainInfo.heights.GetSizeX() - 1 );
	const int ny = Clamp( nTileY, 0, terrainInfo.heights.GetSizeY() - 1 );
	return GetFullTerraHeight( nx, ny );
}

float CTerraGen::GetFullTerraHeight( const int nx, const int ny ) const
{
	return GetTerraHeight( nx, ny ) + terrainInfo.addHeights[ny][nx];
}

float CTerraGen::GetFullTerraHeight( const float x, const float y, const int nTileX, const int nTileY ) const
{
	const float fDx = Clamp( (x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fHeight1 = GetFullTerraHeight( nTileX, nTileY );
	const float fHeight2 = GetFullTerraHeight( nTileX + 1, nTileY );
	const float fHeight3 = GetFullTerraHeight( nTileX + 1, nTileY + 1 );
	const float fHeight4 = GetFullTerraHeight( nTileX, nTileY + 1 );
	return ( (fHeight1 + (fHeight2 - fHeight1) * fDx) * (1.0f - fDy) +
		(fHeight4 + (fHeight3 - fHeight4) * fDx) * fDy );
}

float CTerraGen::GetFullTerraHeight( const float x, const float y ) const
{
	const int nTileX = Clamp( int(x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 2 );
	const int nTileY = Clamp( int(y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 2 );
	const float fDx = Clamp( (x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fHeight1 = GetFullTerraHeight( nTileX, nTileY );
	const float fHeight2 = GetFullTerraHeight( nTileX + 1, nTileY );
	const float fHeight3 = GetFullTerraHeight( nTileX + 1, nTileY + 1 );
	const float fHeight4 = GetFullTerraHeight( nTileX, nTileY + 1 );
	return ( (fHeight1 + (fHeight2 - fHeight1) * fDx) * (1.0f - fDy) +
					 (fHeight4 + (fHeight3 - fHeight4) * fDx) * fDy );
}


float CTerraGen::GetRealTerraHeight( const float x, const float y ) const
{
	float fHeight;
	GetMaxCragHeightEx( CVec2(x, y), &fHeight );
	return GetTerraHeight( x, y ) + fHeight;
}


float CTerraGen::GetRealTerraHeightFast( const int nTileX, const int nTileY ) const
{
	const int nx = Clamp( nTileX, 0, terrainInfo.heights.GetSizeX() - 1 );
	const int ny = Clamp( nTileY, 0, terrainInfo.heights.GetSizeY() - 1 );
	return terrainInfo.heights[ny][nx];
}


float CTerraGen::GetTerraHeightNative( const float x, const float y ) const
{
	const int nTileX = Clamp( int(x * DEF_INV_TILE_SIZE), 0, terrainInfo.heights.GetSizeX() - 2 );
	const int nTileY = Clamp( int(y * DEF_INV_TILE_SIZE), 0, terrainInfo.heights.GetSizeY() - 2 );
	const float fDx = Clamp( (x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float h1 = GetTerraHeightWORivers( nTileX, nTileY );
	const float h2 = GetTerraHeightWORivers( nTileX + 1, nTileY );
	const float h3 = GetTerraHeightWORivers( nTileX, nTileY + 1 );
	const float h4 = GetTerraHeightWORivers( nTileX + 1, nTileY + 1 );
	return ( h1 + (h2 - h1) * fDx ) * (1.0f - fDy) + ( h3 + (h4 - h3) * fDx ) * fDy;
}


#define DEF_PICK_OFFSET (DEF_TILE_SIZE * 1.0f)

float CTerraGen::GetUpperHeight( const STerrainInfo::SVSOPoint &rPoint ) const
{
	const CVec3 vPick1( rPoint.vPos - rPoint.vNorm * DEF_PICK_OFFSET );
	const float fPickHeight1 = GetTerraHeight( vPick1.x, vPick1.y );

	//const CVec3 vPick2( rPoint.vPos - rPoint.vNorm * DEF_PICK_OFFSET / 2.0f );
	//const float fPickHeight2 = GetTerraHeight( vPick2.x, vPick2.y );

	return fPickHeight1;
	//return max( fPickHeight, GetLowerHeight(rPoint) + DEF_CRAG_HEIGHT );
}


float CTerraGen::GetLowerHeight( const STerrainInfo::SVSOPoint &rPoint ) const
{
	const CVec3 vPick1( rPoint.vPos + rPoint.vNorm * DEF_PICK_OFFSET );
	const float fPickHeight1 = GetTerraHeight( vPick1.x, vPick1.y );

	//const CVec3 vPick2( rPoint.vPos + rPoint.vNorm * DEF_PICK_OFFSET / 2.0f );
	//const float fPickHeight2 = GetTerraHeight( vPick2.x, vPick2.y );

	return fPickHeight1;
	//return min( fPickHeight1, fPickHeight2 );
}


inline CVec3 CTerraGen::FindNormalInVertex( const CVec3 &vVert, const int nTileX, const int nTileY ) const
{
	CVec3 vResNorm( VNULL3 );
	int nNormsCount = 0;
	if ( nTileY > 0 )
	{
		if ( nTileX > 0 )
			FindNormalsInTile( terrainInfo.tiles[nTileY - 1][nTileX - 1], vVert, vResNorm, nNormsCount );
		FindNormalsInTile( terrainInfo.tiles[nTileY - 1][nTileX], vVert, vResNorm, nNormsCount );
		if ( nTileX < (terrainInfo.tiles.GetSizeX() - 1) )
			FindNormalsInTile( terrainInfo.tiles[nTileY - 1][nTileX + 1], vVert, vResNorm, nNormsCount );
	}

	if ( nTileX > 0 )
		FindNormalsInTile( terrainInfo.tiles[nTileY][nTileX - 1], vVert, vResNorm, nNormsCount );
	FindNormalsInTile( terrainInfo.tiles[nTileY][nTileX], vVert, vResNorm, nNormsCount );
	if ( nTileX < (terrainInfo.tiles.GetSizeX() - 1) )
		FindNormalsInTile( terrainInfo.tiles[nTileY][nTileX + 1], vVert, vResNorm, nNormsCount );

	if ( nTileY < (terrainInfo.tiles.GetSizeY() - 1) )
	{
		if ( nTileX > 0 )
			FindNormalsInTile( terrainInfo.tiles[nTileY + 1][nTileX - 1], vVert, vResNorm, nNormsCount );
		FindNormalsInTile( terrainInfo.tiles[nTileY + 1][nTileX], vVert, vResNorm, nNormsCount );
		if ( nTileX < (terrainInfo.tiles.GetSizeX() - 1) )
			FindNormalsInTile( terrainInfo.tiles[nTileY + 1][nTileX + 1], vVert, vResNorm, nNormsCount );
	}

	if ( nNormsCount > 0 )
		vResNorm /= nNormsCount;
	else
		vResNorm = V3_AXIS_Z;

	Normalize( &vResNorm );

	return vResNorm;
}


CVec3 CTerraGen::GetTerraNorm( const CVec3 &vPos ) const
{
	const int nTileX = Clamp( int(vPos.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY = Clamp( int(vPos.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const float fDx = Clamp( (vPos.x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (vPos.y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );

	CVec2 vBary;
	const CVec3fEx vSrc( vPos, 0 );

	const STerrainInfo::STile &tile = terrainInfo.tiles[nTileY][nTileX];
	for ( vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
	{
		const CVec3fEx &v1 = tile.vertices[it->i1];
		const CVec3fEx &v2 = tile.vertices[it->i2];
		const CVec3fEx &v3 = tile.vertices[it->i3];
		GetBaryCoords( vSrc, v1, v2, v3, &vBary );

		if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
		{
			const CVec3 vNorm1 = FindNormalInVertex( CVec3(v1.x, v1.y, max(v1.z + tile.addHeights[it->i1], 0.0f)), nTileX, nTileY );
			const CVec3 vNorm2 = FindNormalInVertex( CVec3(v2.x, v2.y, max(v2.z + tile.addHeights[it->i2], 0.0f)), nTileX, nTileY );
			const CVec3 vNorm3 = FindNormalInVertex( CVec3(v3.x, v3.y, max(v3.z + tile.addHeights[it->i3], 0.0f)), nTileX, nTileY );
			return ( vNorm2 - vNorm1 ) * vBary.x + ( vNorm3 - vNorm1 ) * vBary.y + vNorm1;
		}
	}

	return V3_AXIS_Z;
}


CVec3 CTerraGen::GetTerraNormFast( const float x, const float y, const int nTileX, const int nTileY ) const
{
	const float fDx = Clamp( (x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	return ( terrainNorms[nTileY][nTileX] + (terrainNorms[nTileY][nTileX+1] - terrainNorms[nTileY][nTileX]) * fDx ) * ( 1.0f - fDy ) +
				 ( terrainNorms[nTileY+1][nTileX] + (terrainNorms[nTileY+1][nTileX+1] - terrainNorms[nTileY+1][nTileX]) * fDx ) * fDy;
}


void CTerraGen::CreateNormals()
{
	terrainNorms.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	UpdateNormals( 0, 0, terrainInfo.heights.GetSizeX()-1, terrainInfo.heights.GetSizeY()-1 );
}


void CTerraGen::UpdateNormals( const int nX1, const int nY1, const int nX2, const int nY2 )
{
	CVec3 vNorm, vDNorm;
	int nNormsCount;
	//float fHeight;

	terrainNorms.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	for ( int g = nY1; g <= nY2; ++g )
	{
		for ( int i = nX1; i <= nX2; ++i )
		{
			//GetMaxCragHeightEx( CVec2( (float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE ), &fHeight );
			const CVec3 vCur( (float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE, GetFullTerraHeight(i, g) );
			//GetMaxCragHeightEx( CVec2( (float)( i - 1 ) * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE ), &fHeight );
			const CVec3 vPrevX( vCur.x - DEF_TILE_SIZE, vCur.y, (i > 0) ? ( GetFullTerraHeight( i - 1, g)) : vCur.z );
			//GetMaxCragHeightEx( CVec2( (float)i * DEF_TILE_SIZE, (float)( g - 1 ) * DEF_TILE_SIZE ), &fHeight );
			const CVec3 vPrevY( vCur.x, vCur.y - DEF_TILE_SIZE, (g > 0) ? ( GetFullTerraHeight( i, g - 1)) : vCur.z );
			//GetMaxCragHeightEx( CVec2( (float)( i + 1 ) * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE ), &fHeight );
			const CVec3 vNextX( vCur.x + DEF_TILE_SIZE, vCur.y, (i < ( terrainInfo.heights.GetSizeX() - 1)) ? GetFullTerraHeight(i + 1, g) : vCur.z );
			//GetMaxCragHeightEx( CVec2( (float)i * DEF_TILE_SIZE, (float)( g + 1 ) * DEF_TILE_SIZE ), &fHeight );
			const CVec3 vNextY( vCur.x, vCur.y + DEF_TILE_SIZE, (g < (terrainInfo.heights.GetSizeY() - 1)) ? GetFullTerraHeight(i , g + 1) : vCur.z );
			vNorm.Set( 0, 0, 0 );
			nNormsCount = 0;
			if ( i > 0 )
			{
				if ( g > 0 )
				{
					vDNorm = ( vPrevX - vCur ) ^ ( vPrevY - vCur );
					Normalize( &vDNorm ); 
					if ( vDNorm.z < 0.0f )
						vDNorm = -vDNorm;
					vNorm += vDNorm;
					++nNormsCount;
				}
				if ( g < (terrainInfo.heights.GetSizeY() - 1) )
				{
					vDNorm = ( vPrevX - vCur ) ^ ( vNextY - vCur );
					Normalize( &vDNorm ); 
					if ( vDNorm.z < 0.0f ) 
						vDNorm = -vDNorm;
					vNorm += vDNorm;
					++nNormsCount;
				}
			}
			if ( i < (terrainInfo.heights.GetSizeX() - 1) )
			{
				if ( g > 0 )
				{
					vDNorm = ( vNextX - vCur ) ^ ( vPrevY - vCur );
					Normalize( &vDNorm ); 
					if ( vDNorm.z < 0.0f ) 
						vDNorm = -vDNorm;
					vNorm += vDNorm;
					++nNormsCount;
				}
				if ( g < (terrainInfo.heights.GetSizeY() - 1) )
				{
					vDNorm = ( vNextX - vCur ) ^ ( vNextY - vCur );
					Normalize( &vDNorm ); 
					if ( vDNorm.z < 0.0f ) 
						vDNorm = -vDNorm;
					vNorm += vDNorm;
					++nNormsCount;
				}
			}
			terrainNorms[g][i] = vNorm * invCoeffs[nNormsCount];
		}
	}
}



