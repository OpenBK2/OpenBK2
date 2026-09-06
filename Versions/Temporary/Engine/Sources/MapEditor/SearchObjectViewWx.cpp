#include "stdafx.h"

#include "SearchObjectView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

// Find Object, in wx. The third modal dialog and the smallest: one edit, two
// buttons, no state to remember.
//
// It is here for two things that are not about this dialog at all.
//
// **It is the first fixed-size dialog.** IDD_SEARCH_OBJECT has no
// WS_THICKFRAME, so there is no CResizeDialog under it and no anchors to
// translate -- the sizer's job is not to follow a resize but to work out how
// big the dialog should be in the first place. SetSizerAndFit is that, and it
// is what the other three non-resizable templates will want.
//
// **It is the first to get the editor's own window frame.** Sixteen of the
// twenty templates in MapEditor.rc are EXSTYLE WS_EX_TOOLWINDOW, and the two
// dialogs migrated before this one quietly lost it. See WxToolDialog.h for why
// wx will not hand a dialog that style through wxFRAME_TOOL_WINDOW, and what
// is done instead; both of those now derive from it too.

namespace
{
	class CSearchObjectWxDialog : public CWxToolDialog
	{
		wxTextCtrl *pText = nullptr;

	public:
		CSearchObjectWxDialog( wxWindow *pParent, const std::string &rszText )
			// "Find Object" is the CAPTION in the template. The class has always
			// been called CSearchObjectDialog and the window has never said so.
			//
			// No wxRESIZE_BORDER: the template has no WS_THICKFRAME, so this
			// dialog has never been resizable and there is nothing in it that
			// would use the room.
			: CWxToolDialog( pParent, wxID_ANY, "Find Object" )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// Label and edit on one row, the label vertically centred against it
			// -- the template does that by hand, putting the label at y=8 and the
			// edit at y=7 so their text lines up.
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Name:" ),
								 wxSizerFlags().Centre().Border( wxRIGHT, 6 ) );
			// 320 is the template's 215 dialog units at this font, and it is the
			// only hard number here: everything else comes out of Fit below.
			pText = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString::FromUTF8( rszText.c_str() ),
																			wxDefaultPosition, wxSize( 320, -1 ) );
			pRow->Add( pText, wxSizerFlags( 1 ).Centre() );
			pSizer->Add( pRow, wxSizerFlags().Expand().Border( wxALL, 8 ) );

			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );

			// Fit, not a size: a dialog that cannot be resized should be exactly
			// as big as its contents need, and then the numbers in the template
			// stop being something anyone has to maintain.
			SetSizerAndFit( pSizer );
			Centre();

			// What GotoDlgCtrl did. Focus alone is not the same thing: reaching an
			// edit through the dialog manager selects its text, so typing replaces
			// the previous search rather than appending to it.
			pText->SetFocus();
			pText->SelectAll();
		}

		std::string GetText() const
		{
			return std::string( pText->GetValue().utf8_str() );
		}
	};
}


namespace NSearchObject
{
	bool RunWx( IWidget *pParent, std::string *pszText )
	{
		if ( pszText == 0 )
		{
			return false;
		}
		CSearchObjectWxDialog dialog( nullptr, *pszText );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		( *pszText ) = dialog.GetText();
		return true;
	}
}

#endif // OBK2_WITH_WX
