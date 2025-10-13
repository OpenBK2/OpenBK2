#pragma once

#include "vendor/granny/include/granny.h"
#include "Misc/2Darray.h"

#include <cstdint>

void GetVertices( granny_mesh *pMesh, std::vector<CVec3> *pRes, CVec3 *vMin, CVec3 *vMax );
void GetTriangles( granny_mesh *pMesh, std::vector<STriangle> *pRes );
void LoadGrannyModel( const std::string &szFileName, std::vector<CVec3> *pVerts, std::vector<STriangle> *pTrgs,
														 CVec3 *vMin, CVec3 *vMax );

void ScanFill( CArray2D<uint8_t> *pMask, const uint8_t nColor );
void ScanFillInArea( CArray2D<uint8_t> *pMask, const int nMinX, const int nMinY, const int nMaxX, const int nMaxY, const uint8_t nColor );
void DrawLine( CArray2D<uint8_t> *pArray, const int nX1, const int nY1, const int nX2, const int nY2, const uint8_t nColor );
void SimpleFill( CArray2D<uint8_t> *pArray, const int x, const int y, const uint8_t nColor,const uint8_t nBorderColor );
void WiseFill( CArray2D<uint8_t> *pMask, const uint8_t nColor );
void WiseFillInArea( CArray2D<uint8_t> *pMask, const int nMinX, const int nMinY, const int nMaxX, const int nMaxY, const uint8_t nColor );

inline void DrawRect( CArray2D<uint8_t> *pImage,
											const int x1, const int y1, const int x2, const int y2,
											const int x3, const int y3, const int x4, const int y4,
											const uint8_t nColor )
{
	DrawLine( pImage, x1, y1, x2, y2, nColor );
	DrawLine( pImage, x2, y2, x3, y3, nColor );
	DrawLine( pImage, x3, y3, x4, y4, nColor );
	DrawLine( pImage, x4, y4, x1, y1, nColor );
}

inline void SmoothColor( CArray2D<uint8_t> *pImage, const int nRadius, const uint8_t nSrcColor, const uint8_t nDstColor )
{
	for ( int g = 0; g < pImage->GetSizeY(); ++g )
	{
		for ( int i = 0; i < pImage->GetSizeX(); ++i )
		{
			if ( (*pImage)[g][i] == nSrcColor )
			{
				for ( int gg = -nRadius; gg <= nRadius; ++gg )
				{
					for ( int ii = -nRadius; ii <= nRadius; ++ii )
					{
						if ( (*pImage)[g + gg][i + ii] != nSrcColor )
							(*pImage)[g + gg][i + ii] = nDstColor;
					}
				}
			}
		}
	}
}

inline void ReplaceColor( CArray2D<uint8_t> *pImage, const uint8_t nSrcColor, const uint8_t nDstColor )
{
	for ( int g = 0; g < pImage->GetSizeY(); ++g )
	{
		for ( int i = 0; i < pImage->GetSizeX(); ++i )
		{
			if ( (*pImage)[g][i] == nSrcColor )
				(*pImage)[g][i] = nDstColor;
		}
	}
}


