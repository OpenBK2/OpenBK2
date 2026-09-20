#pragma once

#include <string>

// Where the editor keeps what it remembers between runs: the window placement,
// the recent maps, the saved layouts, the tables that were open.
//
// This was CWinApp's SetRegistryKey, GetProfileString and WriteProfileString,
// then the same registry calls written out by hand once MFC went, and now
// wxConfig, which is the registry on Windows and a file elsewhere.
//
// On Windows the key is still the one MFC used --
// HKCU\Software\<IDS_REGISTRY_PATH>\<application title>\<AFX_IDS_APP_TITLE>,
// "Software\Nival Interactive\Blitzkrieg 2 Editor\Common Editor" for this
// editor -- so nothing saved by an earlier build moves or is lost.
//
// Elsewhere wxFileConfig writes the same entries to a file in the user's
// configuration directory, which is what XDG_CONFIG_HOME names.
//
// Entries are text. Everything stored here was already text: the numbers and
// rectangles the main frame keeps are formatted and parsed by their callers,
// as they were when CRegistrySection did it, "хранится в виде строки для
// наглядности".
namespace NAppProfile
{
	// The entry's text, or rszDefault when it is not there. An empty section
	// means the profile's own level, where the format version lives.
	std::string GetString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszDefault );

	// Creates the section if need be. False if the store refused.
	bool WriteString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszValue );

	// Forget everything the editor has stored: the -reg switch, and a format
	// version this build does not understand. This is what SHDeleteKey over
	// the profile key did.
	void DeleteAll();
}
