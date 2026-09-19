#pragma once

#include <string>

// The application's registry profile: what CWinApp's SetRegistryKey,
// GetProfileString and WriteProfileString were, without MFC.
//
// The key is the one MFC used, so nothing saved before moves:
// HKCU\Software\<IDS_REGISTRY_PATH>\<application title>\<AFX_IDS_APP_TITLE>,
// which is "Software\Nival Interactive\Blitzkrieg 2 Editor\Common Editor" for
// this editor. Sections are subkeys of it and entries are REG_SZ values, as
// MFC stored them.
namespace NAppProfile
{
	// The profile's key under HKEY_CURRENT_USER.
	std::string GetRootKey();
	// The entry's text, or rszDefault when the entry or its section is not
	// there, or is not a string.
	std::string GetString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszDefault );
	// Creates the section if need be. False if the registry refused.
	bool WriteString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszValue );
}
