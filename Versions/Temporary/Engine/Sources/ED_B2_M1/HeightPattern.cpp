#include "stdafx.h"

#include "HeightPattern.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool SHeightPattern::CreateByGradient( float fValue, int nGridLines, const SGradient &rGradient )
{
	NI_ASSERT( nGridLines > 0, StrFmt( "Invalid GridLines Number: %d\n", nGridLines ) );
	if ( nGridLines < 0 )
	{
		return false;	
	}
	bOdd = ( ( nGridLines & 0x1 ) != 0 );
	//
	heights.SetSizes( nGridLines, nGridLines );
	heights.FillZero();
	//
	SCreateHeightPatternByGradientFunctional functional( this, &rGradient, fValue, true );
	const bool bResult = ApplyInRadius( CTRect<int>( 0, 0, nGridLines, nGridLines ), functional );
	//Trace2DFloatArray( heights, StrFmt( "SHeightPattern::CreateByGradient(): %s", bResult ? "true" : "false" ) ); 
	return bResult;
}


bool SHeightPattern::CreateByValue( float fValue, int nGridLines, bool bEllipse )
{
	NI_ASSERT( nGridLines > 0, StrFmt( "Invalid GridLines Number: %d\n", nGridLines ) );
	if ( nGridLines < 0 )
	{
		return false;	
	}
	bOdd = ( ( nGridLines & 0x1 ) != 0 );
	//
	heights.SetSizes( nGridLines, nGridLines );
	//
	if ( bEllipse )
	{
		heights.FillZero();
		SCreateHeightPatternByValueFunctional functional( this, fValue );
		return ApplyInRadius( CTRect<int>( 0, 0, nGridLines, nGridLines ), functional );
	}
	else
	{
		heights.FillEvery( fValue );
		return true;
	}
}

// basement storage  


