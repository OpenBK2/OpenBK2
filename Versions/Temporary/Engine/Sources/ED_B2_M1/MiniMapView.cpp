#include "stdafx.h"

#include "MiniMapView.h"

#include "Image/Image.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Scene.h"
#include "SceneB2/TerraGen.h"
#include "SceneB2/TerrainInfo.h"
#include "Stats_B2_M1/Vis2AI.h"

// The minimap's picture and arithmetic, which its window (MiniMapViewWx.cpp)
// uses.

namespace
{
	// A tile's terrain colour; a terrain type the terra set does not have is red.
	NImage::SColor GetColor( const NDb::STerrain &rTerrainDesc, const STerrainInfo &rTerrainInfo, const int nX, const int nY )
	{
		if ( rTerrainInfo.tileTerraMap[nY][nX] >= rTerrainDesc.pTerraSet->terraTypes.size() )
		{
			return NImage::SColor( 0xFF, 0xFF, 0, 0 );
		}
		return NImage::SColor( rTerrainDesc.pTerraSet->terraTypes[rTerrainInfo.tileTerraMap[nY][nX]]->nColor );
	}


	uint32_t ToPixel( uint32_t nRed, uint32_t nGreen, uint32_t nBlue )
	{
		return ( ( nRed & 0xFF ) << 16 ) | ( ( nGreen & 0xFF ) << 8 ) | ( nBlue & 0xFF );
	}
}


namespace NMiniMapView
{
	// The map turned 45 degrees into a square whose corners are the middles of
	// its sides: tile (x, y) at (x + y, x + sizeY - y - 1), and between each tile
	// and its neighbours one pixel to the left, their average -- in the order
	// the MFC window set them, so an overlap resolves the same way. The corners
	// outside the diamond are COLORREF 16: CBrush( COLOR_BTNFACE + 1 ), a system
	// colour index passed where a colour was wanted.
	bool BuildImage( const NDb::STerrain *pTerrainDesc, SImage *pImage )
	{
		if ( pImage == 0 )
		{
			return false;
		}
		( *pImage ) = SImage();
		if ( pTerrainDesc == 0 )
		{
			return false;
		}
		const STerrainInfo *const pTerrainInfo = Scene()->GetTerraManager()->GetTerraInfo();
		if ( pTerrainInfo == 0 )
		{
			return false;
		}
		const int nMapSizeX = pTerrainDesc->nNumPatchesX * VIS_TILES_IN_PATCH;
		const int nMapSizeY = pTerrainDesc->nNumPatchesY * VIS_TILES_IN_PATCH;
		const int nSide = nMapSizeX + nMapSizeY - 1;
		if ( nSide <= 0 )
		{
			return false;
		}
		pImage->nMapSizeX = nMapSizeX;
		pImage->nMapSizeY = nMapSizeY;
		pImage->nSide = nSide;
		pImage->pixels.assign( static_cast<size_t>( nSide ) * nSide, ToPixel( 16, 0, 0 ) );
		const auto put = [pImage, nSide]( int nX, int nY, uint32_t nPixel )
		{
			if ( ( nX >= 0 ) && ( nX < nSide ) && ( nY >= 0 ) && ( nY < nSide ) )
			{
				pImage->pixels[static_cast<size_t>( nY ) * nSide + nX] = nPixel;
			}
		};
		for ( int x = 0; x < nMapSizeX && x < pTerrainInfo->heights.GetSizeX() && x < pTerrainInfo->tileTerraMap.GetSizeX(); ++x )
		{
			for ( int y = 0; y < nMapSizeY && y < pTerrainInfo->heights.GetSizeY() && y < pTerrainInfo->tileTerraMap.GetSizeY(); ++y )
			{
				const int nPixelX = x + y;
				const int nPixelY = x + nMapSizeY - y - 1;
				const NImage::SColor color11 = GetColor( *pTerrainDesc, *pTerrainInfo, x, y );
				put( nPixelX, nPixelY, ToPixel( color11.r, color11.g, color11.b ) );
				if ( x > 0 && y > 0 )
				{
					const NImage::SColor color10 = GetColor( *pTerrainDesc, *pTerrainInfo, x, y - 1 );
					const NImage::SColor color01 = GetColor( *pTerrainDesc, *pTerrainInfo, x - 1, y );
					const NImage::SColor color00 = GetColor( *pTerrainDesc, *pTerrainInfo, x - 1, y - 1 );
					const uint32_t r = ( color00.r + color01.r + color10.r + color11.r ) / 4;
					const uint32_t g = ( color00.g + color01.g + color10.g + color11.g ) / 4;
					const uint32_t b = ( color00.b + color01.b + color10.b + color11.b ) / 4;
					put( nPixelX - 1, nPixelY, ToPixel( r, g, b ) );
				}
			}
		}
		return true;
	}


	// CMiniMapWindow::EditorToMiniMap.
	CVec2 EditorToMiniMap( const SImage &rImage, int nWidth, int nHeight, const CVec2 &vEditorPos )
	{
		CVec3 vNear;
		CVec3 vFar;
		Camera()->GetProjectiveRayPoints( &vNear, &vFar, vEditorPos );
		const float t = ( 0.0f - vNear.z ) / ( vFar.z - vNear.z );
		const float fBitmapX = ( ( vFar.x - vNear.x ) * t + vNear.x ) / VIS_TILE_SIZE;
		const float fBitmapY = ( ( vFar.y - vNear.y ) * t + vNear.y ) / VIS_TILE_SIZE;
		const float fSide = (float)( rImage.nMapSizeX + rImage.nMapSizeY - 1 );
		return CVec2( ( fBitmapX + fBitmapY ) * (float)nWidth / fSide,
									( fBitmapX + (float)rImage.nMapSizeY - fBitmapY - 1 ) * (float)nHeight / fSide );
	}


	// CMiniMapWindow::MiniMapToEditor.
	CVec2 MiniMapToEditor( const SImage &rImage, int nWidth, int nHeight, const CVec2 &vMiniMapPos )
	{
		const float fSide = (float)( rImage.nMapSizeX + rImage.nMapSizeY - 1 );
		const float fBitmapX = vMiniMapPos.x * fSide / (float)nWidth;
		const float fBitmapY = vMiniMapPos.y * fSide / (float)nHeight;
		return CVec2( Clamp( ( fBitmapX + fBitmapY - (float)rImage.nMapSizeY + 1.0f ) / 2.0f, 0.0f, (float)rImage.nMapSizeX ) * VIS_TILE_SIZE,
									Clamp( ( fBitmapX - fBitmapY + (float)rImage.nMapSizeX - 1.0f ) / 2.0f, 0.0f, (float)rImage.nMapSizeY ) * VIS_TILE_SIZE );
	}
}
