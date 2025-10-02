#include "StdAfx.h"

#include "StrProc.h"

namespace NStr 
{

static int gs_nCodePage = GetACP();

// разделить строку на массив строк по заданному разделителю
template <class T1>
static void SplitStringT( const basic_string<T1> &szString, vector< basic_string<T1> > *pVector, const T1 tSeparator )
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
	} while( nPos != basic_string<T1>::npos );
}
void SplitString( const string &szString, vector<string> *pVector, const char cSeparator )
{
	SplitStringT( szString, pVector, cSeparator );
}
void SplitString( const wstring &szString, vector<wstring> *pVector, const wchar_t cSeparator )
{
	SplitStringT( szString, pVector, cSeparator );
}

template <class TChar>
void SplitStringWithMultipleBracketsT( const basic_string<TChar> &szString, vector<basic_string<TChar> > &szVector, const TChar cSeparator )
{
	for ( CStringIterator<TChar, const basic_string<TChar>&, CBracketSeparator<TChar, SBracketsQuoteTest<TChar> > > it(szString, cSeparator); !it.IsEnd(); it.Next() )
		szVector.push_back( it.Get() );
}

void SplitStringWithMultipleBrackets( const string &szString, vector<string> &szVector, const char cSeparator )
{
	SplitStringWithMultipleBracketsT( szString, szVector, cSeparator );
}
void SplitStringWithMultipleBrackets( const wstring &szString, vector<wstring> &szVector, const wchar_t cSeparator )
{
	SplitStringWithMultipleBracketsT( szString, szVector, cSeparator );
}

// отрезать все символы 'cTrim' справа
void TrimRight( string &szString, const char cTrim )
{
	size_t nPos = szString.find_last_not_of( cTrim );
	if ( nPos == string::npos )
	{
		if ( szString.find_first_of( cTrim ) == 0 )
			szString.clear();
	}
	else
		szString.erase( nPos + 1, string::npos );
}
void TrimRight( string &szString, const char *pszTrim )
{
	size_t nPos = szString.find_last_not_of( pszTrim );
	if ( nPos == string::npos )
	{
		if ( szString.find_first_of( pszTrim ) == 0 )
			szString.clear();
	}
	else
		szString.erase( nPos + 1, string::npos );
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
void TrimInside( string &szString, const char *pszTrim )
{
  szString.erase( remove_if(szString.begin(), szString.end(), CSymbolCheckFunctional(pszTrim)), szString.end() );
}

int ToInt( const char *pszString )
{
	int nNumber = 0;
	sscanf( pszString, "%i", &nNumber );
	return nNumber;
}
float ToFloat( const char *pszString )
{
	float fNumber = 0;
	sscanf( pszString, "%f", &fNumber );
	return fNumber;
}
double ToDouble( const char *pszString )
{
	double fNumber = 0;
	sscanf( pszString, "%lf", &fNumber );
	return fNumber;
}
unsigned long ToULong( const char *pszString )
{
	unsigned long ulNumber = 0;
	sscanf( pszString, "%ul", &ulNumber );
	return ulNumber;
}

// <[+/-]>[dec digit]*
bool IsDecNumber( const string &szString )
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
bool IsOctNumber( const string &szString )
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
bool IsHexNumber( const string &szString )
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
	BYTE *pData = (BYTE*)pBuffer;
	for ( const char *it = pszData; *it != 0; it += 2 )
		*pData++ = ( HexSymbolToHalfByte( *it ) << 4 ) | HexSymbolToHalfByte( *(it +1) );
	if ( pnSize ) 
		*pnSize = int( pData - (BYTE*)pBuffer );
	return pBuffer;
}

// ************************************************************************************************************************ //
// **
// ** перевод Unicode <=> UTF-8
// ** bytes | bits | representation
// **     1 |    7 | 0vvvvvvv
// **     2 |   11 | 110vvvvv 10vvvvvv
// **     3 |   16 | 1110vvvv 10vvvvvv 10vvvvvv
// **     4 |   21 | 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
// **
// ************************************************************************************************************************ //

void UnicodeToUTF8( string *pRes, const wstring &szString )
{
	pRes->resize( 0 );
	pRes->reserve( szString.size() * 2 );
	for ( wstring::const_iterator it = szString.begin(); it != szString.end(); ++it )
	{
		const wchar_t chr = *it;
		if ( chr < 0x80 )
			*pRes += ( chr );
		else if ( chr < 0x800 ) 
		{
			*pRes += ( 0xC0 | chr>>6 );
			*pRes += ( 0x80 | chr & 0x3F );
		}
		else
		{
			*pRes += ( 0xE0 | chr>>12 );
			*pRes += ( 0x80 | chr>>6 & 0x3F );
			*pRes += ( 0x80 | chr & 0x3F );
		}
	}
}
void UTF8ToUnicode( wstring *pRes, const string &szString )
{
	pRes->resize( 0 );
	pRes->reserve( szString.size() );
	string::const_iterator it = szString.begin();
	while ( it != szString.end() ) 
	{
		BYTE chr = BYTE( *it );
		if ( (chr & 0x80) == 0 ) 
			*pRes += chr;
		else if ( (chr & 0xe0) == 0xc0 )	// check first 3 bits ( wchar < 0x800 )
		{
			wchar_t res = (chr & 0x1f) << 6;
			++it;
			chr = BYTE( *it );
			res |= ( chr & 0x3f );
			*pRes += res;
		}
		else if ( (*it & 0xf0) == 0xe0 ) 	// check first 4 bits ( wchar < 0xffff )
		{
			wchar_t res = ( chr & 0x0f ) << 12;
			++it;
			chr = BYTE( *it );
			res |= ( chr & 0x3f ) << 6;
			++it;
			chr = BYTE( *it );
			res |= chr & 0x3f;
			*pRes += res;
		}
		++it;
	}
}

// перевод MBCS <=> Unicode
void SetCodePage( const int nCodePage )
{
	gs_nCodePage = nCodePage;
}
void ToMBCS( string *pRes, const wstring &szSrc )
{
	const int nBuffLen = szSrc.length()*2 + 10;
	pRes->resize( nBuffLen );
	const int nLength = WideCharToMultiByte( gs_nCodePage, 0, szSrc.c_str(), szSrc.length(), &((*pRes)[0]), nBuffLen, 0, 0 );
	pRes->resize( nLength );
}
void ToUnicode( wstring *pRes, const string &szSrc )
{
	const int nBuffLen = szSrc.length() + 3;
	pRes->resize( nBuffLen );
	const int nLength = MultiByteToWideChar( gs_nCodePage, 0, szSrc.c_str(), szSrc.length(), &((*pRes)[0]), nBuffLen );
	pRes->resize( nLength );
}

void UTF8ToMBCS( string *pRes, const string &szSrc )
{
	wstring wszTemp;
	UTF8ToUnicode( &wszTemp, szSrc );
	ToMBCS( pRes, wszTemp );
}
void MBCSToUTF8( string *pRes, const string &szSrc )
{
	wstring wszTemp;
	ToUnicode( &wszTemp, szSrc );
	UnicodeToUTF8( pRes, wszTemp );
}

// GUID => string
void GUID2String( string *pString, const GUID &guid )
{
	*pString = StrFmt( "%.8X-%.4X-%.4X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], 
		guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
}
void String2GUID( const string &szString, GUID *pGuid )
{
	((BYTE*)&pGuid->Data1)[3] = ( HexSymbolToHalfByte( szString[0] ) << 4 ) | HexSymbolToHalfByte( szString[1] );
	((BYTE*)&pGuid->Data1)[2] = ( HexSymbolToHalfByte( szString[2] ) << 4 ) | HexSymbolToHalfByte( szString[3] );
	((BYTE*)&pGuid->Data1)[1] = ( HexSymbolToHalfByte( szString[4] ) << 4 ) | HexSymbolToHalfByte( szString[5] );
	((BYTE*)&pGuid->Data1)[0] = ( HexSymbolToHalfByte( szString[6] ) << 4 ) | HexSymbolToHalfByte( szString[7] );

	((BYTE*)&pGuid->Data2)[1] = ( HexSymbolToHalfByte( szString[9] ) << 4 ) | HexSymbolToHalfByte( szString[10] );
	((BYTE*)&pGuid->Data2)[0] = ( HexSymbolToHalfByte( szString[11] ) << 4 ) | HexSymbolToHalfByte( szString[12] );

	((BYTE*)&pGuid->Data3)[1] = ( HexSymbolToHalfByte( szString[14] ) << 4 ) | HexSymbolToHalfByte( szString[15] );
	((BYTE*)&pGuid->Data3)[0] = ( HexSymbolToHalfByte( szString[16] ) << 4 ) | HexSymbolToHalfByte( szString[17] );

	pGuid->Data4[0] = ( HexSymbolToHalfByte( szString[19] ) << 4 ) | HexSymbolToHalfByte( szString[20] );
	pGuid->Data4[1] = ( HexSymbolToHalfByte( szString[21] ) << 4 ) | HexSymbolToHalfByte( szString[22] );
	pGuid->Data4[2] = ( HexSymbolToHalfByte( szString[24] ) << 4 ) | HexSymbolToHalfByte( szString[25] );
	pGuid->Data4[3] = ( HexSymbolToHalfByte( szString[26] ) << 4 ) | HexSymbolToHalfByte( szString[27] );
	pGuid->Data4[4] = ( HexSymbolToHalfByte( szString[28] ) << 4 ) | HexSymbolToHalfByte( szString[29] );
	pGuid->Data4[5] = ( HexSymbolToHalfByte( szString[30] ) << 4 ) | HexSymbolToHalfByte( szString[31] );
	pGuid->Data4[6] = ( HexSymbolToHalfByte( szString[32] ) << 4 ) | HexSymbolToHalfByte( szString[33] );
	pGuid->Data4[7] = ( HexSymbolToHalfByte( szString[34] ) << 4 ) | HexSymbolToHalfByte( szString[35] );
}

}; // end of namespace NStr


