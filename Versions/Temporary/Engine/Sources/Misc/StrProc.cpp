#include "stdafx.h"

#include "StrProc.h"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <system_error>

#include <fmt/format.h>

namespace NStr 
{

// разделить строку на массив строк по заданному разделителю
template <class T1>
static void SplitStringT( const std::basic_string<T1> &szString, std::vector< std::basic_string<T1> > *pVector, const T1 tSeparator )
{
	int nPos = 0, nLastPos = 0;
	//
	do
	{
		nPos = szString.find( tSeparator, nLastPos );
		// add string
		pVector->push_back( szString.substr( nLastPos, nPos - nLastPos ) );
		nLastPos = nPos + 1;//szString.find_first_not_of( cSeparator, nPos );
		//
	} while( nPos != std::basic_string<T1>::npos );
}
void SplitString( const std::string &szString, std::vector<std::string> *pVector, const char cSeparator )
{
	SplitStringT( szString, pVector, cSeparator );
}
void SplitString( const std::wstring &szString, std::vector<std::wstring> *pVector, const wchar_t cSeparator )
{
	SplitStringT( szString, pVector, cSeparator );
}

template <class TChar>
void SplitStringWithMultipleBracketsT( const std::basic_string<TChar> &szString, std::vector<std::basic_string<TChar> > &szVector, const TChar cSeparator )
{
	for ( CStringIterator<TChar, const std::basic_string<TChar>&, CBracketSeparator<TChar, SBracketsQuoteTest<TChar> > > it(szString, cSeparator); !it.IsEnd(); it.Next() )
		szVector.push_back( it.Get() );
}

void SplitStringWithMultipleBrackets( const std::string &szString, std::vector<std::string> &szVector, const char cSeparator )
{
	SplitStringWithMultipleBracketsT( szString, szVector, cSeparator );
}
void SplitStringWithMultipleBrackets( const std::wstring &szString, std::vector<std::wstring> &szVector, const wchar_t cSeparator )
{
	SplitStringWithMultipleBracketsT( szString, szVector, cSeparator );
}

// отрезать все символы 'cTrim' справа
void TrimRight( std::string &szString, const char cTrim )
{
	size_t nPos = szString.find_last_not_of( cTrim );
	if ( nPos == std::string::npos )
	{
		if ( szString.find_first_of( cTrim ) == 0 )
			szString.clear();
	}
	else
		szString.erase( nPos + 1, std::string::npos );
}
void TrimRight( std::string &szString, const char *pszTrim )
{
	size_t nPos = szString.find_last_not_of( pszTrim );
	if ( nPos == std::string::npos )
	{
		if ( szString.find_first_of( pszTrim ) == 0 )
			szString.clear();
	}
	else
		szString.erase( nPos + 1, std::string::npos );
}
// вырезать все символы 'cTrim' из строки
class CSymbolCheckFunctional
{
private:
  const char *pszSymbols;
public:
  explicit CSymbolCheckFunctional( const char *pszNewSymbols ) : pszSymbols( pszNewSymbols ) {  }
  bool operator()( const char cSymbol )
  {
    for ( const char *p = pszSymbols; *p != 0; ++p )
    {
      if ( *p == cSymbol )
        return true;
    }
    return false;
  }
};
void TrimInside( std::string &szString, const char *pszTrim )
{
  szString.erase( remove_if(szString.begin(), szString.end(), CSymbolCheckFunctional(pszTrim)), szString.end() );
}

// sscanf took the decimal point from LC_NUMERIC and said nothing when the
// input did not parse. from_chars ignores the locale and reports an error;
// on any error these keep returning 0, which is what callers saw before for
// input that did not parse.
// sscanf also skipped leading whitespace and accepted a leading +, and
// from_chars does neither, so both are handled here.
static const char *SkipSpace( const char *pszString )
{
	while ( *pszString == ' ' || *pszString == '\t' || *pszString == '\n' || *pszString == '\r' )
	{
		++pszString;
	}
	return pszString;
}
// "%i" picked the radix from the prefix; from_chars needs it spelled out
static int PickRadix( const char **ppString )
{
	const char *p = *ppString;
	if ( p[0] == '0' && ( p[1] == 'x' || p[1] == 'X' ) )
	{
		*ppString = p + 2;
		return 16;
	}
	if ( p[0] == '0' && p[1] >= '0' && p[1] <= '7' )
	{
		return 8;
	}
	return 10;
}
int ToInt( const char *pszString )
{
	const char *p = SkipSpace( pszString );
	const bool bNegative = ( *p == '-' );
	if ( *p == '+' || *p == '-' )
	{
		++p;
	}
	const int nRadix = PickRadix( &p );
	unsigned int nMagnitude = 0;
	const std::from_chars_result res = std::from_chars( p, p + strlen( p ), nMagnitude, nRadix );
	if ( res.ec != std::errc() )
	{
		return 0;
	}
	return bNegative ? -static_cast<int>( nMagnitude ) : static_cast<int>( nMagnitude );
}
int ToIntDec( const char *pszString )
{
	const char *p = SkipSpace( pszString );
	const bool bNegative = ( *p == '-' );
	if ( *p == '+' || *p == '-' )
	{
		++p;
	}
	unsigned int nMagnitude = 0;
	const std::from_chars_result res = std::from_chars( p, p + strlen( p ), nMagnitude, 10 );
	if ( res.ec != std::errc() )
	{
		return 0;
	}
	return bNegative ? -static_cast<int>( nMagnitude ) : static_cast<int>( nMagnitude );
}
float ToFloat( const char *pszString )
{
	const char *p = SkipSpace( pszString );
	const bool bNegative = ( *p == '-' );
	if ( *p == '+' || *p == '-' )
	{
		++p;
	}
	const char *const pEnd = p + strlen( p );
	float fNumber = 0;
	std::from_chars_result res;
	if ( p[0] == '0' && ( p[1] == 'x' || p[1] == 'X' ) )
	{
		// "%f" accepted a hex float; from_chars wants the prefix removed
		res = std::from_chars( p + 2, pEnd, fNumber, std::chars_format::hex );
	}
	else
	{
		res = std::from_chars( p, pEnd, fNumber );
	}
	if ( res.ec != std::errc() )
	{
		return 0;
	}
	return bNegative ? -fNumber : fNumber;
}
double ToDouble( const char *pszString )
{
	const char *p = SkipSpace( pszString );
	const bool bNegative = ( *p == '-' );
	if ( *p == '+' || *p == '-' )
	{
		++p;
	}
	const char *const pEnd = p + strlen( p );
	double fNumber = 0;
	std::from_chars_result res;
	if ( p[0] == '0' && ( p[1] == 'x' || p[1] == 'X' ) )
	{
		// "%f" accepted a hex float; from_chars wants the prefix removed
		res = std::from_chars( p + 2, pEnd, fNumber, std::chars_format::hex );
	}
	else
	{
		res = std::from_chars( p, pEnd, fNumber );
	}
	if ( res.ec != std::errc() )
	{
		return 0;
	}
	return bNegative ? -fNumber : fNumber;
}
unsigned long ToULong( const char *pszString )
{
	const char *p = SkipSpace( pszString );
	if ( *p == '+' )
	{
		++p;
	}
	unsigned long ulNumber = 0;
	const std::from_chars_result res = std::from_chars( p, p + strlen( p ), ulNumber );
	if ( res.ec != std::errc() )
	{
		return 0;
	}
	return ulNumber;
}

// <[+/-]>[dec digit]*
bool IsDecNumber( const std::string &szString )
{
	if ( szString.empty() )
		return false;
	int i, nFirstDigit = IsSign( szString[0] ) ? 1 : 0;
	int nNumDigits = szString.size() - nFirstDigit;
	if ( nNumDigits == 0 )
		return false;												// this is not a number at all => zero length digits
	if ( (nNumDigits > 1) && (szString[nFirstDigit] == '0') )
		return false;												// hex number
	for ( i=nFirstDigit; (i < szString.size()) && IsDecDigit(szString[i]); ++i ) { ; }
	return ( (i > nFirstDigit) && (i == szString.size()) );
}
// <[+/-]>[0][oct digit]*
bool IsOctNumber( const std::string &szString )
{
	if ( szString.empty() )
		return false;
	int i, nFirstDigit = IsSign( szString[0] ) ? 1 : 0;
	int nNumDigits = szString.size() - nFirstDigit;
	if ( nNumDigits == 0 )
		return false;
	if ( szString[nFirstDigit] != '0' )
		return false;
	if ( nNumDigits < 2 )
		return false;

	for ( i=nFirstDigit; (i < szString.size()) && IsOctDigit(szString[i]); ++i ) { ; }
	return ( (i > nFirstDigit) && (i == szString.size()) );
}
// <[+/-]>[0x][hex digit]*
bool IsHexNumber( const std::string &szString )
{
	if ( szString.empty() )
		return false;
	int i, nFirstDigit = IsSign( szString[0] ) ? 1 : 0;
	int nNumDigits = szString.size() - nFirstDigit;
	if ( nNumDigits < 3 )
		return false;
	if ( (szString[nFirstDigit] != '0') || (szString[nFirstDigit + 1] != 'x') )
		return false;
	for ( i=nFirstDigit + 2; (i < szString.size()) && IsHexDigit(szString[i]); ++i ) { ; }
	return ( (i > nFirstDigit) && (i == szString.size()) );
}

// ************************************************************************************************************************ //
// **
// ** string-to-bin and vice versa
// **
// **
// **
// ************************************************************************************************************************ //

const char* BinToString( const void *pData, int nSize, char *pszBuffer )
{
	char *pszCurr = pszBuffer;
	for ( const unsigned char *it = (unsigned char*)pData; it != (unsigned char*)pData + nSize; ++it )
	{
		*pszCurr++ = HalfByteToHexSymbol( ((*it) >> 4) & 0x0f );
		*pszCurr++ = HalfByteToHexSymbol( (*it) & 0x0f );
	}
	return pszBuffer;
}
void* StringToBin( const char *pszData, void *pBuffer, int *pnSize )
{
	uint8_t *pData = (uint8_t*)pBuffer;
	for ( const char *it = pszData; *it != 0; it += 2 )
		*pData++ = ( HexSymbolToHalfByte( *it ) << 4 ) | HexSymbolToHalfByte( *(it +1) );
	if ( pnSize ) 
		*pnSize = int( pData - (uint8_t*)pBuffer );
	return pBuffer;
}

}; // end of namespace NStr


