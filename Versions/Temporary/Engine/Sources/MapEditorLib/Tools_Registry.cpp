#include "stdafx.h"

#include "Tools_Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CRegistrySection::CRegistrySection( HKEY hKey, REGSAM samDesired, LPCTSTR pszRegistrySection )
{
  LONG eResult = ERROR_SUCCESS;
  DWORD dwDisposition;
  eResult = ::RegCreateKeyEx( hKey,
														 pszRegistrySection,
														 0,
														 0,
														 REG_OPTION_NON_VOLATILE,
														 samDesired,
														 0,
														 &hRegistrySection,
														 &dwDisposition );
  if ( eResult != ERROR_SUCCESS )
  {
		hRegistrySection = 0;
	}
}


CRegistrySection::~CRegistrySection()
{
  if ( hRegistrySection != 0 )
	{
		::RegCloseKey( hRegistrySection );
		hRegistrySection = 0;
	}
}


LONG CRegistrySection::LoadString( LPCTSTR pszRegistryKey, string *pszLoadValue, const string &rszDefaultValue ) const
{
  if ( ( pszLoadValue != 0 ) && ( hRegistrySection != 0 ) )
	{
		( *pszLoadValue ) = rszDefaultValue;

		DWORD dwLoadValueType;
		DWORD dwLoadValueLength = 0xFFF;
		BYTE pBuffer[0xFFF];
		LONG eResult = ERROR_SUCCESS;
		eResult = ::RegQueryValueEx( hRegistrySection,
																pszRegistryKey,
																0,
																&dwLoadValueType,
																pBuffer,
																&dwLoadValueLength );
		if( ( eResult != ERROR_SUCCESS ) || ( dwLoadValueType != REG_SZ ) )
		{
			( *pszLoadValue ) = rszDefaultValue;
		}
		else
		{
			( *pszLoadValue ) = string( reinterpret_cast<LPCTSTR>( pBuffer ) );
		}
		return eResult;
	}
	else
	{
		return ERROR_CANTOPEN;
	}
}


LONG CRegistrySection::SaveString( LPCTSTR pszRegistryKey, const string &szSaveValue ) const
{
	return ::RegSetValueEx( hRegistrySection,
													pszRegistryKey,
													0,
													REG_SZ,
													reinterpret_cast<const BYTE*>( szSaveValue.c_str() ),
													szSaveValue.size() + 1 );
}


LONG CRegistrySection::LoadDWORD( LPCTSTR pszRegistryKey, DWORD *pdwLoadValue, DWORD dwDefaultValue ) const
{
  if ( ( pdwLoadValue != 0 ) && ( hRegistrySection != 0 ) )
	{
		( *pdwLoadValue ) = dwDefaultValue;

		DWORD dwLoadValueType;
		DWORD dwLoadValueLength = 0xFFF;
		BYTE pBuffer[0xFFF];
		LONG eResult = ERROR_SUCCESS;
		eResult = ::RegQueryValueEx( hRegistrySection,
																pszRegistryKey,
																0,
																&dwLoadValueType,
																pBuffer,
																&dwLoadValueLength );
		if( ( eResult != ERROR_SUCCESS ) || ( dwLoadValueType != REG_DWORD ) || ( dwLoadValueLength < sizeof( DWORD ) ) )
		{
			( *pdwLoadValue ) = dwDefaultValue;
		}
		else
		{
			( *pdwLoadValue ) = *( reinterpret_cast<DWORD*>( pBuffer ) );
		}
		return eResult;
	}
	else
	{
		return ERROR_CANTOPEN;
	}
}


LONG CRegistrySection::SaveDWORD( LPCTSTR pszRegistryKey, DWORD dwSaveValue ) const
{
	return ::RegSetValueEx( hRegistrySection,
													pszRegistryKey,
													0,
													REG_DWORD,
													reinterpret_cast<const BYTE*>( &dwSaveValue ),
													sizeof( DWORD ) );    	
}

// basement storage  

