#include "stdafx.h"

#include "Clipboard.h"

// The third translation unit in MapEditorLib that sees a wx header, and safe
// for the same reason the other two are: nothing below reaches the game
// database, so losing windows.h's A/W macros costs nothing here.
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/string.h>

namespace NClipboard
{
	bool SetText( const std::string &rszText )
	{
		if ( wxTheClipboard == nullptr )
		{
			return false;
		}
		// Open/Close rather than wxClipboardLocker, to match the wx views here
		// and because the result of the open is wanted.
		if ( !wxTheClipboard->Open() )
		{
			return false;
		}
		// Clear returns void, unlike Open and SetData, so clearing succeeds as
		// soon as the clipboard is open.
		bool bDone = true;
		if ( rszText.empty() )
		{
			wxTheClipboard->Clear();
		}
		else
		{
			// wx takes the data object, as the Win32 clipboard took the HGLOBAL.
			bDone = wxTheClipboard->SetData(
				new wxTextDataObject( wxString::FromUTF8( rszText.c_str() ) ) );
		}
		wxTheClipboard->Close();
		return bDone;
	}
}
