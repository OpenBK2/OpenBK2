#include "stdafx.h"

#include "AppProfile.h"

#include "ResourceDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Resources.h"
#include "MapEditorLib/Interface_UserData.h"

#include <vector>

namespace NAppProfile
{
	std::string GetRootKey()
	{
		return "Software\\" + NResources::GetString( IDS_REGISTRY_PATH ) + "\\" +
					 Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle + "\\" +
					 NResources::GetString( AFX_IDS_APP_TITLE );
	}


	std::string GetString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszDefault )
	{
		const std::string szKey = GetRootKey() + "\\" + rszSection;
		HKEY hKey = 0;
		if ( ::RegOpenKeyExA( HKEY_CURRENT_USER, szKey.c_str(), 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
		{
			return rszDefault;
		}
		std::string szResult = rszDefault;
		DWORD nType = 0;
		DWORD nBytes = 0;
		if ( ( ::RegQueryValueExA( hKey, rszEntry.c_str(), nullptr, &nType, nullptr, &nBytes ) == ERROR_SUCCESS ) &&
				 ( nType == REG_SZ ) )
		{
			std::vector<char> buffer( nBytes + 1, '\0' );
			if ( ::RegQueryValueExA( hKey, rszEntry.c_str(), nullptr, &nType, reinterpret_cast<BYTE*>( buffer.data() ), &nBytes ) == ERROR_SUCCESS )
			{
				// The stored terminator, if there is one, is not part of the text.
				szResult.assign( buffer.data() );
			}
		}
		::RegCloseKey( hKey );
		return szResult;
	}


	bool WriteString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszValue )
	{
		const std::string szKey = GetRootKey() + "\\" + rszSection;
		HKEY hKey = 0;
		if ( ::RegCreateKeyExA( HKEY_CURRENT_USER, szKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
													 &hKey, nullptr ) != ERROR_SUCCESS )
		{
			return false;
		}
		const LONG nResult = ::RegSetValueExA( hKey, rszEntry.c_str(), 0, REG_SZ,
																					 reinterpret_cast<const BYTE*>( rszValue.c_str() ),
																					 static_cast<DWORD>( rszValue.size() + 1 ) );
		::RegCloseKey( hKey );
		return ( nResult == ERROR_SUCCESS );
	}
}
