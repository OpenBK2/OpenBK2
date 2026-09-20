#include "stdafx.h"

#include "MessageBoxes.h"

#include "Interface_MainFrame.h"
#include "Interface_UserData.h"
#include "WxWidget.h"

// The only translation unit in MapEditorLib that sees a wx header, and the
// reason the declarations in MessageBoxes.h name no toolkit. Including wx here
// is safe because nothing below reaches the game database: wx undefines
// windows.h's A/W macros, and a TU that calls NDb::Get<>, dereferences a CDBPtr
// or names NDb::GetObject after that will not link against NDb::GetObjectA.
#include <wx/msgdlg.h>
#include <wx/string.h>

namespace
{
	// The main frame, so that a box cannot fall behind the editor and so that wx
	// disables the frame while it is up, as MB_APPLMODAL did.
	wxWindow* Owner()
	{
		// The container owns the main window, not IMainFrame: the front end
		// registers both with Set(). This is MainWindowHandle()'s walk, stopping
		// at the wx window rather than its HWND.
		IMainFrameContainer *const pContainer = Singleton<IMainFrameContainer>();
		IWidget *const pWindow = ( pContainer != nullptr ) ? pContainer->GetMainWindow() : nullptr;
		return ( pWindow != nullptr ) ? ToWxOwnerWindow( pWindow ) : nullptr;
	}


	// What every one of these boxes was titled with, bar the four that named the
	// job they were part of.
	wxString AppTitle()
	{
		return wxString::FromUTF8( Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() );
	}


	int Show( const std::string &rszText, const wxString &rTitle, long nStyle )
	{
		return wxMessageBox( wxString::FromUTF8( rszText.c_str() ), rTitle, nStyle, Owner() );
	}
}


namespace NMessage
{
	bool AskYesNo( const std::string &rszText )
	{
		return Show( rszText, AppTitle(), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION ) == wxYES;
	}


	EAnswer AskYesNoCancel( const std::string &rszText )
	{
		switch ( Show( rszText, AppTitle(), wxYES_NO | wxCANCEL | wxNO_DEFAULT | wxICON_QUESTION ) )
		{
			case wxYES:	return ANSWER_YES;
			case wxNO:	return ANSWER_NO;
		}
		return ANSWER_CANCEL;
	}


	void Error( const std::string &rszText )
	{
		Show( rszText, AppTitle(), wxOK | wxICON_ERROR );
	}


	void Warning( const std::string &rszText )
	{
		Show( rszText, AppTitle(), wxOK | wxICON_EXCLAMATION );
	}


	void Information( const std::string &rszText )
	{
		Show( rszText, AppTitle(), wxOK | wxICON_INFORMATION );
	}


	void Error( const std::string &rszText, const std::string &rszTitle )
	{
		Show( rszText, wxString::FromUTF8( rszTitle.c_str() ), wxOK | wxICON_ERROR );
	}


	void Information( const std::string &rszText, const std::string &rszTitle )
	{
		Show( rszText, wxString::FromUTF8( rszTitle.c_str() ), wxOK | wxICON_INFORMATION );
	}
}
