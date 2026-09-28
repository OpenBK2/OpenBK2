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

// LF, as the tree's sources are checked out; this used to write CRLF, which
// the generated files are written with byte for byte.
inline CStrStream& endl( CStrStream& sStream ) { sStream << "\n"; return sStream; }
// Between the blocks of generated code: an empty line. It used to be a line of
// 126 slashes, which the port removed from the sources as comments that carry
// no information, leaving the empty line this writes.
inline CStrStream& separator( CStrStream& sStream )
{
	sStream << endl;
	return sStream;
}

inline CStrStream& qcomma( CStrStream& sStream ) { sStream << "\""; return sStream; }
inline CStrStream& tab( CStrStream& sStream ) { sStream << "\t"; return sStream; }

}


