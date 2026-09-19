#include "stdafx.h"

#include "NewObjectView.h"


#include "MapEditorLib/WxModal.h"
#include "Misc/StrProc.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>

// "Create New <type> Object", in wx. The first MapEditor dialog to move since
// the five that opened the migration, and the first anywhere that remembers
// something other than where it was put: the Add Type button's state is
// parameter 0 of the same file that holds the size and position, so a session
// that turns it on gets it on next time, in either toolkit.
//
// The rules it enforces are not here. The postfix the Add Type button
// maintains, and the three conditions that decide whether OK is available, are
// in NewObjectViewMfc.cpp with the MFC dialog calling them too; see
// NewObjectView.h for why those two in particular.
//
// One thing reads oddly and is deliberate: every edit writes straight into the
// caller's SBuildDataParams, including the ones the user then cancels. That is
// what the MFC dialog does -- the struct is scratch space the caller throws
// away unless this answers true -- and making the wx one tidier would have
// meant the two disagreeing about what a cancelled dialog leaves behind.

namespace
{
	// The label CNewObjectDialog::GetXMLFilePath answers with, so both
	// implementations share Editor/ResizeDialogStyles/CNewObjectDialog.xml.
	const char *const PSZ_STATE_NAME = "CNewObjectDialog";
	// Which slot in it carries the Add Type button.
	const size_t N_ADD_TYPE_PARAMETER = 0;


	class CNewObjectWxDialog : public CWxToolDialog
	{
		// Size, position and the Add Type button, in CNewObjectDialog's file.
		NWxPlacement::CSizedPlacement placement { PSZ_STATE_NAME };

		wxChoice *pTypes = nullptr;
		wxToggleButton *pAddType = nullptr;
		wxTextCtrl *pName = nullptr;
		wxCheckBox *pExport = nullptr;

		std::vector<std::string> objectTypeNameList;
		SBuildDataParams *pBuildDataParams = nullptr;
		// The type may only be chosen for an .xdb; for anything else the button
		// is dead and the postfix is never added.
		bool bEnableType = true;

	public:
		CNewObjectWxDialog( const std::vector<std::string> &rObjectTypeNameList,
												int nObjectTypeNameIndex, SBuildDataParams *_pBuildDataParams )
			: CWxToolDialog( nullptr, wxID_ANY, "Create Game Data Base Object",
											 wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				objectTypeNameList( rObjectTypeNameList ), pBuildDataParams( _pBuildDataParams )
		{
			// CNewObjectDialog::SetBuildDataParams, which runs before the window
			// exists there and before the controls are made here. Note what it
			// does to the name the caller suggested: it clears it, and the box
			// opens empty. The MFC dialog copies that name into its DDX member
			// first and then overwrites the member from the cleared struct in
			// OnInitDialog, so the suggestion never reaches the screen -- keeping
			// it here would be the one visible difference between the two.
			if ( ( nObjectTypeNameIndex >= 0 ) &&
					 ( nObjectTypeNameIndex < static_cast<int>( objectTypeNameList.size() ) ) )
			{
				pBuildDataParams->szObjectTypeName = objectTypeNameList[nObjectTypeNameIndex];
			}
			pBuildDataParams->szObjectName.clear();
			std::string szExtension = pBuildDataParams->szObjectNameExtention;
			NStr::ToLower( &szExtension );
			bEnableType = ( szExtension == ".xdb" );

			// The template's rows: a type label and its list, a name label with
			// its box and the Add Type button beside it, then the export check
			// with OK and Cancel on the same line. A grid-bag sizer because the
			// list spans both of the columns the name row splits into --
			// IDC_NO_TYPE_COMBO is 270 wide against the name box's 226.
			wxGridBagSizer *pGrid = new wxGridBagSizer( 6, 8 );
			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Type:" ), wxGBPosition( 0, 0 ),
									wxDefaultSpan, wxALIGN_CENTRE_VERTICAL );
			// CBS_SORT, and the item data is the position in the caller's list --
			// which the sort is free to reorder, so the data is what is read back.
			pTypes = NWx::Child<wxChoice>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																		 0, nullptr, wxCB_SORT );
			pGrid->Add( pTypes, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxEXPAND );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Name:" ), wxGBPosition( 1, 0 ),
									wxDefaultSpan, wxALIGN_CENTRE_VERTICAL );
			pName = NWx::Child<wxTextCtrl>( this, wxID_ANY );
			pGrid->Add( pName, wxGBPosition( 1, 1 ), wxDefaultSpan, wxEXPAND );
			// BS_AUTOCHECKBOX | BS_PUSHLIKE: a check box that looks like a button.
			pAddType = NWx::Child<wxToggleButton>( this, wxID_ANY, "Add Type", wxDefaultPosition,
																						 wxDefaultSize, wxBU_EXACTFIT );
			pGrid->Add( pAddType, wxGBPosition( 1, 2 ), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL );
			pGrid->AddGrowableCol( 1 );

			wxBoxSizer *pBottomRow = new wxBoxSizer( wxHORIZONTAL );
			pExport = NWx::Child<wxCheckBox>( this, wxID_ANY, "Export object after creation" );
			// The checkbox takes the width, which is what pushes the buttons to the
			// right -- ANCHORE_LEFT_BOTTOM | RESIZE_HOR against ANCHORE_RIGHT_BOTTOM.
			// Not wxSizerFlags().Right(): a horizontal sizer has nothing to align
			// horizontally *in*, and wx asserts on being asked.
			pBottomRow->Add( pExport, wxSizerFlags( 1 ).CentreVertical() );
			pBottomRow->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
											 wxSizerFlags().CentreVertical() );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pGrid, wxSizerFlags().Expand().Border( wxALL, 8 ) );
			// The fields are anchored to the top and the export check and the
			// buttons to the bottom, so a dialog taller than its contents grows in
			// the middle. It is resizable and remembers its size, so this is what
			// it looks like most of the time.
			pSizer->AddStretchSpacer( 1 );
			pSizer->Add( pBottomRow, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			SetMinSize( GetSize() );
			// The template's own size, so a first open looks like the MFC one
			// rather than like whatever the sizer's minimum happens to be.
			SetSize( ConvertDialogToPixels( wxSize( 310, 65 ) ) +
							 ( GetSize() - GetClientSize() ) );

			// CNewObjectDialog::OnInitDialog, in its order: the remembered Add
			// Type state, then the postfix that follows from it, then the list,
			// then the OK button and the title.
			placement.Restore( this );
			pAddType->SetValue( IsAddingType() );
			pAddType->Enable( bEnableType );
			pExport->SetValue( pBuildDataParams->bNeedExport );
			ApplyPostfixAndShowName();
			CreateTypeList();
			UpdateOkButton();
			UpdateTitle();

			pName->Bind( wxEVT_TEXT, &CNewObjectWxDialog::OnNameChanged, this );
			pTypes->Bind( wxEVT_CHOICE, &CNewObjectWxDialog::OnTypeChanged, this );
			pExport->Bind( wxEVT_CHECKBOX, &CNewObjectWxDialog::OnExportChanged, this );
			pAddType->Bind( wxEVT_TOGGLEBUTTON, &CNewObjectWxDialog::OnAddTypeChanged, this );
		}

		bool WasPlaced() const { return placement.WasPlaced(); }

		// The size, the position and the Add Type button, as CResizeDialog saves
		// them from its OnOK and OnCancel -- so on either button, not just OK.
		void SaveState()
		{
			placement.Save( this );
		}

	private:
		bool IsAddingType() const
		{
			return ( placement.State().GetIntParameter( N_ADD_TYPE_PARAMETER ) > 0 ) && bEnableType;
		}

		void ApplyPostfixAndShowName()
		{
			NNewObject::ApplyTypePostfix( pBuildDataParams, IsAddingType() );
			// ChangeValue, not SetValue: the MFC dialog guards its own
			// UpdateData( false ) with bCreateControls so that filling the box
			// does not read as the user typing. This is that guard.
			pName->ChangeValue( wxString::FromUTF8( pBuildDataParams->szObjectName.c_str() ) );
		}

		void CreateTypeList()
		{
			pTypes->Clear();
			for ( size_t i = 0; i < objectTypeNameList.size(); ++i )
			{
				pTypes->Append( wxString::FromUTF8( objectTypeNameList[i].c_str() ),
												reinterpret_cast<void*>( static_cast<intptr_t>( i ) ) );
			}
			pTypes->SetStringSelection(
				wxString::FromUTF8( pBuildDataParams->szObjectTypeName.c_str() ) );
			// One type is not a choice.
			pTypes->Enable( objectTypeNameList.size() > 1 );
		}

		void UpdateOkButton()
		{
			if ( wxWindow *const pOk = FindWindow( wxID_OK ) )
			{
				pOk->Enable( NNewObject::CanAccept( pBuildDataParams,
																						std::string( pName->GetValue().utf8_str() ) ) );
			}
		}

		void UpdateTitle()
		{
			SetTitle( wxString::FromUTF8( NNewObject::Title( pBuildDataParams ).c_str() ) );
		}

		void OnNameChanged( wxCommandEvent & )
		{
			pBuildDataParams->szObjectName = std::string( pName->GetValue().utf8_str() );
			UpdateOkButton();
		}

		void OnTypeChanged( wxCommandEvent & )
		{
			// The old type's postfix comes off before the new type's goes on,
			// which is what the MFC handler's dance with nParameters[0] amounts
			// to: strip with the button ignored, change the type, add it back.
			NNewObject::ApplyTypePostfix( pBuildDataParams, false );
			const int nSelected = pTypes->GetSelection();
			if ( nSelected != wxNOT_FOUND )
			{
				const int nIndex = static_cast<int>(
					reinterpret_cast<intptr_t>( pTypes->GetClientData( nSelected ) ) );
				if ( ( nIndex >= 0 ) && ( nIndex < static_cast<int>( objectTypeNameList.size() ) ) )
				{
					pBuildDataParams->szObjectTypeName = objectTypeNameList[nIndex];
				}
			}
			ApplyPostfixAndShowName();
			UpdateOkButton();
			UpdateTitle();
		}

		void OnExportChanged( wxCommandEvent & )
		{
			pBuildDataParams->bNeedExport = pExport->GetValue();
		}

		void OnAddTypeChanged( wxCommandEvent & )
		{
			placement.State().SetIntParameter( N_ADD_TYPE_PARAMETER, pAddType->GetValue() ? 1 : 0 );
			ApplyPostfixAndShowName();
			UpdateOkButton();
		}
	};
}


namespace NNewObject
{
	bool RunWx( IWidget *pParent, const std::vector<std::string> &rObjectTypeNameList,
							int nObjectTypeNameIndex, SBuildDataParams *pBuildDataParams )
	{
		CNewObjectWxDialog dialog( rObjectTypeNameList, nObjectTypeNameIndex, pBuildDataParams );
		if ( !dialog.WasPlaced() )
		{
			NWxModal::CentreOver( &dialog, pParent );
		}
		const bool bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );
		dialog.SaveState();
		return bAccepted;
	}
}

