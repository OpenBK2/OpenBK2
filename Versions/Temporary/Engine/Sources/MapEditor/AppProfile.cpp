#include "stdafx.h"

#include "AppProfile.h"

#include "ResourceDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Resources.h"
#include "MapEditorLib/Interface_UserData.h"

#include <wx/config.h>

namespace
{
	wxString FromNarrow( const std::string &rszText )
	{
		return wxString::FromUTF8( rszText.c_str(), rszText.size() );
	}


	// One store for the process.
	//
	// wxConfig is wxRegConfig here, which puts the vendor and application names
	// under HKCU\Software exactly where CWinApp's profile was, and wxFileConfig
	// elsewhere, which puts them in the user's configuration directory.
	// wxCONFIG_USE_SUBDIR gives that a directory of its own rather than a
	// dotfile; it means nothing to the registry.
	//
	// Made on first use, because the application title comes from the user data
	// singleton and the vendor from the string table, and neither exists until
	// the editor has started.
	wxConfigBase& Profile()
	{
		static wxConfig config( FromNarrow( Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle ),
														FromNarrow( NResources::GetString( IDS_REGISTRY_PATH ) ),
														wxEmptyString, wxEmptyString,
														wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR );
		return config;
	}


	// MFC kept one more level under the application's key, AFX_IDS_APP_TITLE,
	// with the sections under that. Keeping it is what makes an entry land on
	// the value an earlier build wrote.
	wxString ProfileRoot()
	{
		return "/" + FromNarrow( NResources::GetString( AFX_IDS_APP_TITLE ) );
	}


	wxString EntryPath( const std::string &rszSection, const std::string &rszEntry )
	{
		wxString path = ProfileRoot();
		if ( !rszSection.empty() )
		{
			path += "/" + FromNarrow( rszSection );
		}
		return path + "/" + FromNarrow( rszEntry );
	}
}


namespace NAppProfile
{
	std::string GetString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszDefault )
	{
		wxString value;
		if ( !Profile().Read( EntryPath( rszSection, rszEntry ), &value ) )
		{
			return rszDefault;
		}
		return std::string( value.utf8_str() );
	}


	bool WriteString( const std::string &rszSection, const std::string &rszEntry, const std::string &rszValue )
	{
		if ( !Profile().Write( EntryPath( rszSection, rszEntry ), FromNarrow( rszValue ) ) )
		{
			return false;
		}
		// wxFileConfig holds the file in memory and writes it when it is
		// destroyed, which for a static is after the editor has gone. These
		// writes are rare -- a layout, a window placement, the recent list on
		// exit -- so each one goes out now rather than being lost to a crash.
		return Profile().Flush();
	}


	void DeleteAll()
	{
		Profile().DeleteGroup( ProfileRoot() );
		Profile().Flush();
	}
}
