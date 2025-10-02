#include "stdafx.h"

#include "probability.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float CalculateProbability( float x0, float y0, float x1, float y1 )
{
	if ( x0 == x1 && y0 == y1 )
	{
		if ( x0 >= y0 )
			return 1.0f;
		else
			return 0.0f;
	}
	else if ( x0 == x1 )
	{
		if ( x0 <= y0 )
			return 0.0f;
		else if ( x0 <= y1 )
			return ( x0 - y0 ) / ( y1 - y0 );
		else
			return 1.0f;
	}
	else if ( y0 == y1 )
	{
		if ( x1 <= y0 )
			return 0.0f;
		else if ( x0 <= y0 )
			return ( x1 - y0 ) / ( x1 - x0 );
		else
			return 1.0f;
	}
	else
	{
		bool bSwaped = false;
		if ( y1 - y0 > x1 - x0 )
		{
			swap( x0, y0 );
			swap( x1, y1 );
			bSwaped = true;
		}

		float fRes;

		if ( x1 <= y0 )
			fRes = 0.0f;
		else if ( x1 <= y1 )
			fRes = sqr( x1 - y0 ) / ( 2 * ( y1 - y0 ) * ( x1 - x0 ) );
		else if ( x0 <= y0 )
			fRes = ( x1 - y0 + x1 - y1 ) / ( 2 * ( x1 - x0 ) );
//			( x1 - y0 + x1 - y1 ) * ( y1 - y0 ) / ( 2 * ( y1 - y0 ) * ( x1 - x0 ) );
		else if ( x0 <= y1 )
			fRes = 1 - sqr( y1 - x0 ) / ( 2 * ( y1 - y0 ) * ( x1 - x0 ) );
		else
			fRes = 1.0f;

		if ( bSwaped )
			fRes = 1.0f - fRes;

		return fRes;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

