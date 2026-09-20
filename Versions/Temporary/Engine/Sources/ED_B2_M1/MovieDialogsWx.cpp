#include "stdafx.h"

#include "MovieDialogs.h"


#include <fmt/format.h>

#include "MapEditorLib/WxWidget.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <cstdio>

// The script movie editor's two dialogs, in wx. Neither template has
// WS_THICKFRAME, so neither is resizable; each is fitted to its contents.
//
// They are not the same shape underneath, and the difference is the whole of
// what had to be checked before writing them:
//
//   * **Key settings calls CResizeDialog::OnInitDialog**, so it takes the
//     editor's shell font and reopens where it was last left, from
//     Editor/ResizeDialogStyles/CMovEditorKeySettingsDlg.xml. This one does the
//     same, through the same file; see MapEditorLib/WxPlacement.h.
//   * **Movie settings does not call it.** It gets neither the shell font nor
//     the placement, and opens centred on the frame every time. It does still
//     *write* a placement file, because its OnOK chains to CResizeDialog's,
//     which is where the writing happens -- a file nothing ever reads, so
//     nothing here writes one.
//
// Where the focus starts is kept in both: each template lists an edit box
// first, so that is where a dialog puts the focus, and the dialog manager
// selects the text in it.

namespace
{
	// "Script movie settings", IDD_DLG_MOVED_SETTINGS: one label, one edit.
	class CMovieSettingsWxDialog : public CWxToolDialog
	{
		wxTextCtrl *pLength = nullptr;

	public:
		CMovieSettingsWxDialog( wxWindow *pParent, float fLength )
			: CWxToolDialog( pParent, wxID_ANY, "Script movie settings" )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Movie length:" ),
								 wxSizerFlags().CentreVertical().Border( wxRIGHT, 6 ) );
			// "{:g}", as OnInitDialog writes it, and the template's 133 dialog
			// units at this dialog's font.
			pLength = NWx::Child<wxTextCtrl>( this, wxID_ANY, fmt::format( "{:g}", fLength ), wxDefaultPosition,
																				wxSize( ConvertDialogToPixels( wxSize( 133, 0 ) ).x, -1 ) );
			pRow->Add( pLength, wxSizerFlags( 1 ).CentreVertical() );
			pSizer->Add( pRow, wxSizerFlags().Expand().Border( wxALL, 8 ) );
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			// The template's first control, which is where the dialog manager
			// puts the focus, with the text selected.
			pLength->SetFocus();
			pLength->SelectAll();
		}

		// OnOK's sscanf: what does not parse is 0.
		float GetLength() const
		{
			float fLength = 0;
			sscanf( std::string( pLength->GetValue().utf8_str() ).c_str(), "%g", &fLength );
			return fLength;
		}
	};


	// "Key settings", IDD_DLG_MOVED_KEY_SETTINGS: the key's name, its in and out
	// tangents, and its parameter.
	class CMovieKeySettingsWxDialog : public CWxToolDialog
	{
		wxRadioButton *pInFlat = nullptr;
		wxRadioButton *pInTangent = nullptr;
		wxRadioButton *pOutFlat = nullptr;
		wxRadioButton *pOutTangent = nullptr;
		wxTextCtrl *pParam = nullptr;

		// One group box with Flat and Tangent in it. wxRB_GROUP starts a group,
		// as WS_GROUP does on the two Flat buttons in the template.
		wxSizer* TangentBox( const char *pszLabel, bool bTangent, wxRadioButton **ppFlat, wxRadioButton **ppTangent )
		{
			wxStaticBoxSizer *pBox = new wxStaticBoxSizer( wxVERTICAL, this, pszLabel );
			wxWindow *const pBoxWindow = pBox->GetStaticBox();
			*ppFlat = NWx::Child<wxRadioButton>( pBoxWindow, wxID_ANY, "Flat", wxDefaultPosition, wxDefaultSize,
																					 wxRB_GROUP );
			*ppTangent = NWx::Child<wxRadioButton>( pBoxWindow, wxID_ANY, "Tangent" );
			( *ppFlat )->SetValue( !bTangent );
			( *ppTangent )->SetValue( bTangent );
			pBox->Add( *ppFlat, wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 4 ) );
			pBox->Add( *ppTangent, wxSizerFlags().Border( wxALL, 4 ) );
			return pBox;
		}

	public:
		CMovieKeySettingsWxDialog( wxWindow *pParent, const NDb::SScriptMovieKeyPos &rKey, const std::string &rszName )
			: CWxToolDialog( pParent, wxID_ANY, "Key settings" )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// The name box: read-only and disabled in the template, so it shows
			// the name and takes neither focus nor input.
			wxTextCtrl *pName = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString::FromUTF8( rszName.c_str() ),
																								 wxDefaultPosition,
																								 wxSize( ConvertDialogToPixels( wxSize( 162, 0 ) ).x, -1 ),
																								 wxTE_READONLY );
			pName->Disable();
			pSizer->Add( pName, wxSizerFlags().Expand().Border( wxALL, 8 ) );

			wxBoxSizer *pTangents = new wxBoxSizer( wxHORIZONTAL );
			pTangents->Add( TangentBox( "In", rKey.bIsTangentIn, &pInFlat, &pInTangent ),
											wxSizerFlags( 1 ).Expand() );
			pTangents->Add( TangentBox( "Out", rKey.bIsTangentOut, &pOutFlat, &pOutTangent ),
											wxSizerFlags( 1 ).Expand().Border( wxLEFT, 6 ) );
			pSizer->Add( pTangents, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 8 ) );

			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Key parameter:" ),
									 wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );
			pParam = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString::FromUTF8( rKey.szKeyParam.c_str() ) );
			pSizer->Add( pParam, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 8 ) );

			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxALL, 8 ) );
			SetSizerAndFit( pSizer );
			// The parameter box is the template's first control, so it has the
			// focus and its text is selected; the name box is disabled and cannot
			// take it.
			pParam->SetFocus();
			pParam->SelectAll();
		}

		// GetDialogData: the tangents from the radios and the parameter as typed.
		// The name is not read back; nothing edits it.
		void ReadKey( NDb::SScriptMovieKeyPos *pKey ) const
		{
			pKey->bIsTangentIn = pInTangent->GetValue();
			pKey->bIsTangentOut = pOutTangent->GetValue();
			pKey->szKeyParam = std::string( pParam->GetValue().utf8_str() );
		}
	};
}


namespace NMovieSettings
{
	bool Run( IWidget *pParent, float *pfLength )
	{
		if ( pfLength == 0 )
		{
			return false;
		}
		CMovieSettingsWxDialog dialog( ToWxOwnerWindow( pParent ), *pfLength );
		dialog.CentreOnParent();
		if ( dialog.ShowModal() != wxID_OK )
		{
			return false;
		}
		( *pfLength ) = dialog.GetLength();
		return true;
	}
}


namespace NMovieKeySettings
{
	bool Run( IWidget *pParent, NDb::SScriptMovieKeyPos *pKey, const std::string &rszName )
	{
		if ( pKey == 0 )
		{
			return false;
		}
		CMovieKeySettingsWxDialog dialog( ToWxOwnerWindow( pParent ), *pKey, rszName );
		NWxPlacement::CPlacement placement( "CMovEditorKeySettingsDlg" );
		if ( !placement.Restore( &dialog ) )
		{
			dialog.CentreOnParent();
		}
		const int nResult = dialog.ShowModal();
		// Kept whichever button was used, which is where CResizeDialog writes it
		// too: its OnOK and its OnCancel.
		placement.Save( &dialog );
		if ( nResult != wxID_OK )
		{
			return false;
		}
		dialog.ReadKey( pKey );
		return true;
	}
}

