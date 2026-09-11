#include "stdafx.h"

#include "AIGeneralDialogs.h"

#ifdef OBK2_WITH_WX

#include <fmt/format.h>

#include "AIGeneralTypes.h"

#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <cstdio>
#include <string>

// The AI general palette's two dialogs, in wx: the first modal dialogs out of
// ED_B2_M1 rather than MapEditor, and deliberately plain.
//
// Both follow NSearchObject's shape for a fixed-size dialog. Neither template
// has WS_THICKFRAME, so neither is resizable here; each is fitted to its
// contents and centred rather than given the template's numbers, and neither
// keeps its placement between sessions, which is what the other fixed-size
// migrated dialogs settled on.
//
// **Where the focus starts is kept.** Both templates list OK first, and a
// dialog puts the focus on its first tab stop, so both MFC dialogs open with OK
// focused and Enter accepting the value already in them. wx would start on the
// first control that takes focus -- the edit box -- which is arguably more
// useful and is not what these did, so OK is focused explicitly.

namespace
{
	// Where both dialogs start; see the note at the top.
	void FocusOk( wxDialog *pDialog )
	{
		if ( wxWindow *pOk = pDialog->FindWindow( wxID_OK ) )
		{
			pOk->SetFocus();
		}
	}


	// "Mobile reinforcement", IDD_DLG_AIGEN_MOBILE_ID: one label, one edit box.
	class CAIGenMobileWxDialog : public CWxToolDialog
	{
		wxTextCtrl *pMobileID = nullptr;

	public:
		CAIGenMobileWxDialog( wxWindow *pParent, int nMobileID )
			: CWxToolDialog( pParent, wxID_ANY, "Mobile reinforcement" )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Mobile reinforcement ScriptID:" ),
								 wxSizerFlags().Centre().Border( wxRIGHT, 6 ) );
			pMobileID = NWx::Child<wxTextCtrl>( this, wxID_ANY, fmt::format( "{}", nMobileID ),
																					wxDefaultPosition, wxSize( ConvertDialogToPixels( wxSize( 81, 0 ) ).x, -1 ) );
			pRow->Add( pMobileID, wxSizerFlags( 1 ).Centre() );
			pSizer->Add( pRow, wxSizerFlags().Expand().Border( wxALL, 8 ) );
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			Centre();
			FocusOk( this );
		}

		// OnOK's sscanf: what does not parse is 0.
		int GetMobileID() const
		{
			int nMobileID = 0;
			sscanf( std::string( pMobileID->GetValue().utf8_str() ).c_str(), "%d", &nMobileID );
			return nMobileID;
		}
	};


	// "Parcel properties", IDD_DLG_AIGEN_PARCEL: a type and an importance.
	class CAIGenParcelWxDialog : public CWxToolDialog
	{
		wxChoice *pType = nullptr;
		wxTextCtrl *pImportance = nullptr;

	public:
		CAIGenParcelWxDialog( wxWindow *pParent, NDb::EParcelType eType, float fImportance )
			: CWxToolDialog( pParent, wxID_ANY, "Parcel properties" )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			wxFlexGridSizer *pGrid = new wxFlexGridSizer( 2, wxSize( 6, 4 ) );
			pGrid->AddGrowableCol( 1 );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Parcel type:" ), wxSizerFlags().CentreVertical() );
			// Unsorted, as the template's combo box is, so a type's position is
			// its value: the original reads GetCurSel back as the enum.
			pType = NWx::Child<wxChoice>( this, wxID_ANY, wxDefaultPosition,
																		wxSize( ConvertDialogToPixels( wxSize( 93, 0 ) ).x, -1 ) );
			for ( int nType = 0; nType < typeAIGeneralParcel.Size(); ++nType )
			{
				pType->Append( wxString::FromUTF8( typeAIGeneralParcel.GetMnemonic( nType ).c_str() ) );
			}
			// SelectString in the original, which is a case-insensitive prefix
			// match; the whole name is what was meant, and FindString cannot
			// land on a longer name that merely starts the same way. Case
			// insensitive still, as CB_SELECTSTRING is.
			const int nSelection = pType->FindString( wxString::FromUTF8( typeAIGeneralParcel.GetMnemonic( eType ).c_str() ), false );
			if ( nSelection != wxNOT_FOUND )
			{
				pType->SetSelection( nSelection );
			}
			pGrid->Add( pType, wxSizerFlags().Expand() );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Importance:" ), wxSizerFlags().CentreVertical() );
			// "{:f}", six decimals, as the original writes it.
			pImportance = NWx::Child<wxTextCtrl>( this, wxID_ANY, fmt::format( "{:f}", fImportance ) );
			pGrid->Add( pImportance, wxSizerFlags().Expand() );

			pSizer->Add( pGrid, wxSizerFlags().Expand().Border( wxALL, 8 ) );
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			Centre();
			FocusOk( this );
		}

		// OnOK's reading of the combo box: the selected position as the enum,
		// which is -1 with nothing selected, exactly as GetCurSel is.
		NDb::EParcelType GetType() const
		{
			return static_cast<NDb::EParcelType>( pType->GetSelection() );
		}

		// OnOK's sscanf: what does not parse is 0.
		float GetImportance() const
		{
			float fImportance = 0;
			sscanf( std::string( pImportance->GetValue().utf8_str() ).c_str(), "%f", &fImportance );
			return fImportance;
		}
	};
}


namespace NAIGenMobileDialog
{
	bool RunWx( IWidget *pParent, int *pMobileID )
	{
		if ( pMobileID == 0 )
		{
			return false;
		}
		CAIGenMobileWxDialog dialog( nullptr, *pMobileID );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		( *pMobileID ) = dialog.GetMobileID();
		return true;
	}
}


namespace NAIGenParcelDialog
{
	bool RunWx( IWidget *pParent, NDb::EParcelType *pType, float *pImportance )
	{
		if ( ( pType == 0 ) || ( pImportance == 0 ) )
		{
			return false;
		}
		CAIGenParcelWxDialog dialog( nullptr, *pType, *pImportance );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		( *pType ) = dialog.GetType();
		( *pImportance ) = dialog.GetImportance();
		return true;
	}
}

#endif // OBK2_WITH_WX
