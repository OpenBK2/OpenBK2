#pragma once

#include "System_export.h"

#include <charconv>
#include <cstring>
#include <cwchar>
#include <system_error>


// func is called on var set or cmd call
#define REGISTER_CMD( var, func ) NGlobal::RegisterCmd( var, func, 0 );
#define REGISTER_VAR( var, func, defval, save ) NGlobal::RegisterVar( var, func, 0, defval, save );
#define REGISTER_VAR_EX( var, func, cont, defval, save ) NGlobal::RegisterVar( var, func, cont, defval, save );

enum EStorageClass
{
	STORAGE_DONT_CARE = -1,
	STORAGE_NONE = 0,
	STORAGE_GLOBAL = 1,
	STORAGE_USER = 2,
	STORAGE_SAVE = 3,
};

namespace NGlobal
{

class CValue
{
	ZDATA
	float fVal;
	std::wstring szVal;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fVal); f.Add(3,&szVal); return 0; }
private:
	//
	void SetVal( const char *pszString )
	{
		const int nLen = strlen( pszString );
		szVal.resize( nLen );
		copy( pszString, pszString + nLen, szVal.begin() );
	}
public:
	// Parse the leading number out of a string the way atof did, but without
	// the locale dependency and without exceptions. from_chars is the only
	// standard parser specified to ignore LC_NUMERIC. Trailing junk is not an
	// error, so "1024x768" still gives 1024; anything that does not start with
	// a number gives 0.
	static float ParseFloat( const char *pszVal, size_t nLen )
	{
		// from_chars does not skip leading whitespace, atof did
		while ( nLen > 0 && ( *pszVal == ' ' || *pszVal == '\t' ) )
		{
			++pszVal;
			--nLen;
		}
		float fRes = 0.0f;
		const std::from_chars_result res = std::from_chars( pszVal, pszVal + nLen, fRes );
		if ( res.ec != std::errc() )
		{
			return 0.0f;
		}
		return fRes;
	}
	// numbers are ASCII, so narrowing is enough to reuse the same parser
	static float ParseFloat( const wchar_t *pszVal, size_t nLen )
	{
		std::string szNarrow;
		szNarrow.reserve( nLen );
		for ( size_t i = 0; i < nLen; ++i )
		{
			szNarrow += ( pszVal[i] > 0 && pszVal[i] < 0x80 ) ? char( pszVal[i] ) : '?';
		}
		return ParseFloat( szNarrow.c_str(), szNarrow.size() );
	}

	CValue() : fVal( 0 ) {}
	CValue( float _fVal ) : fVal( _fVal ) { SetVal( StrFmt("%g", _fVal) ); }
	CValue( int _n ) : fVal( _n ) { SetVal( StrFmt( "%g", fVal ) ); }
	CValue( const std::wstring &_szVal )	: fVal( ParseFloat( _szVal.c_str(), _szVal.size() ) ), szVal( _szVal ) {}
	CValue( const wchar_t *pszVal )	: fVal( ParseFloat( pszVal, wcslen( pszVal ) ) ), szVal(pszVal) {}
	CValue( const std::string &_szVal )	: fVal( ParseFloat( _szVal.c_str(), _szVal.size() ) ) { SetVal( _szVal.c_str() ); }
	CValue( const char *pszVal )	: fVal( ParseFloat( pszVal, strlen( pszVal ) ) ) { SetVal( pszVal ); }

	float GetFloat() const { return fVal; }
	const std::wstring& GetString() const { return szVal; }

	operator float() const { return fVal; }
	operator std::wstring() const { return szVal; }
};

typedef void (*VarHandler)( const std::string &szID, const CValue &sValue, void *pContext );
typedef void (*CmdHandler)( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext );

SYSTEM_EXPORT int RegisterCmd( const std::string &szID, CmdHandler pHandler, void *pContext );
SYSTEM_EXPORT int RegisterVar( const std::string &szID, VarHandler pHandler, void *pContext, const CValue &sValue, EStorageClass storage );
void UnregisterCmd( const std::string &szID, int nID );
void UnregisterVar( const std::string &szID, int nID );
SYSTEM_EXPORT void RemoveVar( const std::string &szID );

SYSTEM_EXPORT const CValue &GetVar( const std::string &szName, const CValue &sDefault = CValue() );
SYSTEM_EXPORT void SetVar( const std::string &szName, const CValue &sValue, EStorageClass storage = STORAGE_DONT_CARE );
SYSTEM_EXPORT void GetIDList( std::vector<std::string> *pList );
SYSTEM_EXPORT void GetVarsByClass( std::vector< std::pair<std::string, CValue> > *pList, EStorageClass eStorageClass );

SYSTEM_EXPORT void ProcessCommand( const std::wstring &szCmd );
SYSTEM_EXPORT void LoadConfig( const std::string &szFileName, EStorageClass _newVarStorage = STORAGE_DONT_CARE );
SYSTEM_EXPORT void ResetVarsToDefault( EStorageClass storage );
SYSTEM_EXPORT void SaveAllVars( const std::string &szGlobalName, const std::string &szUserName );

}

