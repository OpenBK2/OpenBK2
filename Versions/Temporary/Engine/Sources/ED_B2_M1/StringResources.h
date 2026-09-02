#pragma once

#include <fmt/format.h>

#define RCSTR(s)	(s)

inline CString RCStr( const unsigned nStringID )
{
	extern HINSTANCE theEDB2M1DllInstance;

	CString s;
	s.LoadString( theEDB2M1DllInstance, nStringID );

	if ( s.IsEmpty() )
	{
		std::string szMsg = fmt::format( "WARNING: can't find string resource (ID={})", nStringID );
		NI_ASSERT( 0, szMsg.c_str() );
	}

	return s;
}



