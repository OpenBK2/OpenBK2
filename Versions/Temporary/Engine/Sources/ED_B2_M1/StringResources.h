#pragma once

#define RCSTR(s)	(s)

inline CString RCStr( const unsigned nStringID )
{
	extern HINSTANCE theEDB2M1DllInstance;

	CString s;
	s.LoadString( theEDB2M1DllInstance, nStringID );

	if ( s.IsEmpty() )
	{
		string szMsg = StrFmt( "WARNING: can't find string resource (ID=%d)", nStringID );
		NI_ASSERT( 0, szMsg.c_str() );
	}

	return s;
}



