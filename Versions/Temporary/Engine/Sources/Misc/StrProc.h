#pragma once
#include "Misc_export.h"

#include <algorithm>

#include <boost/config.hpp>

namespace NStr
{

// слить последовательность строк в одну строку, элементы последовательности
// должны уметь приводиться к string
template< class It >
std::string Join( It first, It last, const std::string &szSeparator = " " )
{
	if ( first != last )
	{
		It cur = first;
		std::string szRes = *(cur++);

		while ( cur != last )
		{
			szRes += szSeparator + *cur;
			++ cur;
		}

		return szRes;
	}
	else
		return "";
}


// разделить строку на массив строк по заданному разделителю
MISC_EXPORT void SplitString( const std::string &szString, std::vector<std::string> *pVector, const char cSeparator );
MISC_EXPORT void SplitString( const std::wstring &szString, std::vector<std::wstring> *pVector, const wchar_t cSeparator );
// разделить строку на массив строк по заданному разделителю с учётом скобок любой вложенности
MISC_EXPORT void SplitStringWithMultipleBrackets( const std::string &szString, std::vector<std::string> &szVector, const char cSeparator );
MISC_EXPORT void SplitStringWithMultipleBrackets( const std::wstring &szString, std::vector<std::wstring> &szVector, const wchar_t cSeparator );
// отрезать все символы 'cTrim'
// отрезать все 'cTrim' слева
inline void TrimLeft( std::string &szString, const char cTrim ) { szString.erase( 0, szString.find_first_not_of( cTrim ) ); }
// отрезать все 'pszTrim' слева
inline void TrimLeft( std::string &szString, const char *pszTrim ) { szString.erase( 0, szString.find_first_not_of( pszTrim ) ); }
// отрезать все whitespaces слева
inline void TrimLeft( std::string &szString ) { TrimLeft(szString, " \t\n\r"); }
// отрезать все 'pszTrim' справа
MISC_EXPORT void TrimRight( std::string &szString, const char *pszTrim );
// отрезать все 'cTrim' справа
MISC_EXPORT void TrimRight( std::string &szString, const char cTrim );
// отрезать все whitespaces справа
inline void TrimRight( std::string &szString ) { TrimRight(szString, " \t\n\r"); }
// отрезать все 'pszTrim' с обоих концов
inline void TrimBoth( std::string &szString, const char *pszTrim ) { TrimLeft( szString, pszTrim ); TrimRight( szString, pszTrim ); }
// отрезать все 'cTrim' с обоих концов
inline void TrimBoth( std::string &szString, const char cTrim ) { TrimLeft( szString, cTrim ); TrimRight( szString, cTrim ); }
// отрезать все whitespaces с обоих концов
inline void TrimBoth( std::string &szString ) { TrimBoth(szString, " \t\n\r"); }
// вырезать все символы 'cTrim' из строки
void TrimInside( std::string &szString, const char *pszTrim );
inline void TrimInside( std::string &szString, const char cTrim ) { szString.erase( remove(szString.begin(), szString.end(), cTrim), szString.end() ); }
inline void TrimInside( std::string &szString ) { TrimInside(szString, " \t\n\r"); }

template<class T>
void FastSearch( const char *pszBegin, const int nSize, const std::string &szSample, std::vector<int> *pFoundEntriesPos, T charsComparer );

template<class T>
int FastSerachFirst( const char *pszBegin, const std::string &szSample, T charsComparer );

template<class T>
int SerachFirst( const char *pszBegin, const std::string &szSample, T charsComparer );

// привести к верхнему или нижнему регистру
// MSVCMustDie_* are required to keep compiler happy when default calling conversion is __fastcall
inline int MSVCMustDie_tolower( int a ) { return tolower(a); }
inline int MSVCMustDie_toupper( int a ) { return toupper(a); }
inline void ToLower( std::string *pRes )
{
	std::transform( pRes->begin(), pRes->end(), pRes->begin(), MSVCMustDie_tolower );
}
inline void ToLower( std::string *pRes, const std::string &szString )
{
	pRes->resize( szString.size() );
	std::transform( szString.begin(), szString.end(), pRes->begin(), MSVCMustDie_tolower );
}
inline void ToUpper( std::string *pRes )
{
	std::transform( pRes->begin(), pRes->end(), pRes->begin(), MSVCMustDie_toupper );
}
inline void ToUpper( std::string *pRes, const std::string &szString )
{
	pRes->resize( szString.size() );
	transform( szString.begin(), szString.end(), pRes->begin(), MSVCMustDie_toupper );
}

// to upper
BOOST_FORCEINLINE char ASCII_toupper( const char chr ) { return chr >= 'a' && chr <= 'z' ? chr - 'a' + 'A' : chr; }

// упрощённая и ускоренная версия tolower - работает только на первой половине кодовой таблицы!
BOOST_FORCEINLINE char ASCII_tolower( const char chr ) { return chr - ( ('A' - 'a') & ( (('A' - chr - 1) & (chr - 'Z' - 1)) >> 7 ) ); }
inline void ToLowerASCII( std::string *pRes )
{
	for ( std::string::iterator it = pRes->begin(); it != pRes->end(); ++it )
		*it = ASCII_tolower( *it );
}
inline void ToLowerASCII( std::string *pRes, const std::string &szString )
{
	const int nSize = szString.size();
	pRes->resize( nSize );
	for ( int i = 0; i < nSize; ++i )
		(*pRes)[i] = ASCII_tolower( szString[i] );
}

// convert 'string', which represents integer value in any radix (oct, dec, hex) to 'int'
MISC_EXPORT int ToInt( const char *pszString );
inline int ToInt( const std::string &szString ) { return ToInt( szString.c_str() ); }
unsigned long ToULong( const char *pszString );
inline unsigned long ToULong( const std::string &szString ) { return ToULong( szString.c_str() ); }
// convert 'string', which represents FP value to 'float' and 'double'
MISC_EXPORT float ToFloat( const char *pszString );
inline float ToFloat( const std::string &szString ) { return ToFloat( szString.c_str() ); }
double ToDouble( const char *pszString );
inline double ToDouble( const std::string &szString ) { return ToDouble( szString.c_str() ); }

// является ли строка представлением числа
inline bool IsBinDigit( const char cChar ) { return ( (cChar == '0') && (cChar == '1') ); }
inline bool IsOctDigit( const char cChar ) { return ( (cChar >= '0') && (cChar <= '7') ); }
inline bool IsDecDigit( const char cChar ) { return ( (cChar >= '0') && (cChar <= '9') ); }
inline bool IsHexDigit( const char cChar ) { return ( (cChar >= '0') && (cChar <= '9') ) || ( (cChar >= 'a') && (cChar <= 'f') ) || ( (cChar >= 'A') && (cChar <= 'F') ); }
inline bool IsSign( const char cChar ) { return ( (cChar == '-') || (cChar == '+') ); }
MISC_EXPORT bool IsDecNumber( const std::string &szString );
bool IsOctNumber( const std::string &szString );
MISC_EXPORT bool IsHexNumber( const std::string &szString );

// перевод string => bin и обратно
// NOTE: BinToString() doesn't attach '\0' at the end!!!
MISC_EXPORT void* StringToBin( const char *pszData, void *pBuffer, int *pnSize );
MISC_EXPORT const char* BinToString( const void *pData, int nSize, char *pszBuffer );
BOOST_FORCEINLINE char HalfByteToHexSymbol( const unsigned char chr ) { return chr >= 10 ? 'a' + ( chr - 10 ) : '0' + chr; }
BOOST_FORCEINLINE unsigned char HexSymbolToHalfByte( const char chr )
{
	if ( chr >= 'a' )
		return chr - 'a' + 10;
	else if ( chr >= 'A' )
		return chr - 'A' + 10;
	else
		return chr - '0';
}
inline std::string ToHex(int number)
{
	std::stringstream stream;
	stream << std::hex << std::uppercase << number;
	std::string result( stream.str() );
	return result;
}

// перевод UNICODE => UTF-8 и обратно
MISC_EXPORT void UnicodeToUTF8( std::string *pRes, const std::wstring &szString );
MISC_EXPORT void UTF8ToUnicode( std::wstring *pRes, const std::string &szString );

// перевод MBCS => Unicode и обратно
MISC_EXPORT void SetCodePage( const int nCodePage );
MISC_EXPORT void ToMBCS( std::string *pRes, const std::wstring &szSrc );
inline std::string ToMBCS( const std::wstring &szSrc ) { std::string szDst; ToMBCS( &szDst, szSrc ); return szDst; }
MISC_EXPORT void ToUnicode( std::wstring *pRes, const std::string &szSrc );
inline std::wstring ToUnicode( const std::string &szSrc ) { std::wstring szDst; ToUnicode( &szDst, szSrc ); return szDst; }

// перевод MBCS => UTF-8 и обратно
MISC_EXPORT void UTF8ToMBCS( std::string *pRes, const std::string &szSrc );
MISC_EXPORT void MBCSToUTF8( std::string *pRes, const std::string &szSrc );

template <class TChar>
void ReplaceAllChars( std::basic_string<TChar> *pString, const TChar tFrom, const TChar tTo )
{
	for ( typename std::basic_string<TChar>::iterator it = pString->begin(); it != pString->end(); ++it )
	{
		if ( *it == tFrom )
			*it = tTo;
	}
}

// ************************************************************************************************************************ //
// **
// ** string iterator end it's helper classes
// **
// **
// **
// ************************************************************************************************************************ //

template <typename TChar>
class CCharSeparator
{
	const TChar tChr;
public:
	CCharSeparator( const TChar chr )
		: tChr( chr ) {  }
	bool operator()( const TChar tSymbol ) const { return tSymbol == tChr; }
};


template <class TChar>
struct SQuoteTest
{
	static BOOST_FORCEINLINE bool IsOpen( const TChar chr )
	{
		return ( chr == TChar('\"') );
	}
	static BOOST_FORCEINLINE bool IsClose( const TChar chr )
	{
		return ( chr == TChar('\"') );
	}
	static BOOST_FORCEINLINE TChar GetClose( const TChar chr )
	{
		return chr == TChar('\"') ? TChar('\"') : TChar( -1 );
	}
};


template <class TChar>
struct SBracketsTest
{
	static BOOST_FORCEINLINE bool IsOpen( const TChar chr )
	{
		return ( chr == TChar('(') ) || ( chr == TChar('[') ) || ( chr == TChar('{') ) || ( chr == TChar('<') );
	}
	static BOOST_FORCEINLINE bool IsClose( const TChar chr )
	{
		return ( chr == TChar(')') ) || ( chr == TChar(']') ) || ( chr == TChar('}') ) || ( chr == TChar('>') );
	}
	static BOOST_FORCEINLINE TChar GetClose( const TChar chr )
	{
		switch ( chr )
		{
			case '('	:	return TChar( ')' );
			case '['	:	return TChar( ']' );
			case '{'	:	return TChar( '}' );
			case '<'	:	return TChar( '>' );
		}
		return TChar( -1 );
	}
};

template <class TChar>
struct SBracketsQuoteTest
{
	static BOOST_FORCEINLINE bool IsOpen( const TChar chr )
	{
		return ( chr == TChar('(') ) || ( chr == TChar('[') ) || ( chr == TChar('{') ) || ( chr == TChar('<') ) || ( chr == TChar('\"') ) || ( chr == TChar('\'') );
	}
	static BOOST_FORCEINLINE bool IsClose( const char chr )
	{
		return ( chr == TChar(')') ) || ( chr == TChar(']') ) || ( chr == TChar('}') ) || ( chr == TChar('>') ) || ( chr == TChar('\"') ) || ( chr == TChar('\'') );
	}
	static BOOST_FORCEINLINE TChar GetClose( const char chr )
	{
		switch ( chr )
		{
			case '('	:	return TChar( ')'  );
			case '['	:	return TChar( ']'  );
			case '{'	:	return TChar( '}'  );
			case '<'	:	return TChar( '>'  );
			case '\"'	:	return TChar( '\"' );
			case '\''	:	return TChar( '\'' );
		}
		return TChar( -1 );
	}
};

template <class TChar, class TBrackets = SBracketsTest<TChar> >
class CBracketSeparator
{
	const TChar cSeparator;								// separator char
	std::vector<TChar> stc;										// close brackets stack
public:
	CBracketSeparator( const TChar _chr )
		: cSeparator( _chr ) { stc.reserve(32); }
	//
	bool operator()( const TChar chr )
	{
		if ( stc.empty() )
		{
			if ( TBrackets::IsOpen(chr) )
				stc.push_back( TBrackets::GetClose(chr) );
			else
			{
				if ( chr == cSeparator )
					return true;
			}
		}
		else if ( chr == stc.back() )
			stc.pop_back();
		//
		return false;
	}
};

template <class TChar, class TStorage = std::basic_string<TChar>, class TSeparator = CCharSeparator<TChar> >
class CStringIterator
{
	TStorage szInput;											// input string
	int nPrevPos;													// previous found position
	int nCurrPos;													// current found position
	TSeparator separator;									// separator functional
public:
	CStringIterator( const TChar *pszString, const TChar cSeparator )
		: szInput( pszString ), nPrevPos( -1 ), nCurrPos( -1 ), separator( cSeparator ) { Next(); }
	CStringIterator( const std::basic_string<TChar> &szString, const TChar cSeparator )
		: szInput( szString ), nPrevPos( -1 ), nCurrPos( -1 ), separator( cSeparator ) { Next(); }
	// iterate to next tag position
	void Next()
	{
		nPrevPos = nCurrPos + 1;
		for ( int i = nPrevPos; i < szInput.size(); ++i )
		{
			if ( separator(szInput[i]) )
			{
				nCurrPos = i;
				return;
			}
		}
		nCurrPos = szInput.size();
	}
	// are we finished iteration?
	bool IsEnd() const
	{
		return nPrevPos > nCurrPos;
	}
	//
	std::basic_string<TChar> Get() const
	{
		return szInput.substr(nPrevPos, nCurrPos - nPrevPos);
	}
	void Get( std::basic_string<TChar> *pString )
	{
		if ( nCurrPos > nPrevPos )
		{
			pString->resize( nCurrPos - nPrevPos );
			memcpy( &((*pString)[0]), &(szInput[nPrevPos]), (nCurrPos - nPrevPos) * sizeof(TChar) );
		}
		else
			pString->clear();
	}
	int GetPrevPos() const { return nPrevPos; }
	int GetCurrPos() const { return nCurrPos; }
};

namespace NImplementation
{
	struct SSearchStr
	{
		const char *pszBegin;
		const int nLength;
		const std::string &szSample;

		SSearchStr( const char *_pszBegin, const int _nLength, const std::string &_szSample )
			: pszBegin( _pszBegin ), nLength( _nLength ), szSample( _szSample ) { }

		const char operator[]( const int nIndex ) const
		{
			NI_VERIFY( nIndex <= nLength + szSample.size() + 1, "wrong index", return '$' );
			if ( nIndex <= szSample.size() )
				return szSample[nIndex-1];
			else if ( nIndex == szSample.size() + 1 )
				return ' ';
			else
				return *(pszBegin + ( nIndex - szSample.size() - 2 ));
		}
	};

	struct SPrefixesArray
	{
		const int nSampleSize;
		std::vector<int> sizes;

		SPrefixesArray( const int _nSampleSize ) : nSampleSize( _nSampleSize ), sizes( nSampleSize + 2, 0 ) { }
		int& operator[]( const int nIndex )
		{
			return nIndex <= nSampleSize ? sizes[nIndex] : sizes[nSampleSize + 1];
		}
	};
}
template<class T>
void FastSearch( const char *pszBegin, const int nSize, const std::string &szSample, std::vector<int> *pFoundEntriesPos, T charsComparer )
{
	if ( nSize == 0 || szSample.empty() )
		return;

	NImplementation::SSearchStr str( pszBegin, nSize, szSample );
	NImplementation::SPrefixesArray prefixes( szSample.size() + 2 );

	int i = 1;
	while ( i != nSize + szSample.size() + 1 )
	{
		if ( i == szSample.size() )
			prefixes[i+1] = 0;
		else
		{
			int nLen = prefixes[i];
			// special case: all symbols in the szSample are equal
			if ( nLen == szSample.size() && prefixes[nLen] == nLen - 1 && str[i + 1] == str[nLen] )
				prefixes[i+1] = nLen;
			else
			{
				while ( nLen > 0 && ( nLen == szSample.size() || !charsComparer( str[nLen + 1], str[i + 1] ) ) )
					nLen = prefixes[nLen];

				prefixes[i + 1] = charsComparer( str[nLen + 1], str[i + 1] ) ? nLen + 1 : 0;
			}

			if ( prefixes[i + 1] == szSample.size() )
				pFoundEntriesPos->push_back( i - 2 * szSample.size() );
		}

		++i;
	}
}

template<class T>
int FastSerachFirst( const char *pszBegin, const std::string &szSample, T charsComparer )
{
	const int nSize = strlen( pszBegin );
	if ( nSize == 0 || szSample.empty() )
		return -1;

	NImplementation::SSearchStr str( pszBegin, nSize, szSample );
	NImplementation::SPrefixesArray prefixes( szSample.size() );

	int i = 1;
	while ( i < nSize + szSample.size() )
	{
		int nLen = prefixes[i - 1];
		if ( nLen == szSample.size() )
			nLen = prefixes[nLen];
		while ( nLen > 0 && !charsComparer( str[nLen], str[i] ) )
			nLen = prefixes[nLen];

		if ( charsComparer( str[nLen], str[i] ) )
		{
			prefixes[i] = nLen + 1;
			if ( prefixes[i] == szSample.size() && i != szSample.size() - 1 )
				return i + 1 - 2 * szSample.size();
		}
		else
			prefixes[i] = 0;

		++i;
	}
	return -1;
}

template<class T>
int SerachFirst( const char *pszBegin, const std::string &szSample, T charsComparer )
{
	const int nStringSize = strlen( pszBegin );
	const int nPatternSize = szSample.size();
	for ( int i = 0; i <= nStringSize - nPatternSize; ++i )
	{
		int n = 0;
		while ( n < nPatternSize - 1 && charsComparer( pszBegin[i+n], szSample[n] ) )
			++n;
		if ( charsComparer( pszBegin[i+n], szSample[n] ) )
			return i;
	}
	return -1;
}

struct SASCIICharsComparer
{
	const bool operator()( const char ch1, const char ch2 ) const
	{
		return NStr::ASCII_tolower( ch1 ) == NStr::ASCII_tolower( ch2 );
	}
};

}; // end of namespace NStr


