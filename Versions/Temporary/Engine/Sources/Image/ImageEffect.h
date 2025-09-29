#pragma once

#include "ImageColor.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NImage
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXTERNVAR const DWORD BLACK_COLOR;
EXTERNVAR const DWORD WHITE_COLOR;
EXTERNVAR const DWORD GRAY_LIGHTER_COLOR;
EXTERNVAR const DWORD GRAY_DARKER_COLOR;
EXTERNVAR const DWORD BASE_EMBOSS_COLOR;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetImageParams( DWORD *pdwMinColor, DWORD *pdwMaxColor, DWORD *pdwAverageColor, const CArray2D<DWORD> &rImage );
void Invert( CArray2D<DWORD> *pImage );
void FullColor( CArray2D<DWORD> *pImage, float fRatio );
void GammaCorrection( CArray2D<DWORD> *pImage, float fBrightness, float fContrast, float fGamma ); // [-1...1]
//
void ApplyFilter( CArray2D<DWORD> *pImage, const CArray2D<int> &rFilter, DWORD dwMinAlpha );
void MarkEdge( CArray2D<DWORD> *pImage, DWORD dwEdgeColor, bool bOutside, DWORD dwMinAlpha );
void EraseEdge( CArray2D<DWORD> *pImage, DWORD dwMinAlpha );
//
void Emboss( CArray2D<DWORD> *pImage, const CTPoint<int> &rShiftPoint, const CArray2D<int> &rFilter, DWORD dwMinAlpha );
void Noise( CArray2D<DWORD> *pImage, const CArray2D<DWORD> &rNoise, bool bEqualize, DWORD dwMinAlpha );
//
void FastAddImageByAlpha( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, DWORD dwMinAlpha );
void FastAddImageByColor( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, DWORD dwColor, bool bInclude );
//
void GetShadow( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, const CTPoint<int> &rShiftPoint, DWORD dwShadowColor, DWORD dwNonShadowColor, DWORD dwMinAlpha );
void GetAlphaEmboss( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, const CTPoint<int> &rShiftPoint, int nFilterSize, DWORD dwMinAlpha );
//
void DrawLine( CArray2D<DWORD> *pImage, const struct SVector &vStart, const struct SVector &vEnd, const struct SColor &color );
//
//все одного размера!
template<class TArray>
void FastComposeImagesByAlpha( CArray2D<DWORD> *pDestImage, const TArray &rSourceImageList, DWORD dwMinAlpha )
{
	NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	//
	const CTPoint<int> size( pDestImage->GetSizeX(), pDestImage->GetSizeY() );
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			for ( TArray::const_iterator itSourceImage = rSourceImageList.begin(); itSourceImage != rSourceImageList.begin(); ++itSourceImage )
			{
				if ( GetAlphaFromARGBColor( ( *( *itSourceImage ) )[nYIndex][nXIndex] ) >= dwMinAlpha )
				{
					( *pDestImage )[nYIndex][nXIndex] = ( *( *itSourceImage ) )[nYIndex][nXIndex];
					break;
				}
			}
		}
	}
}
//все одного размера!
template<class TArray>
void FastComposeImagesByColor( CArray2D<DWORD> *pDestImage, const TArray &rSourceImageList, DWORD dwColor, bool bInclude )
{
	NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	//
	const CTPoint<int> size( pDestImage->GetSizeX(), pDestImage->GetSizeY() );
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			for ( TArray::const_iterator itSourceImage = rSourceImageList.begin(); itSourceImage != rSourceImageList.begin(); ++itSourceImage )
			{
				if ( ( *( *itSourceImage ) )[nYIndex][nXIndex] == dwColor )
				{
					if ( bInclude )
					{
						( *pDestImage )[nYIndex][nXIndex] = ( *( *itSourceImage ) )[nYIndex][nXIndex];
						break;
					}
				}
				else
				{
					if ( !bInclude )
					{
						( *pDestImage )[nYIndex][nXIndex] = ( *( *itSourceImage ) )[nYIndex][nXIndex];
						break;
					}
				}
			}
		}
	}
}

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал применяющий данный паттерн к указанной точке картинки
struct SApplyFilterFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	const CArray2D<int> *pFilter;
	int nDivider;
	CTPoint<int> filterHalfSize; 

	SApplyFilterFunctional( CArray2D<DWORD> *_pDestImage,
													const CArray2D<DWORD> *_pSourceImage,
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
		DWORD dwRed = 0;
		DWORD dwGreen = 0;
		DWORD dwBlue = 0;
		for ( int nFilterYIndex = 0, nLocalYIndex = localIndices.miny;
					nLocalYIndex <= localIndices.maxy;
					++nFilterYIndex, ++nLocalYIndex )
		{
			for ( int nFilterXIndex = 0, nLocalXIndex = localIndices.minx; 
						nLocalXIndex <= localIndices.maxx;
						++nFilterXIndex, ++nLocalXIndex )
			{
				int nValue = ( *pFilter )[nFilterYIndex][nFilterXIndex];
				const DWORD dwColor = ( *pSourceImage )[nLocalYIndex][nLocalXIndex];
				dwRed += GetRedFromARGBColor( dwColor ) * nValue;
				dwGreen += GetGreenFromARGBColor( dwColor ) * nValue;
				dwBlue += GetBlueFromARGBColor( dwColor ) * nValue;
			}
		}
		dwRed = Clamp<int>( dwRed / nDivider, 0, 0xFF );
		dwGreen = Clamp<int>( dwGreen / nDivider, 0, 0xFF );
		dwBlue = Clamp<int>( dwBlue / nDivider, 0, 0xFF );
		//
		DWORD dwDestColor = MakeARGBColor<DWORD>( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ),
																							dwRed, dwGreen, dwBlue );
		( *pDestImage )[nYIndex][nXIndex] = dwDestColor;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал применяющий данный паттерн к указанной точке картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCApplyFilterFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	const CArray2D<int> *pFilter;
	int nDivider;
	CTPoint<int> size; 
	CTPoint<int> filterHalfSize; 

	SWBCApplyFilterFunctional( CArray2D<DWORD> *_pDestImage,
														 const CArray2D<DWORD> *_pSourceImage,
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
		DWORD dwRed = 0;
		DWORD dwGreen = 0;
		DWORD dwBlue = 0;
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
				const DWORD dwColor = ( *pSourceImage )[nYPos][nXPos];
				dwRed += GetRedFromARGBColor( dwColor ) * nValue;
				dwGreen += GetGreenFromARGBColor( dwColor ) * nValue;
				dwBlue += GetBlueFromARGBColor( dwColor ) * nValue;
			}
		}
		dwRed = Clamp<int>( dwRed / nDivider, 0, 0xFF );
		dwGreen = Clamp<int>( dwGreen / nDivider, 0, 0xFF );
		dwBlue = Clamp<int>( dwBlue / nDivider, 0, 0xFF );
		//
		DWORD dwDestColor = MakeARGBColor<DWORD>( GetAlphaFromARGBColor( ( *pSourceImage )[nYIndex][nXIndex] ),
																							dwRed, dwGreen, dwBlue );
		( *pDestImage )[nYIndex][nXIndex] = dwDestColor;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал помечающий край картинки
struct SMarkOutsideEdgeFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	DWORD dwEdgeColor;
	DWORD dwMinAlpha;

	SMarkOutsideEdgeFunctional( CArray2D<DWORD> *_pDestImage,
															const CArray2D<DWORD> *_pSourceImage,
															DWORD _dwEdgeColor,
															DWORD _dwMinAlpha ) 
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал помечающий край картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCMarkOutsideEdgeFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	DWORD dwEdgeColor;
	DWORD dwMinAlpha;
	CTPoint<int> size; 

	SWBCMarkOutsideEdgeFunctional( CArray2D<DWORD> *_pDestImage,
																 const CArray2D<DWORD> *_pSourceImage,
																 DWORD _dwEdgeColor,
																 DWORD _dwMinAlpha ) 
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал помечающий край картинки
struct SMarkInsideEdgeFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	DWORD dwEdgeColor;
	DWORD dwMinAlpha;

	SMarkInsideEdgeFunctional( CArray2D<DWORD> *_pDestImage,
														 const CArray2D<DWORD> *_pSourceImage,
														 DWORD _dwEdgeColor,
														 DWORD _dwMinAlpha ) 
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал помечающий край картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCMarkInsideEdgeFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	DWORD dwEdgeColor;
	DWORD dwMinAlpha;
	CTPoint<int> size; 

	SWBCMarkInsideEdgeFunctional( CArray2D<DWORD> *_pDestImage,
																const CArray2D<DWORD> *_pSourceImage,
																DWORD _dwEdgeColor,
																DWORD _dwMinAlpha ) 
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал помечающий край картинки
struct SEraseEdgeFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;

	SEraseEdgeFunctional( CArray2D<DWORD> *_pDestImage, const CArray2D<DWORD> *_pSourceImage )
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал помечающий край картинки, с проверкой диапазонов
// WBC = with bounds check
struct SWBCEraseEdgeFunctional
{
	CArray2D<DWORD> *pDestImage;
	const CArray2D<DWORD> *pSourceImage;
	CTPoint<int> size; 

	SWBCEraseEdgeFunctional( CArray2D<DWORD> *_pDestImage, const CArray2D<DWORD> *_pSourceImage )
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
