
#pragma once
#include "System/RandomGen.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// равномерно в круге с центром в 0 и радиусом fR
inline void RandUniformlyInCircle( const float fR, CVec2 *pvRand )
{
	*pvRand = VNULL2;

	do
	{
		pvRand->x = fR * NRandom::Random( -1.0f, 1.0f ); RecordRandomCall();
		pvRand->y = fR * NRandom::Random( -1.0f, 1.0f ); RecordRandomCall();
	} while ( sqr( pvRand->x ) + sqr( pvRand->y ) > sqr( fR ) );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// равномерно по углу и по радиусу в круге с центром в 0 и радиусом fR
inline void RandQuadrInCircle(	const float fR, 
																CVec2 *pvRand, 
																const float fRatio=0.0f,
																CVec2 vTrajLine = VNULL2)
{
	const int temp = NRandom::Random( 65536 );
	const CVec2 dir( GetVectorByDirection( temp ) );
	const float fRandR = fR * NRandom::Random( 0.0f, 1.0f );

	
	if ( fRatio == 0.0f )
	{
		pvRand->x = dir.x * fRandR;
		pvRand->y = dir.y * fRandR;
	}
	else // для вытянутых траекторий
	{
		Normalize( &vTrajLine );
		*pvRand = vTrajLine* fRandR * dir.x * fRatio +
							CVec2( -vTrajLine.y, vTrajLine.x ) * fRandR * dir.y ;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

