#include "stdafx.h"

#include "Tools_Registry.h"

#include <cstdint>

CRegistrySection::CRegistrySection( HKEY hKey, REGSAM samDesired, LPCTSTR pszRegistrySection )
{
  int32_t eResult = ERROR_SUCCESS;
  uint32_t dwDisposition;
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


int32_t CRegistrySection::LoadString( LPCTSTR pszRegistryKey, string *pszLoadValue, const string &rszDefaultValue ) const
{
  if ( ( pszLoadValue != 0 ) && ( hRegistrySection != 0 ) )
	{
		( *pszLoadValue ) = rszDefaultValue;

		uint32_t dwLoadValueType;
		uint32_t dwLoadValueLength = 0xFFF;
		uint8_t pBuffer[0xFFF];
		int32_t eResult = ERROR_SUCCESS;
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


int32_t CRegistrySection::SaveString( LPCTSTR pszRegistryKey, const string &szSaveValue ) const
{
	return ::RegSetValueEx( hRegistrySection,
													pszRegistryKey,
													0,
													REG_SZ,
													reinterpret_cast<const uint8_t*>( szSaveValue.c_str() ),
													szSaveValue.size() + 1 );
}


int32_t CRegistrySection::LoadDWORD( LPCTSTR pszRegistryKey, uint32_t *pdwLoadValue, uint32_t dwDefaultValue ) const
{
  if ( ( pdwLoadValue != 0 ) && ( hRegistrySection != 0 ) )
	{
		( *pdwLoadValue ) = dwDefaultValue;

		uint32_t dwLoadValueType;
		uint32_t dwLoadValueLength = 0xFFF;
		uint8_t pBuffer[0xFFF];
		int32_t eResult = ERROR_SUCCESS;
		eResult = ::RegQueryValueEx( hRegistrySection,
																pszRegistryKey,
																0,
																&dwLoadValueType,
																pBuffer,
																&dwLoadValueLength );
		if( ( eResult != ERROR_SUCCESS ) || ( dwLoadValueType != REG_DWORD ) || ( dwLoadValueLength < sizeof( uint32_t ) ) )
		{
			( *pdwLoadValue ) = dwDefaultValue;
		}
		else
		{
			( *pdwLoadValue ) = *( reinterpret_cast<uint32_t*>( pBuffer ) );
		}
		return eResult;
	}
	else
	{
		return ERROR_CANTOPEN;
	}
}


int32_t CRegistrySection::SaveDWORD( LPCTSTR pszRegistryKey, uint32_t dwSaveValue ) const
{
	return ::RegSetValueEx( hRegistrySection,
													pszRegistryKey,
													0,
													REG_DWORD,
													reinterpret_cast<const uint8_t*>( &dwSaveValue ),
													sizeof( uint32_t ) );
}

// basement storage  


