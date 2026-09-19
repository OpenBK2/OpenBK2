#include "stdafx.h"

#include "BitFieldView.h"


#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/button.h>
#include <wx/checklst.h>
#include <wx/sizer.h>

// The bit field editor in wx: a check list of the flag names over OK and
// Cancel. Which names, which are checked and what OK writes are all
// NBitField's, in BitFieldViewMfc.cpp.

namespace
{
	class CBitFieldWxDialog : public CWxToolDialog
	{
		NWxPlacement::CSizedPlacement placement;
		wxCheckListBox *pList = nullptr;
		std::vector<NBitField::SField> fields;

	public:
		CBitFieldWxDialog( const std::string &rszFieldsFile, const uint8_t *pData, int nSize )
			// "name" is IDD_BIT_FIELD's caption, and nothing ever replaces it.
			: CWxToolDialog( nullptr, wxID_ANY, "name", wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				placement( "CBinaryBitFieldDialog" )
		{
			NBitField::LoadFields( rszFieldsFile, &fields );

			// IDC_CT_FIELDS: LBS_EXTENDEDSEL with both scroll bars.
			pList = NWx::Child<wxCheckListBox>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																					0, nullptr, wxLB_EXTENDED | wxLB_HSCROLL );
			for ( size_t nField = 0; nField < fields.size(); ++nField )
			{
				const int nIndex = pList->Append( wxString::FromUTF8( fields[nField].szName.c_str() ) );
				pList->Check( nIndex, NBitField::IsSet( pData, nSize, fields[nField].nValue ) );
			}

			// The template's layout in the pixels MFC gives it, measured: the list
			// over a row of two buttons at the bottom right. IDD_BIT_FIELD is in
			// MS Sans Serif 8 with the Cyrillic charset, whose dialog unit is 2
			// pixels where the shell font's is 1.5 -- so 7 dlu margins are 14
			// pixels here, and 50x14 dlu buttons are 100x28.
			const wxSize buttonSize = FromDIP( wxSize( 100, 28 ) );
			wxBoxSizer *pContent = new wxBoxSizer( wxVERTICAL );
			pContent->Add( pList, wxSizerFlags( 1 ).Expand() );
			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			pButtons->AddStretchSpacer( 1 );
			wxButton *const pOk = NWx::Child<wxButton>( this, wxID_OK, "OK", wxDefaultPosition, buttonSize );
			pOk->SetDefault();
			pButtons->Add( pOk, wxSizerFlags().Border( wxRIGHT, FromDIP( 14 ) ) );
			pButtons->Add( NWx::Child<wxButton>( this, wxID_CANCEL, "Cancel", wxDefaultPosition, buttonSize ) );
			pContent->Add( pButtons, wxSizerFlags().Expand().Border( wxTOP, FromDIP( 10 ) ) );
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pContent, wxSizerFlags( 1 ).Expand().Border( wxALL, FromDIP( 14 ) ) );
			SetSizer( pSizer );
			// GetMinimumXDimension and GetMinimumYDimension, and the template's
			// 238x302 dlu for a first open.
			SetMinSize( wxSize( 204, 106 ) );
			SetClientSize( FromDIP( wxSize( 476, 604 ) ) );

			placement.Restore( this );
			// The template's first control is OK, so that is where the focus starts.
			pOk->SetFocus();
		}

		bool WasPlaced() const
		{
			return placement.WasPlaced();
		}

		void SaveState()
		{
			placement.Save( this );
		}

		void Store( uint8_t *pData, int nSize ) const
		{
			std::vector<bool> checked( fields.size(), false );
			for ( size_t nField = 0; nField < fields.size() && nField < pList->GetCount(); ++nField )
			{
				checked[nField] = pList->IsChecked( static_cast<unsigned>( nField ) );
			}
			NBitField::Store( fields, checked, pData, nSize );
		}
	};
}


namespace NBitField
{
	bool RunWx( IWidget *pParent, const std::string &rszFieldsFile, uint8_t *pData, int nSize )
	{
		CBitFieldWxDialog dialog( rszFieldsFile, pData, nSize );
		if ( !dialog.WasPlaced() )
		{
			NWxModal::CentreOver( &dialog, pParent );
		}
		const bool bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );
		dialog.SaveState();
		if ( bAccepted )
		{
			dialog.Store( pData, nSize );
		}
		return bAccepted;
	}
}

