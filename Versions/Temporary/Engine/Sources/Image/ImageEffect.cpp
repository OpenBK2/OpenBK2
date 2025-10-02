#include "StdAfx.h"
#include "ImageEffect.h"

#include "Image.h"
#include "../Misc/Geom.h"
#include "../Misc/Bresenham.h"

namespace NImage
{

const DWORD BLACK_COLOR					= MakeARGBColor<DWORD>( 0xFF, 0x00, 0x00, 0x00 );
const DWORD WHITE_COLOR					= MakeARGBColor<DWORD>( 0xFF, 0xFF, 0xFF, 0xFF );
const DWORD GRAY_LIGHTER_COLOR	= MakeARGBColor<DWORD>( 0xFF, 0x80, 0x80, 0x80 );
const DWORD GRAY_DARKER_COLOR		= MakeARGBColor<DWORD>( 0xFF, 0x7F, 0x7F, 0x7F );
const DWORD BASE_EMBOSS_COLOR		= MakeARGBColor<DWORD>( 0xFF, 0x7F, 0x7F, 0x7F );


void GetImageParams( DWORD *pdwMinColor, DWORD *pdwMaxColor, DWORD *pdwAverageColor, const CArray2D<DWORD> &rImage )
{
	CTPoint<int> size( rImage.GetSizeX(), rImage.GetSizeY() );
	DWORD dwDivider = size.x * size.y;
	if ( dwDivider == 0 )
	{
		if ( pdwMinColor )
		{
			( *pdwMinColor ) = 0;
		}	
		if ( pdwMaxColor )
		{
			( *pdwMaxColor ) = 0;
		}	
		if ( pdwAverageColor )
		{
			( *pdwAverageColor ) = 0;
		}	
		return;
	}
	//
	BYTE nMinA = 0xFF;
	BYTE nMinR = 0xFF;
	BYTE nMinG = 0xFF;
	BYTE nMinB = 0xFF;
	BYTE nMaxA = 0;
	BYTE nMaxR = 0;
	BYTE nMaxG = 0;
	BYTE nMaxB = 0;
	DWORD dwAverageA = 0;
	DWORD dwAverageR = 0;
	DWORD dwAverageG = 0;
	DWORD dwAverageB = 0;
	//
	DWORD dwColor = 0;
	BYTE nA = 0;
	BYTE nR = 0;
	BYTE nG = 0;
	BYTE nB = 0;
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			dwColor = rImage[nYIndex][nXIndex];
			nA = GetAlphaFromARGBColor( dwColor );
			nR = GetRedFromARGBColor( dwColor );
			nG = GetGreenFromARGBColor( dwColor );
			nB = GetBlueFromARGBColor( dwColor );
			if ( nA < nMinA )
			{
				nMinA = nA;
			}
			if ( nA > nMaxA )
			{
				nMaxA = nA;
			}
			if ( nR < nMinR )
			{
				nMinR = nR;
			}
			if ( nR > nMaxR )
			{
				nMaxR = nR;
			}
			if ( nG < nMinG )
			{
				nMinG = nG;
			}
			if ( nG > nMaxG )
			{
				nMaxG = nG;
			}
			if ( nB < nMinB )
			{
				nMinB = nB;
			}
			if ( nB > nMaxB )
			{
				nMaxB = nB;
			}
			dwAverageA += nA;
			dwAverageR += nR;
			dwAverageG += nG;
			dwAverageB += nB;
		}
	}
	dwAverageA = (DWORD)( ( 1.0f * dwAverageA ) / dwDivider + 0.5f );
	dwAverageR = (DWORD)( ( 1.0f * dwAverageR ) / dwDivider + 0.5f );
	dwAverageG = (DWORD)( ( 1.0f * dwAverageG ) / dwDivider + 0.5f );
	dwAverageB = (DWORD)( ( 1.0f * dwAverageB ) / dwDivider + 0.5f );
	if ( pdwMinColor )
	{
		( *pdwMinColor ) = MakeARGBColor<DWORD>( nMinA, nMinR, nMinG, nMinB );
	}	
	if ( pdwMaxColor )
	{
		( *pdwMaxColor ) = MakeARGBColor<DWORD>( nMaxA, nMaxR, nMaxG, nMaxB );
	}	
	if ( pdwAverageColor )
	{
		( *pdwAverageColor ) = MakeARGBColor<DWORD>( dwAverageA, dwAverageR, dwAverageG, dwAverageB );
	}	
}


void Invert( CArray2D<DWORD> *pImage )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	DWORD dwColor = 0;
	DWORD dwA = 0;
	DWORD dwR = 0;
	DWORD dwG = 0;
	DWORD dwB = 0;
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			dwColor = ( *pImage )[nYIndex][nXIndex];
			dwA = GetAlphaFromARGBColor( dwColor );
			dwR = GetRedFromARGBColor( dwColor );
			dwG = GetGreenFromARGBColor( dwColor );
			dwB = GetBlueFromARGBColor( dwColor );
			( *pImage )[nYIndex][nXIndex] = MakeARGBColor<DWORD>( 0xFF - dwA, 0xFF - dwR, 0xFF - dwG, 0xFF - dwB );
		}
	}
}


inline DWORD GetGammaCorrection( DWORD dwVal, float fBrightness, float fPower, float fA, float fB )
{
  const float fVal = float( dwVal ) / 255.0f;
  const float fGammaValue = pow( fVal, fPower );
  const float fContrastValue = Clamp( fA * fGammaValue + fB, 0.0f, 1.0f );
  const float fResult = Clamp( fContrastValue + fBrightness, 0.0f, 1.0f );
	return Clamp<DWORD>( fResult * 255.0f + 0.5f, 0, 0xFF );
}


void GammaCorrection( CArray2D<DWORD> *pImage, float fBrightness, float fContrast, float fGamma )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	//
	if ( ( fBrightness == 0 ) && ( fContrast == 0 ) && ( fGamma == 0 ) )
	{
		return;
	}
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
  // build ramp from the brightness, contrast and gamma values
  // y = a*x + b
  fBrightness = Clamp( fBrightness, -1.0f, 1.0f ) * 0.5f; // to avoid complete dark and complete white values
  fContrast = Clamp( fContrast, -1.0f, 1.0f ) * 0.5f;
  fGamma = Clamp( fGamma, -1.0f, 1.0f ) * 0.5f;
  // calculate equation params for Y = A*X + B
  // contrast: a*x + b
  // если contrast < 0, то a = 1/a (наклон <45 градусов)
  float fA = 1.0f + 4.0f * fabs( fContrast );
  if ( fContrast < 0 )
	{
		fA = 1.0f / fA;
	}
  float fB = 0.5f * ( 1.0f - fA );
  // gamma: x^power
  float fPower = 1.0f;
  {
    if ( fGamma > 0 )
		{
			fPower = 1.0f / ( 5.0f * fGamma + 1 );
		}
    else if ( fGamma < 0 )
		{
			fPower = 1.0f / ( 0.5f * fGamma + 1 );
		}
  }
  // brightness: x + b
	DWORD dwColor = 0;
	DWORD dwA = 0;
	DWORD dwR = 0;
	DWORD dwG = 0;
	DWORD dwB = 0;
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.y; ++nXIndex )
		{
			dwColor = ( *pImage )[nYIndex][nXIndex];
			dwA = GetAlphaFromARGBColor( dwColor );
			dwR = GetGammaCorrection( GetRedFromARGBColor( dwColor ), fBrightness, fPower, fA, fB );
			dwG = GetGammaCorrection( GetGreenFromARGBColor( dwColor ), fBrightness, fPower, fA, fB );
			dwB = GetGammaCorrection( GetBlueFromARGBColor( dwColor ), fBrightness, fPower, fA, fB );
			( *pImage )[nYIndex][nXIndex] = MakeARGBColor<DWORD>( dwA, dwR, dwG, dwB );
		}
	}
}


void FullColor( CArray2D<DWORD> *pImage, float fRatio )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	//
	DWORD dwMinColor = 0;
	DWORD dwMaxColor = 0;
	GetImageParams( &dwMinColor, &dwMaxColor, 0, *pImage );
	const DWORD dwMinR = GetRedFromARGBColor( dwMinColor );
	const DWORD dwMinG = GetGreenFromARGBColor( dwMinColor );
	const DWORD dwMinB = GetBlueFromARGBColor( dwMinColor );
	const DWORD dwMaxR = GetRedFromARGBColor( dwMaxColor );
	const DWORD dwMaxG = GetGreenFromARGBColor( dwMaxColor );
	const DWORD dwMaxB = GetBlueFromARGBColor( dwMaxColor );
	if ( ( dwMaxR == dwMinR ) && ( dwMaxG == dwMinG ) && ( dwMaxB == dwMinB ) )
	{
		return;
	}
	const DWORD dwRedDivider = ( dwMaxR == dwMinR ) ? 1 : ( dwMaxR - dwMinR );
	const DWORD dwGreenDivider = ( dwMaxG == dwMinG ) ? 1 : ( dwMaxG - dwMinG );
	const DWORD dwBlueDivider = ( dwMaxB == dwMinB ) ? 1 : ( dwMaxB - dwMinB );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	DWORD dwColor = 0;
	DWORD dwA = 0;
	DWORD dwR = 0;
	DWORD dwG = 0;
	DWORD dwB = 0;
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			dwColor = ( *pImage )[nYIndex][nXIndex];
			dwA = GetAlphaFromARGBColor( dwColor );
			dwR = Clamp<DWORD>( 255.f * ( ( ( ( 1.0f * ( GetRedFromARGBColor( dwColor ) - dwMinR ) / dwRedDivider ) - 0.5f ) * fRatio ) + 0.5f ) + 0.5f, 0, 0xFF );
			dwG = Clamp<DWORD>( 255.f * ( ( ( ( 1.0f * ( GetGreenFromARGBColor( dwColor ) - dwMinR ) / dwRedDivider ) - 0.5f ) * fRatio ) + 0.5f ) + 0.5f, 0, 0xFF );
			dwB = Clamp<DWORD>( 255.f * ( ( ( ( 1.0f * ( GetBlueFromARGBColor( dwColor ) - dwMinR ) / dwRedDivider ) - 0.5f ) * fRatio ) + 0.5f ) + 0.5f, 0, 0xFF );
			( *pImage )[nYIndex][nXIndex] = MakeARGBColor<DWORD>( dwA, dwR, dwG, dwB );
		}
	}
}

void ApplyFilter( CArray2D<DWORD> *pImage, const CArray2D<int> &rFilter, DWORD dwMinAlpha )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( ( ( rFilter.GetSizeX() & 0x1 ) != 0 ) &&
						 ( ( rFilter.GetSizeY() & 0x1 ) != 0 ),
						 "Filter must have odd sizes." );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	const CTPoint<int> filterSize( rFilter.GetSizeX(), rFilter.GetSizeY() );
	const CTPoint<int> check( rFilter.GetSizeX() / 2, rFilter.GetSizeY() / 2 );
	//
	CArray2D<DWORD> destImage( size.x, size.y );
	destImage.FillZero();
	//	
	int nDivider = 0;
	for ( int nYIndex = 0; nYIndex < filterSize.y; ++ nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < filterSize.x; ++ nXIndex )
		{
			nDivider += rFilter[nYIndex][nXIndex];
		}
	}
	//
	SApplyFilterFunctional filterFunctional( &destImage, pImage, &rFilter, nDivider );
	SWBCApplyFilterFunctional wbcFilterFunctional( &destImage, pImage, &rFilter, nDivider );
	ApplyFunctional( size, check, &filterFunctional, &wbcFilterFunctional );
	( *pImage ) = destImage;
}


void MarkEdge( CArray2D<DWORD> *pImage, DWORD dwEdgeColor, bool bOutside, DWORD dwMinAlpha )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	const CTPoint<int> check( 1, 1 );
	//
	CArray2D<DWORD> edgeImage( size.x, size.y );
	edgeImage.FillZero();
	//	
	if ( bOutside )
	{
		SMarkOutsideEdgeFunctional markEdgeFunctional( &edgeImage, pImage, dwEdgeColor, dwMinAlpha );
		SWBCMarkOutsideEdgeFunctional wbcMarkEdgeFunctional( &edgeImage, pImage, dwEdgeColor, dwMinAlpha );
		ApplyFunctional( size, check, &markEdgeFunctional, &wbcMarkEdgeFunctional );
	}
	else
	{
		SMarkInsideEdgeFunctional markEdgeFunctional( &edgeImage, pImage, dwEdgeColor, dwMinAlpha );
		SWBCMarkInsideEdgeFunctional wbcMarkEdgeFunctional( &edgeImage, pImage, dwEdgeColor, dwMinAlpha );
		ApplyFunctional( size, check, &markEdgeFunctional, &wbcMarkEdgeFunctional );
	}
	FastAddImageByColor( pImage, edgeImage, dwEdgeColor, true );
}


void EraseEdge( CArray2D<DWORD> *pImage, DWORD dwMinAlpha )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	const CTPoint<int> check( 1, 1 );
	//
	CArray2D<DWORD> edgeImage( size.x, size.y );
	edgeImage.FillZero();
	//	
	SEraseEdgeFunctional eraseEdgeFunctional( &edgeImage, pImage );
	SWBCEraseEdgeFunctional wbcEraseEdgeFunctional( &edgeImage, pImage );
	ApplyFunctional( size, check, &eraseEdgeFunctional, &wbcEraseEdgeFunctional );
	( *pImage ) = edgeImage;
}


void Emboss( CArray2D<DWORD> *pImage, const CTPoint<int> &rShiftPoint, const CArray2D<int> &rFilter, DWORD dwMinAlpha )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( ( ( rFilter.GetSizeX() & 0x1 ) != 0 ) &&
						 ( ( rFilter.GetSizeY() & 0x1 ) != 0 ),
						 "Filter must have odd sizes." );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	//
	CArray2D<DWORD> heightImage( size.x, size.y );
	heightImage.FillZero();
	//	
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			heightImage[nYIndex][nXIndex] = ( GetAlphaFromARGBColor( ( *pImage )[nYIndex][nXIndex] ) >= dwMinAlpha ) ? WHITE_COLOR : BLACK_COLOR;
		}
	}
	//
	ApplyFilter( &heightImage, rFilter, dwMinAlpha );
	//	
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			const DWORD dwImageColor = ( *pImage )[nYIndex][nXIndex];
			const DWORD dwHeightColor = heightImage[nYIndex][nXIndex];
			DWORD dwShiftHeigthColor = 0;
			if ( ( nXIndex < -( rShiftPoint.x ) ) ||
					 ( nYIndex < -( rShiftPoint.y ) ) ||
					 ( nXIndex >= ( size.x - rShiftPoint.x ) ) ||
					 ( nYIndex >= ( size.y - rShiftPoint.y ) ) )
			{
				dwShiftHeigthColor = heightImage[nYIndex][nXIndex];
			}
			else
			{
				dwShiftHeigthColor = heightImage[nYIndex + rShiftPoint.y][nXIndex + rShiftPoint.x];
			}
			//
			const DWORD dwRed = Clamp<DWORD>( GetRedFromARGBColor( dwImageColor ) * ( ( GetRedFromARGBColor( dwHeightColor ) / 2 ) + ( 0xFF - ( GetRedFromARGBColor( dwShiftHeigthColor ) / 2 ) ) ) / 0xFF, 0, 0xFF );
			const DWORD dwGreen = Clamp<DWORD>( GetGreenFromARGBColor( dwImageColor ) * ( ( GetGreenFromARGBColor( dwHeightColor ) / 2 ) + ( 0xFF - ( GetGreenFromARGBColor( dwShiftHeigthColor ) / 2 ) ) ) / 0xFF, 0, 0xFF );
			const DWORD dwBlue = Clamp<DWORD>( GetBlueFromARGBColor( dwImageColor ) * ( ( GetBlueFromARGBColor( dwHeightColor ) / 2 ) + ( 0xFF - ( GetBlueFromARGBColor( dwShiftHeigthColor ) / 2 ) ) ) / 0xFF, 0, 0xFF );
			//			
			UpdateColorARGBColor( &( ( *pImage )[nYIndex][nXIndex] ), dwRed, dwGreen, dwBlue );
		}
	}
}


void Noise( CArray2D<DWORD> *pImage, const CArray2D<DWORD> &rNoise, bool bEqualize, DWORD dwMinAlpha )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( ( rNoise.GetSizeX() > 0 ) &&
						 ( rNoise.GetSizeY() > 0 ),
						 "Noise must have non zero sizes." );
	//
	const CTPoint<int> size( pImage->GetSizeX(), pImage->GetSizeY() );
	const CTPoint<int> noiseSize( rNoise.GetSizeX(), rNoise.GetSizeY() );
	//
	DWORD dwAverageColor = MakeARGBColor<DWORD>( 0xFF, 1, 1, 1 );
	if ( bEqualize )
	{
		GetImageParams( 0, 0, &dwAverageColor, rNoise );
		if ( GetRedFromARGBColor( dwAverageColor ) == 0 )
		{
			UpdateRedARGBColor<DWORD>( &dwAverageColor, 1 );
		}
		if ( GetGreenFromARGBColor( dwAverageColor ) == 0 )
		{
			UpdateGreenARGBColor<DWORD>( &dwAverageColor, 1 );
		}
		if ( GetBlueFromARGBColor( dwAverageColor ) == 0 )
		{
			UpdateBlueARGBColor<DWORD>( &dwAverageColor, 1 );
		}
	}
	const DWORD dwRedDivider = GetRedFromARGBColor( dwAverageColor );
	const DWORD dwGreenDivider = GetGreenFromARGBColor( dwAverageColor );
	const DWORD dwBlueDivider = GetBlueFromARGBColor( dwAverageColor );
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			if ( GetAlphaFromARGBColor( ( *pImage )[nYIndex][nXIndex] ) >= dwMinAlpha )
			{
				const DWORD dwColor = ( *pImage )[nYIndex][nXIndex];
				const DWORD dwNoiseColor = rNoise[nYIndex % noiseSize.y][nXIndex % noiseSize.x];
				//
				const DWORD dwRed = Clamp<DWORD>( GetRedFromARGBColor( dwNoiseColor ) * GetRedFromARGBColor( dwColor ) / dwRedDivider + 0.5f, 0, 0xFF );
				const DWORD dwGreen = Clamp<DWORD>( GetGreenFromARGBColor( dwNoiseColor ) * GetGreenFromARGBColor( dwColor ) / dwGreenDivider + 0.5f, 0, 0xFF );
				const DWORD dwBlue = Clamp<DWORD>( GetBlueFromARGBColor( dwNoiseColor ) * GetBlueFromARGBColor( dwColor ) / dwBlueDivider + 0.5f, 0, 0xFF );
				//
				UpdateColorARGBColor( &( ( *pImage )[nYIndex][nXIndex] ), dwRed, dwGreen, dwBlue );
			}
		}
	}
}


void FastAddImageByAlpha( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, DWORD dwMinAlpha )
{
	NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	NI_ASSERT( ( pDestImage->GetSizeX() == rSourceImage.GetSizeX() ) &&
						 ( pDestImage->GetSizeY() == rSourceImage.GetSizeY() ),
						 "Destination and source images must have same sizes." );
	//
	const CTPoint<int> size( rSourceImage.GetSizeX(), rSourceImage.GetSizeY() );
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			if ( GetAlphaFromARGBColor( rSourceImage[nYIndex][nXIndex] ) >= dwMinAlpha )
			{
				( *pDestImage )[nYIndex][nXIndex] = rSourceImage[nYIndex][nXIndex];
			}
		}
	}
}


void FastAddImageByColor( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, DWORD dwColor, bool bInclude )
{
	NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	NI_ASSERT( ( pDestImage->GetSizeX() == rSourceImage.GetSizeX() ) &&
						 ( pDestImage->GetSizeY() == rSourceImage.GetSizeY() ),
						 "Destination and source images must have same sizes." );
	//
	const CTPoint<int> size( rSourceImage.GetSizeX(), rSourceImage.GetSizeY() );
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			if ( rSourceImage[nYIndex][nXIndex] == dwColor )
			{
				if ( bInclude )
				{
					( *pDestImage )[nYIndex][nXIndex] = rSourceImage[nYIndex][nXIndex];
				}
			}
			else
			{
				if ( !bInclude )
				{
					( *pDestImage )[nYIndex][nXIndex] = rSourceImage[nYIndex][nXIndex];
				}
			}
		}
	}
}


void GetShadow( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, const CTPoint<int> &rShiftPoint, DWORD dwShadowColor, DWORD dwNonShadowColor, DWORD dwMinAlpha )
{
	NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	NI_ASSERT( ( pDestImage->GetSizeX() == rSourceImage.GetSizeX() ) &&
						 ( pDestImage->GetSizeY() == rSourceImage.GetSizeY() ),
						 "Destination and source images must have same sizes." );
	//
	const CTPoint<int> size( rSourceImage.GetSizeX(), rSourceImage.GetSizeY() );
	//
	if ( dwNonShadowColor == 0 )
	{
		pDestImage->FillZero();
	}
	else
	{
		pDestImage->FillEvery( dwNonShadowColor );
	}
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			if ( GetAlphaFromARGBColor( rSourceImage[nYIndex][nXIndex] ) >= dwMinAlpha )
			{
				CTPoint<int> shiftPoint( nXIndex + rShiftPoint.x, nYIndex + rShiftPoint.y );
				if ( ( shiftPoint.x >= 0 ) && 
						 ( shiftPoint.y >= 0 ) &&
						 ( shiftPoint.x < size.x ) &&
						 ( shiftPoint.y < size.y ) )
				{
					if ( GetAlphaFromARGBColor( rSourceImage[shiftPoint.y][shiftPoint.x] ) < dwMinAlpha )
					{
						( *pDestImage )[shiftPoint.y][shiftPoint.x] = dwShadowColor;
					}
				}
			}
		}
	}
}


void GetAlphaEmboss( CArray2D<DWORD> *pDestImage, const CArray2D<DWORD> &rSourceImage, const CTPoint<int> &rShiftPoint, int nFilterSize, DWORD dwMinAlpha )
{
	NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	NI_ASSERT( ( pDestImage->GetSizeX() == rSourceImage.GetSizeX() ) &&
						 ( pDestImage->GetSizeY() == rSourceImage.GetSizeY() ),
						 "Destination and source images must have same sizes." );
	//
	const CTPoint<int> size( rSourceImage.GetSizeX(), rSourceImage.GetSizeY() );
	//
	pDestImage->FillZero();
	//
	CArray2D<int> embossFilter( nFilterSize, nFilterSize );
	embossFilter.FillEvery( 0x01 );
	//
	CArray2D<DWORD> heightImage( size.x, size.y );
	heightImage.FillZero();
	//
	//сдвиги относительно начальной точки
	CTPoint<int> frontShift( ( rShiftPoint.x / 2 ), ( rShiftPoint.y / 2 ) );
	CTPoint<int> backShift( frontShift * ( -1 ) );
	if ( ( rShiftPoint.x & 0x01 ) > 0 )
	{
		frontShift.x += ( rShiftPoint.x > 0 ) ? 1 : ( -1 );
	}
	if ( ( rShiftPoint.y & 0x01 ) > 0 )
	{
		frontShift.y += ( rShiftPoint.y > 0 ) ? 1 : ( -1 );
	}
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			heightImage[nYIndex][nXIndex] = ( GetAlphaFromARGBColor( rSourceImage[nYIndex][nXIndex] ) < dwMinAlpha ) ? WHITE_COLOR : BLACK_COLOR;
		}
	}
	//
	ApplyFilter( &heightImage, embossFilter, dwMinAlpha );
	//
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			DWORD dwFrontShiftHeigthColor = 0;
			{
				const int nXPos = Clamp<int>( nXIndex + frontShift.x, 0, size.x - 1 );
				const int nYPos = Clamp<int>( nYIndex + frontShift.y, 0, size.y - 1 );
				dwFrontShiftHeigthColor = heightImage[nYPos][nXPos];
			}
			DWORD dwBackShiftHeigthColor = 0;
			{
				const int nXPos = Clamp<int>( nXIndex + backShift.x, 0, size.x - 1 );
				const int nYPos = Clamp<int>( nYIndex + backShift.y, 0, size.y - 1 );
				dwBackShiftHeigthColor = heightImage[nYPos][nXPos];
			}
			//			
			const DWORD dwRed = Clamp<DWORD>( ( ( GetRedFromARGBColor( dwFrontShiftHeigthColor ) / 2 ) + ( 0xFF - ( GetRedFromARGBColor( dwBackShiftHeigthColor ) / 2 ) ) ) / 2, 0, 0xFF );
			const DWORD dwGreen = Clamp<DWORD>( ( ( GetGreenFromARGBColor( dwFrontShiftHeigthColor ) / 2 ) + ( 0xFF - ( GetGreenFromARGBColor( dwBackShiftHeigthColor ) / 2 ) ) ) / 2, 0, 0xFF );
			const DWORD dwBlue = Clamp<DWORD>( ( ( GetBlueFromARGBColor( dwFrontShiftHeigthColor ) / 2 ) + ( 0xFF - ( GetBlueFromARGBColor( dwBackShiftHeigthColor ) / 2 ) ) ) / 2, 0, 0xFF );
			//			
			( *pDestImage )[nYIndex][nXIndex] = MakeARGBColor<DWORD>( 0xFF, dwRed, dwGreen, dwBlue );
		}
	}
}

void DrawLine( CArray2D<DWORD> *pImage, const SVector &vStart, const SVector &vEnd, const SColor &color )
{
	const SVector v1( Clamp( vStart.x, 0, pImage->GetSizeX() - 1 ), Clamp( vStart.y, 0, pImage->GetSizeY() - 1 ) );
	const SVector v2( Clamp( vEnd.x, 0, pImage->GetSizeX() - 1 ), Clamp( vEnd.y, 0, pImage->GetSizeY() - 1 ) );

	CBres bres;
	bres.InitPoint( v1, v2 );
	while ( bres.GetDirection() != v2 )
	{
		(*pImage)[bres.GetDirection().y][bres.GetDirection().x] = (DWORD)color;
		bres.MakePointStep();
	}
}
}


