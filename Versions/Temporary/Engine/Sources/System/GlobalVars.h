#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "System_export.h"

#include <charconv>
#include <cstring>
#include <cwchar>
#include <system_error>
#include <type_traits>

#include <fmt/format.h>

// func is called on var set or cmd call
// Console commands never carried a context: RegisterCmd hard-coded it to 0
// and nothing here registers one with a real one, so CmdHandler has no void *
// parameter rather than a null one. Same reasoning as REGISTER_VAR below.
#define REGISTER_CMD( var, func ) NGlobal::RegisterCmd( var, func );
// No context, so no handler that takes one. This used to hand the handler a
// hard-coded 0 as its void *, which nothing dereferenced only because none of
// the handlers it is used with looks at the argument. A handler that did would
// have written through a null pointer on the first setvar, and that is both an
// easy mistake to make while editing (the two macros differ by a suffix) and
// undefined behaviour the optimiser is entitled to reason from rather than a
// crash you can rely on. VarHandlerNoContext has no such parameter to pass, so
// the null does not exist on this path any more and the mistake is a compile
// error: a handler taking a void * will not convert.
#define REGISTER_VAR( var, func, defval, save ) NGlobal::RegisterVar( var, func, defval, save );
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
	float fVal = 0.0f;
	std::wstring szVal;

public:
	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &fVal );
		f.Add( 3, &szVal );
		return 0;
	}

private:
	//

	void SetVal( std::string_view s )
	{
		szVal.resize( s.length() );
		copy( s.data(), s.data() + s.length(), szVal.begin() );
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

	CValue()
		: fVal( 0 )
	{
	}

	CValue( float _fVal )
		: fVal( _fVal )
	{
		SetVal( fmt::format( "{:g}", _fVal ).c_str() );
	}

	CValue( int _n )
		: fVal( _n )
	{
		SetVal( fmt::format( "{:g}", fVal ).c_str() );
	}

	CValue( const std::wstring &_szVal )
		: fVal( ParseFloat( _szVal.c_str(), _szVal.size() ) )
		, szVal( _szVal )
	{
	}

	CValue( const wchar_t *pszVal )
		: fVal( ParseFloat( pszVal, wcslen( pszVal ) ) )
		, szVal( pszVal )
	{
	}

	CValue( const std::string &_szVal )
		: fVal( ParseFloat( _szVal.c_str(), _szVal.size() ) )
	{
		SetVal( _szVal.c_str() );
	}

	CValue( const char *pszVal )
		: fVal( ParseFloat( pszVal, strlen( pszVal ) ) )
	{
		SetVal( pszVal );
	}

	float GetFloat() const
	{
		return fVal;
	}

	const std::wstring &GetString() const
	{
		return szVal;
	}

	operator float() const
	{
		return fVal;
	}

	operator std::wstring() const
	{
		return szVal;
	}
};

typedef void ( *VarHandler )( const std::string &szID, const CValue &sValue, void *pContext );
typedef void ( *CmdHandler )( const std::string &szID, const std::vector<std::wstring> &paramsSet );

// What REGISTER_VAR takes: a handler for a variable registered without a
// context, which is either a value only read back through GetVar or a handler
// that keeps its own state. It has no void * parameter rather than a null one.
typedef void ( *VarHandlerNoContext )( const std::string &szID, const CValue &sValue );

// The five standard variable handlers, which store the parsed value through
// the void * a registration hands them. Which one a variable needs is decided
// by that variable's type, and nothing used to check the two agreed: eight
// registrations in this tree had them crossed, three of those live on every
// start, and the symptom was a variable holding the other type's bit pattern
// rather than anything that looks like a bug at the call site.
//
// So a call site does not name these functions. It names the matching tag
// object below, which carries the type the function writes, and RegisterVar
// compares that against the type of the variable before erasing it to void *.
SYSTEM_EXPORT void VarBoolHandlerFn( const std::string &szID, const CValue &sValue, void *pContext );
SYSTEM_EXPORT void VarIntHandlerFn( const std::string &szID, const CValue &sValue, void *pContext );
SYSTEM_EXPORT void VarFloatHandlerFn( const std::string &szID, const CValue &sValue, void *pContext );
SYSTEM_EXPORT void VarWStrHandlerFn( const std::string &szID, const CValue &sValue, void *pContext );
SYSTEM_EXPORT void VarStrHandlerFn( const std::string &szID, const CValue &sValue, void *pContext );

// An empty tag whose only content is the type its handler writes.
template <class T>
struct TVarBinding
{
	typedef T TBound;
};

// These keep the names the 380 existing call sites already pass, so adopting
// the check changed none of them.
inline constexpr TVarBinding<bool> VarBoolHandler{};
inline constexpr TVarBinding<int> VarIntHandler{};
inline constexpr TVarBinding<float> VarFloatHandler{};
inline constexpr TVarBinding<std::wstring> VarWStrHandler{};
inline constexpr TVarBinding<std::string> VarStrHandler{};

inline VarHandler GetVarHandlerFn( TVarBinding<bool> )
{
	return &VarBoolHandlerFn;
}

inline VarHandler GetVarHandlerFn( TVarBinding<int> )
{
	return &VarIntHandlerFn;
}

inline VarHandler GetVarHandlerFn( TVarBinding<float> )
{
	return &VarFloatHandlerFn;
}

inline VarHandler GetVarHandlerFn( TVarBinding<std::wstring> )
{
	return &VarWStrHandlerFn;
}

inline VarHandler GetVarHandlerFn( TVarBinding<std::string> )
{
	return &VarStrHandlerFn;
}

// Which variable types a binding may be pointed at: its own, and nothing else.
template <class TBound, class TVar, class = void>
struct TVarBindingAccepts : std::is_same<TBound, TVar>
{
};

// The one widening, and it is the language's rather than a convenience:
// an object may be accessed through the signed or unsigned variant of its own
// type, so the int handler may write a uint32_t. Three registrations rely on
// it, two of them NTimer::STime. It stays narrow on purpose, since same size
// is not the same thing: DWORD is unsigned long on MSVC, which is a different
// type from int however many bytes it has, and gfx_fsaa was exactly that case.
template <class TBound, class TVar>
struct TVarBindingAccepts<
		TBound,
		TVar,
		typename std::enable_if<std::is_integral<TBound>::value && std::is_integral<TVar>::value &&
                            !std::is_same<TBound, bool>::value && !std::is_same<TVar, bool>::value>::type>
	: std::is_same<typename std::make_unsigned<TBound>::type, typename std::make_unsigned<TVar>::type>
{
};

SYSTEM_EXPORT int RegisterCmd( const std::string &szID, CmdHandler pHandler );
SYSTEM_EXPORT int RegisterVar(
		const std::string &szID, VarHandler pHandler, void *pContext, const CValue &sValue, EStorageClass storage );
// The REGISTER_VAR form. A null handler is allowed and means the variable only
// stores its value for GetVar to read, which is what eight registrations here
// do; what is not allowed is a handler expecting a context, because there is
// none to give it.
SYSTEM_EXPORT int
RegisterVar( const std::string &szID, VarHandlerNoContext pHandler, const CValue &sValue, EStorageClass storage );

// The overload REGISTER_VAR_EX resolves to. The context is still a TVar * here,
// one call before it becomes a void *, which is the whole reason the check can
// exist at all: the type has not been erased yet.
template <class TBound, class TVar>
int RegisterVar(
		const std::string &szID, TVarBinding<TBound> binding, TVar *pContext, const CValue &sValue, EStorageClass storage )
{
	static_assert( TVarBindingAccepts<TBound, TVar>::value,
	               "this console variable is registered with a handler for a different type. The "
	               "handler stores through the pointer it is given, so the variable would hold the "
	               "other type's bit pattern instead of the value. Name the handler that matches "
	               "the variable: VarBoolHandler, VarIntHandler, VarFloatHandler, VarStrHandler or "
	               "VarWStrHandler." );
	return RegisterVar( szID, GetVarHandlerFn( binding ), static_cast<void *>( pContext ), sValue, storage );
}

void UnregisterCmd( const std::string &szID, int nID );
void UnregisterVar( const std::string &szID, int nID );
SYSTEM_EXPORT void RemoveVar( const std::string &szID );

SYSTEM_EXPORT const CValue &GetVar( const std::string &szName, const CValue &sDefault = CValue() );
SYSTEM_EXPORT void SetVar( const std::string &szName, const CValue &sValue, EStorageClass storage = STORAGE_DONT_CARE );
SYSTEM_EXPORT void GetIDList( std::vector<std::string> *pList );
SYSTEM_EXPORT void GetVarsByClass( std::vector<std::pair<std::string, CValue>> *pList, EStorageClass eStorageClass );

SYSTEM_EXPORT void ProcessCommand( const std::wstring &szCmd );
SYSTEM_EXPORT void LoadConfig( const std::string &szFileName, EStorageClass _newVarStorage = STORAGE_DONT_CARE );
SYSTEM_EXPORT void ResetVarsToDefault( EStorageClass storage );
SYSTEM_EXPORT void SaveAllVars( const std::string &szGlobalName, const std::string &szUserName );

} // namespace NGlobal
