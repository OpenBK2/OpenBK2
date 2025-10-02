#pragma once

#include "Common_RTS_AI_export.h"


// angle (replacememt for WORD angle), multiplayer sync
struct COMMON_RTS_AI_EXPORT SAIAngle
{
	union
	{
		WORD wAngle;
		int allign;
	};

	SAIAngle() 
		: allign( 0 )		
	{
	}
	SAIAngle( int _wAngle ) 
		: allign( 0 )
	{
		wAngle = _wAngle;
	}
	operator SAIAngle() const { return wAngle; }
	operator int() const { return wAngle; }
	const SAIAngle & operator=( int _nAngle )
	{
		wAngle = _nAngle;
		return *this;
	}
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &allign );
		return 0;
	}
	SAIAngle & operator+=( const SAIAngle &an )
	{
		wAngle += an.wAngle;
		return *this;
	}
};

COMMON_RTS_AI_EXPORT bool operator>( const SAIAngle &_1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator>( int _1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator>( const SAIAngle &_1, int _2 );

COMMON_RTS_AI_EXPORT bool operator<( const SAIAngle &_1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator<( int _1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator<( const SAIAngle &_1, int _2 );


