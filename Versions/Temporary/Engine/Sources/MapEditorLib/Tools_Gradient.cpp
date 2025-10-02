#include "stdafx.h"

#include "..\misc\2darray.h"
#include "Tools_Gradient.h"
#include "..\Image\ImageColor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void SGradient::UpdateHeightRanges()
{
	CHeightList::iterator itHeight = heightList.begin();
	//
	float fMin = *itHeight;
	float fMax = *itHeight;
	//
	for ( ++itHeight; itHeight != heightList.end(); ++itHeight )
	{
		if ( *itHeight < fMin ) 
		{
			fMin = *itHeight;
		}
		else if ( *itHeight > fMax )
		{
			fMax = *itHeight;
		}
	}
	//
	float fActualSize = fMax - fMin;
	if ( fActualSize < FP_EPSILON )
	{
		//int nPointIndex = 0;
		//DebugTrace( "SGradient::UpdateHeightRanges() Range:[%g,%g]", heightRange.min, heightRange.max );
		//
		for ( itHeight = heightList.begin(); itHeight != heightList.end(); ++itHeight )
		{
			*itHeight = heightRange.min;
			//
			//DebugTrace( "SGradient::UpdateHeightRanges() [%d] = %g", nPointIndex, *itHeight );
			//++nPointIndex;
		}
	}
	else
	{
		const float fRequiredSize = heightRange.max - heightRange.min;
		//
		//int nPointIndex = 0;
		//DebugTrace( "SGradient::UpdateHeightRanges() Range:[%g,%g]", heightRange.min, heightRange.max );
		//
		for ( itHeight = heightList.begin(); itHeight != heightList.end(); ++itHeight )
		{
			*itHeight -= fMin;
			*itHeight *= fRequiredSize / fActualSize;
			*itHeight += heightRange.min;
			//
			//DebugTrace( "SGradient::UpdateHeightRanges() [%d] = %g", nPointIndex, *itHeight );
			//++nPointIndex;
		}
	}
}


bool SGradient::CreateFromImage( const CArray2D<DWORD> &rImage, const CTPoint<float> &rRange, const CTPoint<float> &rHeightRange )
{
	NI_ASSERT( ( rRange.max - rRange.min ) > FP_EPSILON, StrFmt( "Invalid range: %g %g\n", rRange.min, rRange.max ) );
	if ( ( rRange.max - rRange.min ) <= FP_EPSILON )
	{
		return false;
	}
	//
	heightList.clear();
	range = rRange;
	heightRange = rHeightRange;
	//
	const CTPoint<int> size( rImage.GetSizeX(), rImage.GetSizeY() );
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		bool isCompleted = false;
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			const DWORD dwColor = rImage[nYIndex][nXIndex];
			if ( ( GetRedFromARGBColor( dwColor )		< 0x80 ) && 
					 ( GetGreenFromARGBColor( dwColor )	< 0x80 ) &&
					 ( GetBlueFromARGBColor( dwColor )	< 0x80 ) )
			{
				heightList.push_back( heightRange.min + ( ( size.y - nYIndex - 1 ) * ( heightRange.max - heightRange.min ) / ( size.y - 1 ) ) );
				isCompleted = true;
				break;
			}
		}
		if ( !isCompleted )
		{
			heightList.push_back( heightRange.min );
		}
  }
	//
	UpdateHeightRanges();
	return true;
}


bool SGradient::Get( float *pfValue, float fPosition, bool isSquareInterpolated ) const
{
	NI_ASSERT( pfValue != 0, StrFmt( "Invalid parameter: pfValue == 0" ) );
	NI_ASSERT( ( ( heightList.size() > 2 ) && isSquareInterpolated ) ||
						 ( ( heightList.size() > 1 ) && !isSquareInterpolated ),
						 StrFmt( "HeightList size not enough: %d", heightList.size() ) );
	NI_ASSERT( range.min != range.max, StrFmt( "Invalid range: (%g, %g)", range.min, range.max ) );
	NI_ASSERT( ( range.min <= fPosition ) && ( range.max >= fPosition ),
						 StrFmt( "Invalid parameter: %g, must be in range: (%g, %g)", fPosition, range.min, range.max ) );
	if ( pfValue == 0 )
	{
		return false;
	}
	if ( ( ( heightList.size() <= 2 ) || !isSquareInterpolated ) &&
			 ( ( heightList.size() <= 1 ) || isSquareInterpolated ) )
	{
		return false;
	}
	if ( range.min == range.max )
	{
		return false;
	}
	if ( ( range.min > fPosition ) || ( range.max < fPosition ) )
	{
		return false;
	}
	//
	float fGranularity = ( range.max - range.min ) / ( heightList.size() - 1 );
	int nIndex = int( ( fPosition - range.min ) / fGranularity );

	if ( fabs( fPosition - ( fGranularity * nIndex ) ) <= FP_EPSILON )
	{
		( *pfValue ) = heightList[nIndex];
	}
	else if ( isSquareInterpolated )
	{
		if ( nIndex != nPreviousIndex )
		{
			CTPoint<float> p0;
			CTPoint<float> p1;
			CTPoint<float> p2;
			if ( nIndex == 0 )
			{
				p0.x = range.min + 0.0f * fGranularity;;
				p0.y = heightList[0];
				p1.x = range.min + 1.0f * fGranularity;
				p1.y = heightList[1];
				p2.x = range.min + 2.0f * fGranularity;
				p2.y = heightList[2];
			}
			else 
			{
				p0.x = range.min + ( nIndex - 1 ) * fGranularity;
				p0.y = heightList[nIndex - 1];
				p1.x = range.min + ( nIndex + 0 ) * fGranularity;
				p1.y = heightList[nIndex];
				p2.x = range.min + ( nIndex + 1 ) * fGranularity;
				p2.y = heightList[nIndex + 1];
			}
			a = ( ( p1.y - p0.y ) / ( p1.x - p0.x ) - ( p2.y - p0.y ) / ( p2.x - p0.x ) ) / ( p1.x - p2.x );
			b = ( p2.y - p1.y ) / ( p2.x - p1.x ) - ( p2.x + p1.x ) * a;
			c = p1.y - a * p1.x * p1.x - b * p1.x;
			nPreviousIndex = nIndex;
		}
		( *pfValue ) = a * fPosition * fPosition + b * fPosition + c;
	}
	else
	{
		( *pfValue ) = heightList[nIndex] + ( heightList[nIndex + 1] - heightList[nIndex] ) * ( fPosition - range.min - ( fGranularity * nIndex ) ) / fGranularity;
	}
	return true;
}

// basement storage  


