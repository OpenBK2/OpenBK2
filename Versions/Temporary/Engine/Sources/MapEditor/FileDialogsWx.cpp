#include "stdafx.h"

#include "FileDialogs.h"

#include "MapEditorLib/WxModal.h"

#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msw/private.h>

// The pickers in wx. Both are native dialogs, so what makes them modal over the
// editor is having the frame as their parent: wx gives the system dialog that
// window as its owner.

namespace
{
	// The wx frame pOwner belongs to, or null.
	wxWindow* OwnerFrame( IWidget *pOwner )
	{
		const HWND hwndFrame = NWxModal::FindOwnerFrame( pOwner );
		return ( hwndFrame != 0 ) ? wxFindWinFromHandle( hwndFrame ) : nullptr;
	}


	wxString FromUTF8( const std::string &rszText )
	{
		return wxString::FromUTF8( rszText.c_str(), rszText.size() );
	}


	std::string ToUTF8( const wxString &rText )
	{
		return std::string( rText.utf8_str() );
	}
}


namespace NFileDialog
{
	bool OpenFile( IWidget *pOwner, const std::string &rszTitle, const std::string &rszFilter,
								 const std::string &rszInitialDir, std::string *pszPath )
	{
		if ( pszPath == nullptr )
		{
			return false;
		}
		// MFC's filter ends in "||"; wx's has no terminator.
		std::string szFilter = rszFilter;
		while ( !szFilter.empty() && ( szFilter.back() == '|' ) )
		{
			szFilter.pop_back();
		}
		wxFileDialog dialog( OwnerFrame( pOwner ), FromUTF8( rszTitle ), FromUTF8( rszInitialDir ), wxString(),
												 FromUTF8( szFilter ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );
		if ( dialog.ShowModal() != wxID_OK )
		{
			return false;
		}
		( *pszPath ) = ToUTF8( dialog.GetPath() );
		return true;
	}


	bool ChooseFolder( IWidget *pOwner, const std::string &rszTitle, const std::string &rszInitialDir,
										 std::string *pszPath )
	{
		if ( pszPath == nullptr )
		{
			return false;
		}
		wxDirDialog dialog( OwnerFrame( pOwner ), FromUTF8( rszTitle ), FromUTF8( rszInitialDir ),
												wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );
		if ( dialog.ShowModal() != wxID_OK )
		{
			return false;
		}
		( *pszPath ) = ToUTF8( dialog.GetPath() );
		return true;
	}
}
