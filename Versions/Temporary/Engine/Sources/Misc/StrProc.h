#pragma once
#include "Misc_export.h"

namespace NStr
{

// слить последовательность строк в одну строку, элементы последовательности
// должны уметь приводиться к string
template< class It >
string Join( It first, It last, const string &szSeparator = " " )
{
	if ( first != last )
	{
		It cur = first;
		string szRes = *(cur++);

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
MISC_EXPORT void SplitString( const string &szString, vector<string> *pVector, const char cSeparator );
MISC_EXPORT void SplitString( const wstring &szString, vector<wstring> *pVector, const wchar_t cSeparator );
// разделить строку на массив строк по заданному разделителю с учётом скобок любой вложенности
MISC_EXPORT void SplitStringWithMultipleBrackets( const string &szString, vector<string> &szVector, const char cSeparator );
MISC_EXPORT void SplitStringWithMultipleBrackets( const wstring &szString, vector<wstring> &szVector, const wchar_t cSeparator );
// отрезать все символы 'cTrim'
// отрезать все 'cTrim' слева
inline void TrimLeft( string &szString, const char cTrim ) { szString.erase( 0, szString.find_first_not_of( cTrim ) ); }
// отрезать все 'pszTrim' слева
inline void TrimLeft( string &szString, const char *pszTrim ) { szString.erase( 0, szString.find_first_not_of( pszTrim ) ); }
// отрезать все whitespaces слева
inline void TrimLeft( string &szString ) { TrimLeft(szString, " \t\n\r"); } 
// отрезать все 'pszTrim' справа
MISC_EXPORT void TrimRight( string &szString, const char *pszTrim );
// отрезать все 'cTrim' справа
MISC_EXPORT void TrimRight( string &szString, const char cTrim );
// отрезать все whitespaces справа
inline void TrimRight( string &szString ) { TrimRight(szString, " \t\n\r"); }
// отрезать все 'pszTrim' с обоих концов
inline void TrimBoth( string &szString, const char *pszTrim ) { TrimLeft( szString, pszTrim ); TrimRight( szString, pszTrim ); }
// отрезать все 'cTrim' с обоих концов
inline void TrimBoth( string &szString, const char cTrim ) { TrimLeft( szString, cTrim ); TrimRight( szString, cTrim ); }
// отрезать все whitespaces с обоих концов
inline void TrimBoth( string &szString ) { TrimBoth(szString, " \t\n\r"); }
// вырезать все символы 'cTrim' из строки
void TrimInside( string &szString, const char *pszTrim );
inline void TrimInside( string &szString, const char cTrim ) { szString.erase( remove(szString.begin(), szString.end(), cTrim), szString.end() ); }
inline void TrimInside( string &szString ) { TrimInside(szString, " \t\n\r"); }

template<class T>
void FastSearch( const char *pszBegin, const int nSize, const string &szSample, vector<int> *pFoundEntriesPos, T charsComparer );

template<class T>
int FastSerachFirst( const char *pszBegin, const string &szSample, T charsComparer );

template<class T>
int SerachFirst( const char *pszBegin, const string &szSample, T charsComparer );

// привести к верхнему или нижнему регистру
// MSVCMustDie_* are required to keep compiler happy when default calling conversion is __fastcall
inline int MSVCMustDie_tolower( int a ) { return tolower(a); } 
inline int MSVCMustDie_toupper( int a ) { return toupper(a); }
inline void ToLower( string *pRes )
{ 
	transform( pRes->begin(), pRes->end(), pRes->begin(), MSVCMustDie_tolower ); 
}
inline void ToLower( string *pRes, const string &szString )
{ 
	pRes->resize( szString.size() );
	transform( szString.begin(), szString.end(), pRes->begin(), MSVCMustDie_tolower ); 
}
inline void ToUpper( string *pRes )
{ 
	transform( pRes->begin(), pRes->end(), pRes->begin(), MSVCMustDie_toupper ); 
}
inline void ToUpper( string *pRes, const string &szString )
{ 
	pRes->resize( szString.size() );
	transform( szString.begin(), szString.end(), pRes->begin(), MSVCMustDie_toupper ); 
}

// to upper
__forceinline char ASCII_toupper( const char chr ) { return chr >= 'a' && chr <= 'z' ? chr - 'a' + 'A' : chr; }

// упрощённая и ускоренная версия tolower - работает только на первой половине кодовой таблицы!
__forceinline char ASCII_tolower( const char chr ) { return chr - ( ('A' - 'a') & ( (('A' - chr - 1) & (chr - 'Z' - 1)) >> 7 ) ); }
inline void ToLowerASCII( string *pRes )
{ 
	for ( string::iterator it = pRes->begin(); it != pRes->end(); ++it )
		*it = ASCII_tolower( *it );
}
inline void ToLowerASCII( string *pRes, const string &szString )
{ 
	const int nSize = szString.size();
	pRes->resize( nSize );
	for ( int i = 0; i < nSize; ++i )
		(*pRes)[i] = ASCII_tolower( szString[i] );
}

// convert 'string', which represents integer value in any radix (oct, dec, hex) to 'int'
MISC_EXPORT int ToInt( const char *pszString );
inline int ToInt( const string &szString ) { return ToInt( szString.c_str() ); }
unsigned long ToULong( const char *pszString );
inline unsigned long ToULong( const string &szString ) { return ToULong( szString.c_str() ); }
// convert 'string', which represents FP value to 'float' and 'double'
MISC_EXPORT float ToFloat( const char *pszString );
inline float ToFloat( const string &szString ) { return ToFloat( szString.c_str() ); }
double ToDouble( const char *pszString );
inline double ToDouble( const string &szString ) { return ToDouble( szString.c_str() ); }

// является ли строка представлением числа
inline bool IsBinDigit( const char cChar ) { return ( (cChar == '0') && (cChar == '1') ); }
inline bool IsOctDigit( const char cChar ) { return ( (cChar >= '0') && (cChar <= '7') ); }
inline bool IsDecDigit( const char cChar ) { return ( (cChar >= '0') && (cChar <= '9') ); }
inline bool IsHexDigit( const char cChar ) { return ( (cChar >= '0') && (cChar <= '9') ) || ( (cChar >= 'a') && (cChar <= 'f') ) || ( (cChar >= 'A') && (cChar <= 'F') ); }
inline bool IsSign( const char cChar ) { return ( (cChar == '-') || (cChar == '+') ); }
MISC_EXPORT bool IsDecNumber( const string &szString );
bool IsOctNumber( const string &szString );
MISC_EXPORT bool IsHexNumber( const string &szString );

// перевод string => bin и обратно
// NOTE: BinToString() doesn't attach '\0' at the end!!!
MISC_EXPORT void* StringToBin( const char *pszData, void *pBuffer, int *pnSize );
MISC_EXPORT const char* BinToString( const void *pData, int nSize, char *pszBuffer );
__forceinline char HalfByteToHexSymbol( const unsigned char chr ) { return chr >= 10 ? 'a' + ( chr - 10 ) : '0' + chr; }
__forceinline unsigned char HexSymbolToHalfByte( const char chr )
{
	if ( chr >= 'a' )
		return chr - 'a' + 10;
	else if ( chr >= 'A' ) 
		return chr - 'A' + 10;
	else
		return chr - '0';
}

// перевод UNICODE => UTF-8 и обратно
MISC_EXPORT void UnicodeToUTF8( string *pRes, const wstring &szString );
MISC_EXPORT void UTF8ToUnicode( wstring *pRes, const string &szString );

// перевод MBCS => Unicode и обратно
MISC_EXPORT void SetCodePage( const int nCodePage );
MISC_EXPORT void ToMBCS( string *pRes, const wstring &szSrc );
inline string ToMBCS( const wstring &szSrc ) { string szDst; ToMBCS( &szDst, szSrc ); return szDst; }
MISC_EXPORT void ToUnicode( wstring *pRes, const string &szSrc );
inline wstring ToUnicode( const string &szSrc ) { wstring szDst; ToUnicode( &szDst, szSrc ); return szDst; }

// перевод MBCS => UTF-8 и обратно
MISC_EXPORT void UTF8ToMBCS( string *pRes, const string &szSrc );
MISC_EXPORT void MBCSToUTF8( string *pRes, const string &szSrc );

// GUID => string conversion
MISC_EXPORT void GUID2String( string *pString, const GUID &guid );
MISC_EXPORT void String2GUID( const string &szString, GUID *pGuid );

template <class TChar>
void ReplaceAllChars( basic_string<TChar> *pString, const TChar tFrom, const TChar tTo )
{
	for ( basic_string<TChar>::iterator it = pString->begin(); it != pString->end(); ++it )
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
	static __forceinline bool IsOpen( const TChar chr )
	{
		return ( chr == TChar('\"') );
	}
	static __forceinline bool IsClose( const TChar chr )
	{
		return ( chr == TChar('\"') );
	}
	static __forceinline TChar GetClose( const TChar chr )
	{
		return chr == TChar('\"') ? TChar('\"') : TChar( -1 );
	}
};


template <class TChar>
struct SBracketsTest
{
	static __forceinline bool IsOpen( const TChar chr )
	{
		return ( chr == TChar('(') ) || ( chr == TChar('[') ) || ( chr == TChar('{') ) || ( chr == TChar('<') );
	}
	static __forceinline bool IsClose( const TChar chr )
	{
		return ( chr == TChar(')') ) || ( chr == TChar(']') ) || ( chr == TChar('}') ) || ( chr == TChar('>') );
	}
	static __forceinline TChar GetClose( const TChar chr )
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
	static __forceinline bool IsOpen( const TChar chr )
	{
		return ( chr == TChar('(') ) || ( chr == TChar('[') ) || ( chr == TChar('{') ) || ( chr == TChar('<') ) || ( chr == TChar('\"') ) || ( chr == TChar('\'') );
	}
	static __forceinline bool IsClose( const char chr )
	{
		return ( chr == TChar(')') ) || ( chr == TChar(']') ) || ( chr == TChar('}') ) || ( chr == TChar('>') ) || ( chr == TChar('\"') ) || ( chr == TChar('\'') );
	}
	static __forceinline TChar GetClose( const char chr )
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
	vector<TChar> stc;										// close brackets stack
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

template <class TChar, class TStorage = basic_string<TChar>, class TSeparator = CCharSeparator<TChar> >
class CStringIterator
{
	TStorage szInput;											// input string
	int nPrevPos;													// previous found position
	int nCurrPos;													// current found position
	TSeparator separator;									// separator functional
public:
	CStringIterator( const TChar *pszString, const TChar cSeparator )
		: szInput( pszString ), nPrevPos( -1 ), nCurrPos( -1 ), separator( cSeparator ) { Next(); }
	CStringIterator( const basic_string<TChar> &szString, const TChar cSeparator )
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
	basic_string<TChar> Get() const 
	{ 
		return szInput.substr(nPrevPos, nCurrPos - nPrevPos); 
	}
	void Get( basic_string<TChar> *pString )
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
		const string &szSample;

		SSearchStr( const char *_pszBegin, const int _nLength, const string &_szSample )
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
		vector<int> sizes;

		SPrefixesArray( const int _nSampleSize ) : nSampleSize( _nSampleSize ), sizes( nSampleSize + 2, 0 ) { }
		int& operator[]( const int nIndex )
		{
			return nIndex <= nSampleSize ? sizes[nIndex] : sizes[nSampleSize + 1];
		}
	};
}
template<class T>
void FastSearch( const char *pszBegin, const int nSize, const string &szSample, vector<int> *pFoundEntriesPos, T charsComparer )
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
int FastSerachFirst( const char *pszBegin, const string &szSample, T charsComparer )
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
int SerachFirst( const char *pszBegin, const string &szSample, T charsComparer )
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


