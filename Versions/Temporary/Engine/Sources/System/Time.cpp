#include "stdafx.h"
#include "Time.h"

#include "System_export.h"

// CTimeCounter

CTimeCounter::CTimeCounter() 
{
	prevTime = 0; 
	pTime = new CCTime( 0 );
}

void CTimeCounter::ResetTiming() 
{
	prevTime = 0; 
}

void CTimeCounter::SetCurrent( STime t )
{
	pTime->Set( t );
}

int CTimeCounter::GetDeltaT( float fMult, STime currentTime )
{
	if ( fMult != 0 )
	{
		if ( prevTime == 0 )
			prevTime = currentTime;
		STime deltaT = currentTime - prevTime;
		return (std::min)( (int)150, Float2Int( deltaT * fMult ) ); // min 7 fps
	}
	return 0;
}

void CTimeCounter::Advance( float fMult, STime currentTime )
{
	int nDeltaT = GetDeltaT( fMult, currentTime );
	if ( nDeltaT != 0 )
		pTime->Set( pTime->GetValue() + nDeltaT );
	prevTime = currentTime;
}
REGISTER_SAVELOAD_CLASS( SYSTEM, 0x0251100c, CCTime )


