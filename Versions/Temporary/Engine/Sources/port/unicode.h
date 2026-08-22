#pragma once

// Conversion between the engine's narrow encoding and wide characters.
//
// The narrow encoding is UTF-8, everywhere and unconditionally. On Windows
// that is true because Game.exe carries a manifest setting the process active
// code page to UTF-8, so every narrow Win32 API in the process agrees; this
// therefore needs Windows 10 1903 or later. Saying UTF-8 outright rather than
// asking GetACP() is what makes the two platforms behave the same: a Linux
// locale is not guaranteed to be UTF-8 either.

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <iconv.h>
#endif

#include <string>

namespace NPortDetail
{

#if !BOOST_OS_WINDOWS

// One iconv descriptor. iconv_t carries conversion state, so a descriptor
// cannot be used from two threads at once; every holder below is thread_local.
class CIconv
{
	iconv_t cd;
public:
	CIconv( const CIconv& ) = delete;
	void operator=( const CIconv& ) = delete;

	CIconv( const char *pszTo, const char *pszFrom ) : cd( ::iconv_open( pszTo, pszFrom ) ) {}
	~CIconv() { if ( IsOk() ) { ::iconv_close( cd ); } }

	bool IsOk() const { return cd != (iconv_t)-1; }

	//! Bytes written, or 0 for both failure and empty input. Callers treat the
	//! two the same, which is what the Win32 side does with its return of 0.
	size_t Convert( const char *pIn, size_t nInBytes, char *pOut, size_t nOutBytes )
	{
		if ( !IsOk() )
		{
			return 0;
		}
		// drop any state left by a previous partial conversion
		::iconv( cd, 0, 0, 0, 0 );
		char *pInCur = const_cast< char * >( pIn );
		char *pOutCur = pOut;
		size_t nInLeft = nInBytes, nOutLeft = nOutBytes;
		if ( ::iconv( cd, &pInCur, &nInLeft, &pOutCur, &nOutLeft ) == (size_t)-1 )
		{
			return 0;
		}
		return nOutBytes - nOutLeft;
	}
};

#endif

}

//! Wide to UTF-8. Returns an empty string if the input is empty or cannot be
//! converted; no caller here distinguishes the two.
inline std::string WideToUTF8( const std::wstring &value )
{
	if ( value.empty() )
	{
		return std::string();
	}
#if BOOST_OS_WINDOWS
	// ask for the length first, so the result is exactly the right size rather
	// than the four-bytes-per-character worst case
	const int nBytes = ::WideCharToMultiByte( CP_UTF8, 0, value.data(), static_cast< int >( value.size() ), 0, 0, 0, 0 );
	if ( nBytes <= 0 )
	{
		return std::string();
	}
	std::string result( static_cast< size_t >( nBytes ), '\0' );
	::WideCharToMultiByte( CP_UTF8, 0, value.data(), static_cast< int >( value.size() ), &result[0], nBytes, 0, 0 );
	return result;
#else
	static thread_local NPortDetail::CIconv conv( "UTF-8", "WCHAR_T" );
	// a code point is at most 4 bytes in UTF-8, and at least one wchar_t
	std::string result( value.size() * 4, '\0' );
	const size_t nBytes = conv.Convert( reinterpret_cast< const char * >( value.data() ),
	                                    value.size() * sizeof( wchar_t ),
	                                    &result[0], result.size() );
	result.resize( nBytes );
	return result;
#endif
}

//! UTF-8 to wide. Same convention on failure.
inline std::wstring UTF8ToWide( const std::string &value )
{
	if ( value.empty() )
	{
		return std::wstring();
	}
#if BOOST_OS_WINDOWS
	const int nChars = ::MultiByteToWideChar( CP_UTF8, 0, value.data(), static_cast< int >( value.size() ), 0, 0 );
	if ( nChars <= 0 )
	{
		return std::wstring();
	}
	std::wstring result( static_cast< size_t >( nChars ), L'\0' );
	::MultiByteToWideChar( CP_UTF8, 0, value.data(), static_cast< int >( value.size() ), &result[0], nChars );
	return result;
#else
	static thread_local NPortDetail::CIconv conv( "WCHAR_T", "UTF-8" );
	// one byte of UTF-8 can never produce more than one code point
	std::wstring result( value.size(), L'\0' );
	const size_t nBytes = conv.Convert( value.data(), value.size(),
	                                    reinterpret_cast< char * >( &result[0] ),
	                                    result.size() * sizeof( wchar_t ) );
	result.resize( nBytes / sizeof( wchar_t ) );
	return result;
#endif
}
