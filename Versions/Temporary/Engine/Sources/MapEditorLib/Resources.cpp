#include "stdafx.h"

#include "Resources.h"

#include <vector>

namespace
{
	std::vector<HINSTANCE>& Modules()
	{
		static std::vector<HINSTANCE> modules;
		return modules;
	}
}


namespace NResources
{
	HINSTANCE FindModule( LPCSTR pszName, LPCSTR pszType )
	{
		const HINSTANCE hExecutable = ::GetModuleHandleA( nullptr );
		if ( ::FindResourceA( hExecutable, pszName, pszType ) != 0 )
		{
			return hExecutable;
		}
		for ( const HINSTANCE hModule : Modules() )
		{
			if ( ::FindResourceA( hModule, pszName, pszType ) != 0 )
			{
				return hModule;
			}
		}
		return 0;
	}


	void RegisterModule( HINSTANCE hModule )
	{
		if ( hModule != 0 )
		{
			Modules().push_back( hModule );
		}
	}


	bool GetString( unsigned nID, std::string *pszText )
	{
		if ( pszText == nullptr )
		{
			return false;
		}
		pszText->clear();
		// Not FindModule: strings are stored sixteen to a block, and two modules
		// can both have a block without both having the string, so each module
		// is asked for the string itself, the executable first.
		std::vector<HINSTANCE> candidates( 1, ::GetModuleHandleA( nullptr ) );
		candidates.insert( candidates.end(), Modules().begin(), Modules().end() );
		// With a zero length, LoadStringW answers a pointer to the string in the
		// resource itself, which is not terminated, and its length.
		const wchar_t *pszWide = nullptr;
		int nLength = 0;
		for ( const HINSTANCE hModule : candidates )
		{
			nLength = ::LoadStringW( hModule, nID, reinterpret_cast<LPWSTR>( &pszWide ), 0 );
			if ( ( nLength > 0 ) && ( pszWide != nullptr ) )
			{
				break;
			}
		}
		if ( ( nLength <= 0 ) || ( pszWide == nullptr ) )
		{
			return false;
		}
		const int nBytes = ::WideCharToMultiByte( CP_UTF8, 0, pszWide, nLength, nullptr, 0, nullptr, nullptr );
		pszText->resize( nBytes );
		::WideCharToMultiByte( CP_UTF8, 0, pszWide, nLength, &( *pszText )[0], nBytes, nullptr, nullptr );
		return true;
	}


	std::string GetString( unsigned nID )
	{
		std::string szText;
		GetString( nID, &szText );
		return szText;
	}
}
