#include "stdafx.h"

#include "Tools_Geometry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const float MINIMAL_POINT_DISTANCE = 2.0f;

/**

WORD GetAIDirectionByVector( float x, float y, WORD wDefaultDirection )
{
	if ( ( fabs( x ) < 1 ) && ( fabs( y ) < 1 ) )
	{
		return wDefaultDirection;
	}
	//
	float add = 49152.0f;
	//
	if ( ( x <= 0 ) && ( y > 0 ) )
	{
		add = 0.0f;
		swap( x, y );
		y = -( y );
	}
	else if ( ( y <= 0 ) && ( x < 0 ) )
	{
		add = 16384.0f;
		x = -( x );
		y = -( y );
	}
	else if ( ( x >= 0 ) && ( y < 0 ) )
	{
		add = 32768.0f;
		swap( x, y );
		x = -( x );
	}
	//
	if ( ( x + y ) != 0 )
	{
		return (WORD)( ( 16384.0f * y / ( x + y ) ) + add );
	}
	else
	{
		return wDefaultDirection;
	}
}
/**/
/**

void GetVectorByAIDirection( CVec2 *pVec2, WORD wDirection, float fRadius )
{
	if ( pVec2 )
	{
		const float fDir = float( wDirection % 16384 ) / 16384.0f;
		CVec2 result( 1 - fDir, fDir );

		if ( wDirection < 16384 )
		{
			result.y = -result.y;
			swap( result.x, result.y );
		}
		else if ( wDirection < 32768 )
		{
			result.x = -result.x;
			result.y = -result.y;
		}
		else if ( wDirection < 49152 )
		{
			result.x = -result.x;
			swap( result.x, result.y );
		}

		Normalize( &result );
		result *= fRadius;
		( *pVec2 ) = result;
	}
}
/**/

float GetDistanceTo3DLine( const CVec3 &rvPoint, const CVec3 &rvOrigin, const CVec3 &rvDirection )
{
	const CVec3 vRadius = rvPoint - rvOrigin;
	if ( fabs2( vRadius ) < FP_EPSILON2 )
	{
		return 0.0f;
	}
	if ( fabs2( rvDirection ) < FP_EPSILON2 )
	{
		return 0.0f;
	}
	const CVec3 vResult = vRadius ^ rvDirection;
	return fabs( vResult ) / fabs( rvDirection );
}

// basement storage  

