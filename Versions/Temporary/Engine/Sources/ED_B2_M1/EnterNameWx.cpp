#include "stdafx.h"

#include "EnterName.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

// The name prompt, in wx: IDD_DLG_AREA_NAME, a label, an edit box, OK and
// Cancel, with the caption and the label the caller gives -- "Area name" for
// both today, so the template's "Area name:" label shows without its colon, as
// it always has.
//
// What is kept from the running MFC dialog, measured rather than read:
//
//   * the focus starts on OK, the template's first control, so Enter accepts
//     the name the box starts with -- the last one accepted;
//   * the frame is resizable, as the template's WS_THICKFRAME makes it, and
//     placement is not remembered: the MFC class calls CDialog::OnInitDialog
//     rather than CResizeDialog's and never saves, so it has no state file;
//   * it opens centred on the frame (NWxModal::CentreOver).
//
// What is not: in the MFC dialog nothing moves when it is resized -- no control
// was ever anchored -- so making it wider only adds empty space. Here the edit
// box takes the width, which is the only thing a wider name prompt is for. It
// cannot be made smaller than it opens.

namespace
{
	class CEnterNameWxDialog : public CWxToolDialog
	{
		wxTextCtrl *pName = nullptr;

	public:
		CEnterNameWxDialog( const std::string &rszCaption, const std::string &rszLabel, const std::string &rszName )
			: CWxToolDialog( nullptr, wxID_ANY, wxString::FromUTF8( rszCaption.c_str() ), wxDefaultPosition,
											 wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, wxString::FromUTF8( rszLabel.c_str() ) ),
								 wxSizerFlags().CentreVertical().Border( wxRIGHT, 6 ) );
			// The template's 135 dialog units, at this dialog's font.
			pName = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString::FromUTF8( rszName.c_str() ), wxDefaultPosition,
																			wxSize( ConvertDialogToPixels( wxSize( 135, 0 ) ).x, -1 ) );
			pRow->Add( pName, wxSizerFlags( 1 ).CentreVertical() );
			pSizer->Add( pRow, wxSizerFlags().Expand().Border( wxALL, 8 ) );
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			// Wider, never smaller or taller: only the edit box has anywhere to go.
			SetMinSize( GetSize() );
			SetMaxSize( wxSize( -1, GetSize().y ) );
			if ( wxWindow *pOk = FindWindow( wxID_OK ) )
			{
				pOk->SetFocus();
			}
		}

		// Not GetName: wxWindow has one, and this would hide it.
		std::string EnteredName() const
		{
			return std::string( pName->GetValue().utf8_str() );
		}
	};
}


namespace NEnterName
{
	bool RunWx( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName )
	{
		CEnterNameWxDialog dialog( rszCaption, rszLabel, *pszName );
		NWxModal::CentreOver( &dialog, pParent );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		( *pszName ) = dialog.EnteredName();
		return true;
	}
}

#endif // OBK2_WITH_WX
