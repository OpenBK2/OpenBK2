#pragma once

#include "ImageColor.h"

#include <cstdint>

#include "Image_export.h"

namespace NImage
{

EXTERNVAR const uint32_t BLACK_COLOR;
IMAGE_EXPORT EXTERNVAR const uint32_t WHITE_COLOR;
IMAGE_EXPORT EXTERNVAR const uint32_t GRAY_LIGHTER_COLOR;
EXTERNVAR const uint32_t GRAY_DARKER_COLOR;
EXTERNVAR const uint32_t BASE_EMBOSS_COLOR;


void GetImageParams( uint32_t *pdwMinColor, uint32_t *pdwMaxColor, uint32_t *pdwAverageColor, const CArray2D<uint32_t> &rImage );
void Invert( CArray2D<uint32_t> *pImage );
IMAGE_EXPORT void FullColor( CArray2D<uint32_t> *pImage, float fRatio );
void GammaCorrection( CArray2D<uint32_t> *pImage, float fBrightness, float fContrast, float fGamma ); // [-1...1]
//
void ApplyFilter( CArray2D<uint32_t> *pImage, const CArray2D<int> &rFilter, uint32_t dwMinAlpha );
IMAGE_EXPORT void MarkEdge( CArray2D<uint32_t> *pImage, uint32_t dwEdgeColor, bool bOutside, uint32_t dwMinAlpha );
void EraseEdge( CArray2D<uint32_t> *pImage, uint32_t dwMinAlpha );
//
void Emboss( CArray2D<uint32_t> *pImage, const CTPoint<int> &rShiftPoint, const CArray2D<int> &rFilter, uint32_t dwMinAlpha );
IMAGE_EXPORT void Noise( CArray2D<uint32_t> *pImage, const CArray2D<uint32_t> &rNoise, bool bEqualize, uint32_t dwMinAlpha );
//
IMAGE_EXPORT void FastAddImageByAlpha( CArray2D<uint32_t> *pDestImage, const CArray2D<uint32_t> &rSourceImage, uint32_t dwMinAlpha );
void FastAddImageByColor( CArray2D<uint32_t> *pDestImage, const CArray2D<uint32_t> &rSourceImage, uint32_t dwColor, bool bInclude );
//
IMAGE_EXPORT void GetShadow( CArray2D<uint32_t> *pDestImage, const CArray2D<uint32_t> &rSourceImage, const CTPoint<int> &rShiftPoint, uint32_t dwShadowColor, uint32_t dwNonShadowColor, uint32_t dwMinAlpha );
IMAGE_EXPORT void GetAlphaEmboss( CArray2D<uint32_t> *pDestImage, const CArray2D<uint32_t> &rSourceImage, const CTPoint<int> &rShiftPoint, int nFilterSize, uint32_t dwMinAlpha );
//
void DrawLine( CArray2D<uint32_t> *pImage, const struct SVector &vStart, const struct SVector &vEnd, const struct SColor &color );
//

template<class TFunctuinal, class TWBCFunctional>
void ApplyFunctional( const CTPoint<int> &rSize, const CTPoint<int> &rCheck, TFunctuinal *pFunctional, TWBCFunctional  *pWBCFunctional )
{
	NI_ASSERT( pFunctional != 0, "Wrong parameter: pFunctional == 0" );
	NI_ASSERT( pWBCFunctional != 0, "Wrong parameter: pWBCFunctional == 0" );
	//
	const CTRect<int> indices( rCheck.x,
														 rCheck.y,
														 rSize.x - rCheck.x,
														 rSize.y - rCheck.y );
	//середина
	//********
	//********
	//**0000**
	//**0000**
	//**0000**
	//**0000**
	//********
	//********
	for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
	{
		for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
		{
			( *pFunctional )( nXIndex, nYIndex );
		}
	}
	//левый и правый края
	//00****00
	//00****00
	//00****00
	//00****00
	//00****00
	//00****00
	//00****00
	//00****00
	for ( int nYIndex = 0; nYIndex < rSize.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < indices.minx; ++nXIndex )
		{
			( *pWBCFunctional )( nXIndex, nYIndex );
		}
		for ( int nXIndex = indices.maxx; nXIndex < rSize.x; ++nXIndex )
		{
			( *pWBCFunctional )( nXIndex, nYIndex );
		}
	}
	//верхний и нижний края
	//**0000**
	//**0000**
	//********
	//********
	//********
	//********
	//**0000**
	//**0000**
	for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < indices.miny; ++nYIndex )
		{
			( *pWBCFunctional )( nXIndex, nYIndex );
		}
		for ( int nYIndex = indices.maxy; nYIndex < rSize.y; ++nYIndex )
		{
			( *pWBCFunctional )( nXIndex, nYIndex );
		}
	}
}


//функционал применяющий данный паттерн к указанной точке картинки
struct SApplyFilterFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	const CArray2D<int> *pFilter;
	int nDivider;
	CTPoint<int> filterHalfSize; 

	SApplyFilterFunctional( CArray2D<uint32_t> *_pDestImage,
													const CArray2D<uint32_t> *_pSourceImage,
													const CArray2D<int> *_pFilter,
													int _nDivider )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage ), pFilter( _pFilter ), nDivider( _nDivider )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( pFilter != 0, "Wrong parameter: pFilter == 0" );
		NI_ASSERT( nDivider != 0, "Wrong parameter: nDivider == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
		NI_ASSERT( ( ( pFilter->GetSizeX() & 0x1 ) != 0 ) &&
							 ( ( pFilter->GetSizeY() & 0x1 ) != 0 ),
							"Filter must have odd sizes." );
		filterHalfSize.x = pFilter->GetSizeX() / 2;
		filterHalfSize.y = pFilter->GetSizeY() / 2;
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		CTRect<int> localIndices( nXIndex - filterHalfSize.x,
															nYIndex - filterHalfSize.y,
															nXIndex + filterHalfSize.x,
															nYIndex + filterHalfSize.y );
		uint32_t dwRed = 0;
		uint32_t dwGreen = 0;
		uint32_t dwBlue = 0;
		for ( int nFilterYIndex = 0, nLocalYIndex = localIndices.miny;
					nLocalYIndex <= localIndices.maxy;
					++nFilterYIndex, ++nLocalYIndex )
		{
			for ( int nFilterXIndex = 0, nLocalXIndex = localIndices.minx; 
						nLocalXIndex <= localIndices.maxx;
						++nFilterXIndex, ++nLocalXIndex )
			{
				int nValue = ( *pFilter )[nFilterYIndex][nFilterXIndex];
				const uint32_t dwColor = ( *pSourceImage )[nLocalYIndex][nLocalXIndex];
				dwRed += GetRedFromARGBColor( dwColor ) * nValue;
				dwGreen += GetGreenFromARGBColor( dwColor ) * nValue;
				dwBlue += GetBlueFromARGBColor( dwColor ) * nValue;
			}
		}
		dwRed = Clamp<int>( dwRed / nDivider, 0, 0xFF );
		dwGreen = Clamp<int>( dwGreen / nDivider, 0, 0xFF );
		dwBlue = Clamp<int>( dwBlue / nDivider, 0, 0xFF );
		//
		uint32_t dwDestColor = MakeARGBColor<uint32_t>( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ),
																							dwRed, dwGreen, dwBlue );
		( *pDestImage )[nYIndex][nXIndex] = dwDestColor;
	}
};


//функционал применяющий данный паттерн к указанной точке картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCApplyFilterFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	const CArray2D<int> *pFilter;
	int nDivider;
	CTPoint<int> size; 
	CTPoint<int> filterHalfSize; 

	SWBCApplyFilterFunctional( CArray2D<uint32_t> *_pDestImage,
														 const CArray2D<uint32_t> *_pSourceImage,
														 const CArray2D<int> *_pFilter,
														 int _nDivider )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage ), pFilter( _pFilter ), nDivider( _nDivider )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( pFilter != 0, "Wrong parameter: pFilter == 0" );
		NI_ASSERT( nDivider != 0, "Wrong parameter: nDivider == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
		NI_ASSERT( ( ( pFilter->GetSizeX() & 0x1 ) != 0 ) &&
							 ( ( pFilter->GetSizeY() & 0x1 ) != 0 ),
							"Filter must have odd sizes." );
		size.x = pSourceImage->GetSizeX();
		size.y = pSourceImage->GetSizeY();
		filterHalfSize.x = pFilter->GetSizeX() / 2;
		filterHalfSize.y = pFilter->GetSizeY() / 2;
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		CTRect<int> localIndices( nXIndex - filterHalfSize.x,
															nYIndex - filterHalfSize.y,
															nXIndex + filterHalfSize.x,
															nYIndex + filterHalfSize.y );
		uint32_t dwRed = 0;
		uint32_t dwGreen = 0;
		uint32_t dwBlue = 0;
		for ( int nFilterYIndex = 0, nLocalYIndex = localIndices.miny;
					nLocalYIndex <= localIndices.maxy;
					++nFilterYIndex, ++nLocalYIndex )
		{
			for ( int nFilterXIndex = 0, nLocalXIndex = localIndices.minx; 
						nLocalXIndex <= localIndices.maxx;
						++nFilterXIndex, ++nLocalXIndex )
			{
				//
				const int nXPos = Clamp<int>( nLocalXIndex, 0, size.x - 1 );
				const int nYPos = Clamp<int>( nLocalYIndex, 0, size.y - 1 );
				//
				int nValue = ( *pFilter )[nFilterYIndex][nFilterXIndex];
				const uint32_t dwColor = ( *pSourceImage )[nYPos][nXPos];
				dwRed += GetRedFromARGBColor( dwColor ) * nValue;
				dwGreen += GetGreenFromARGBColor( dwColor ) * nValue;
				dwBlue += GetBlueFromARGBColor( dwColor ) * nValue;
			}
		}
		dwRed = Clamp<int>( dwRed / nDivider, 0, 0xFF );
		dwGreen = Clamp<int>( dwGreen / nDivider, 0, 0xFF );
		dwBlue = Clamp<int>( dwBlue / nDivider, 0, 0xFF );
		//
		uint32_t dwDestColor = MakeARGBColor<uint32_t>( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ),
																							dwRed, dwGreen, dwBlue );
		( *pDestImage )[nYIndex][nXIndex] = dwDestColor;
	}
};


//функционал помечающий край картинки
struct SMarkOutsideEdgeFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	uint32_t dwEdgeColor;
	uint32_t dwMinAlpha;

	SMarkOutsideEdgeFunctional( CArray2D<uint32_t> *_pDestImage,
															const CArray2D<uint32_t> *_pSourceImage,
															uint32_t _dwEdgeColor,
															uint32_t _dwMinAlpha )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage ), dwEdgeColor( _dwEdgeColor ), dwMinAlpha( _dwMinAlpha )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ) < dwMinAlpha )
		{
			for ( int nLocalYIndex = ( nYIndex - 1 ); nLocalYIndex <= ( nYIndex + 1 ); ++nLocalYIndex )
			{
				for ( int nLocalXIndex = ( nXIndex - 1 ); nLocalXIndex <= ( nXIndex + 1 ); ++nLocalXIndex )
				{
					if ( GetAlphaFromARGBColor( ( *pSourceImage )[nLocalYIndex][nLocalXIndex] ) >= dwMinAlpha )
					{
						( *pDestImage )[nYIndex][nXIndex] = dwEdgeColor;
						return;
					}
				}
			}
		}
	}
};


//функционал помечающий край картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCMarkOutsideEdgeFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	uint32_t dwEdgeColor;
	uint32_t dwMinAlpha;
	CTPoint<int> size; 

	SWBCMarkOutsideEdgeFunctional( CArray2D<uint32_t> *_pDestImage,
																 const CArray2D<uint32_t> *_pSourceImage,
																 uint32_t _dwEdgeColor,
																 uint32_t _dwMinAlpha )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage ), dwEdgeColor( _dwEdgeColor ), dwMinAlpha( _dwMinAlpha )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
		size.x = pSourceImage->GetSizeX();
		size.y = pSourceImage->GetSizeY();
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ) < dwMinAlpha )
		{
			for ( int nLocalYIndex = ( nYIndex - 1 ); nLocalYIndex <= ( nYIndex + 1 ); ++nLocalYIndex )
			{
				for ( int nLocalXIndex = ( nXIndex - 1 ); nLocalXIndex <= ( nXIndex + 1 ); ++nLocalXIndex )
				{
					if ( ( nLocalXIndex >= 0 ) && 
							 ( nLocalYIndex >= 0 ) &&
							 ( nLocalXIndex < size.x ) &&
							 ( nLocalYIndex < size.y ) )
					{
						if ( GetAlphaFromARGBColor( ( *pSourceImage )[nLocalYIndex][nLocalXIndex] ) >= dwMinAlpha )
						{
							( *pDestImage )[nYIndex][nXIndex] = dwEdgeColor;
							return;
						}
					}
				}
			}
		}
	}
};


//функционал помечающий край картинки
struct SMarkInsideEdgeFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	uint32_t dwEdgeColor;
	uint32_t dwMinAlpha;

	SMarkInsideEdgeFunctional( CArray2D<uint32_t> *_pDestImage,
														 const CArray2D<uint32_t> *_pSourceImage,
														 uint32_t _dwEdgeColor,
														 uint32_t _dwMinAlpha )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage ), dwEdgeColor( _dwEdgeColor ), dwMinAlpha( _dwMinAlpha )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ) >= dwMinAlpha )
		{
			for ( int nLocalYIndex = ( nYIndex - 1 ); nLocalYIndex <= ( nYIndex + 1 ); ++nLocalYIndex )
			{
				for ( int nLocalXIndex = ( nXIndex - 1 ); nLocalXIndex <= ( nXIndex + 1 ); ++nLocalXIndex )
				{
					if ( GetAlphaFromARGBColor( ( *pSourceImage )[nLocalYIndex][nLocalXIndex] ) < dwMinAlpha )
					{
						( *pDestImage )[nYIndex][nXIndex] = dwEdgeColor;
						return;
					}
				}
			}
		}
	}
};


//функционал помечающий край картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCMarkInsideEdgeFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	uint32_t dwEdgeColor;
	uint32_t dwMinAlpha;
	CTPoint<int> size; 

	SWBCMarkInsideEdgeFunctional( CArray2D<uint32_t> *_pDestImage,
																const CArray2D<uint32_t> *_pSourceImage,
																uint32_t _dwEdgeColor,
																uint32_t _dwMinAlpha )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage ), dwEdgeColor( _dwEdgeColor ), dwMinAlpha( _dwMinAlpha )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
		size.x = pSourceImage->GetSizeX();
		size.y = pSourceImage->GetSizeY();
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ) >= dwMinAlpha )
		{
			for ( int nLocalYIndex = ( nYIndex - 1 ); nLocalYIndex <= ( nYIndex + 1 ); ++nLocalYIndex )
			{
				for ( int nLocalXIndex = ( nXIndex - 1 ); nLocalXIndex <= ( nXIndex + 1 ); ++nLocalXIndex )
				{
					if ( ( nLocalXIndex >= 0 ) && 
							 ( nLocalYIndex >= 0 ) &&
							 ( nLocalXIndex < size.x ) &&
							 ( nLocalYIndex < size.y ) )
					{
						if ( GetAlphaFromARGBColor( ( *pSourceImage )[nLocalYIndex][nLocalXIndex] ) < dwMinAlpha )
						{
							( *pDestImage )[nYIndex][nXIndex] = dwEdgeColor;
							return;
						}
					}
				}
			}
		}
	}
};


//функционал помечающий край картинки
struct SEraseEdgeFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;

	SEraseEdgeFunctional( CArray2D<uint32_t> *_pDestImage, const CArray2D<uint32_t> *_pSourceImage )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( ( *pSourceImage )[nYIndex][nXIndex] != 0 )
		{
			for ( int nLocalYIndex = ( nYIndex - 1 ); nLocalYIndex <= ( nYIndex + 1 ); ++nLocalYIndex )
			{
				for ( int nLocalXIndex = ( nXIndex - 1 ); nLocalXIndex <= ( nXIndex + 1 ); ++nLocalXIndex )
				{
					if ( ( *pSourceImage )[nLocalYIndex][nLocalXIndex] == 0 )
					{
						( *pDestImage )[nYIndex][nXIndex] = 0;
						return;
					}
				}
			}
			( *pDestImage )[nYIndex][nXIndex] = ( *pSourceImage )[nYIndex][nXIndex];
		}
	}
};


//функционал помечающий край картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCEraseEdgeFunctional
{
	CArray2D<uint32_t> *pDestImage;
	const CArray2D<uint32_t> *pSourceImage;
	CTPoint<int> size; 

	SWBCEraseEdgeFunctional( CArray2D<uint32_t> *_pDestImage, const CArray2D<uint32_t> *_pSourceImage )
		: pDestImage( _pDestImage ), pSourceImage( _pSourceImage )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		NI_ASSERT( pSourceImage != 0, "Wrong parameter: pSourceImage == 0" );
		NI_ASSERT( ( pDestImage->GetSizeX() == pSourceImage->GetSizeX() ) &&
							 ( pDestImage->GetSizeY() == pSourceImage->GetSizeY() ),
							"Destination and source images must have same sizes." );
		size.x = pSourceImage->GetSizeX();
		size.y = pSourceImage->GetSizeY();
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( ( *pSourceImage )[nYIndex][nXIndex] != 0 )
		{
			for ( int nLocalYIndex = ( nYIndex - 1 ); nLocalYIndex <= ( nYIndex + 1 ); ++nLocalYIndex )
			{
				for ( int nLocalXIndex = ( nXIndex - 1 ); nLocalXIndex <= ( nXIndex + 1 ); ++nLocalXIndex )
				{
					if ( ( nLocalXIndex >= 0 ) && 
							 ( nLocalYIndex >= 0 ) &&
							 ( nLocalXIndex < size.x ) &&
							 ( nLocalYIndex < size.y ) )
					{
						if ( ( *pSourceImage )[nLocalYIndex][nLocalXIndex] == 0 )
						{
							( *pDestImage )[nYIndex][nXIndex] = 0;
							return;
						}
					}
				}
			}
			( *pDestImage )[nYIndex][nXIndex] = ( *pSourceImage )[nYIndex][nXIndex];
		}
	}
};

}


