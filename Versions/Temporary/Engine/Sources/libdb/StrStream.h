#pragma once

#include <fmt/format.h>

namespace NCodeGen
{

class CStrStream
{
	std::string *pszStr;
public:
	typedef CStrStream& (*OpFunc)( CStrStream& );
	CStrStream( std::string *_pszStr ) : pszStr( _pszStr ) { }

	CStrStream& operator<<( const char *psz ) { *pszStr += psz; return *this; }
	CStrStream& operator<<( int n ) { *pszStr += std::to_string(  n ); return *this; }
	CStrStream& operator<<( double f ) { *pszStr += fmt::format( "{:g}", f ); return *this; }
	CStrStream& operator<<( const std::string &s ) { *pszStr += s; return *this; }
	CStrStream& operator<<( OpFunc func ) { return func(*this); }
};

inline CStrStream& endl( CStrStream& sStream ) { sStream << "\r\n"; return sStream; }
inline CStrStream& separator( CStrStream& sStream )
{ 
	sStream << "//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << endl;
	return sStream;
}

inline CStrStream& qcomma( CStrStream& sStream ) { sStream << "\""; return sStream; }
inline CStrStream& tab( CStrStream& sStream ) { sStream << "\t"; return sStream; }

}


