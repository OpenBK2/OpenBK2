#include "stdafx.h"

#include "ReinfPointsDialogs.h"


#include <fmt/format.h>

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"
#include "ReinforcementTypes.h"
#include "StringResources.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

// The reinforcement points palette's two dialogs, in wx. Neither template has
// WS_THICKFRAME, so neither is resizable; each is fitted to its contents, with
// the list given the size its template gave it.
//
// **Neither remembers where it was put**, and that is the thing to establish
// before writing one of these rather than after. Both derive from CResizeDialog
// and both call its OnInitDialog, which is usually enough to mean a placement
// file -- but placement is keyed on GetXMLFilePath, which comes from
// DECLARE_RESIZE_DLG_WND_COMMON_METHODS, and neither of these two declares it.
// The base returns an empty label, NDialogState refuses an empty one, and
// nothing is read or written. So these open centred on the frame every time,
// and there is no NWxPlacement::CPlacement here. What they do get from that
// OnInitDialog is the shell font, which a wx dialog has anyway.
//
// The database half of Add and Remove is not here: it is in
// ReinfPointsDialogs.cpp, which the MFC dialog calls as well, so both run the
// same inserts and the same rollback.

namespace
{
	// "Typed templates", IDD_DLG_REINFPTS_TEMPLATES: Add and Remove over a
	// three-column list of the point's typed templates.
	class CReinfPointsTemplatesWxDialog : public CWxToolDialog
	{
		wxListCtrl *pList = nullptr;
		wxButton *pRemove = nullptr;

		CReinfPointsState::CTypedTemplateType *pTemplates = nullptr;
		CMapInfoEditor *pMapInfoEditor = nullptr;
		int nPlayer = 0;
		int nReinfPoint = 0;
		// -1 for none, as CReinfPointsTypedDlg's nSelectedTemplate is.
		long nSelected = -1;
		// The guard the MFC dialog does not need: it selects a row by sending
		// LVM_SETITEMSTATE, which raises LVN_ITEMCHANGED, and its message map
		// listens for NM_CLICK -- a user's click and nothing else. wx has one
		// event for both, so filling the list would otherwise report a selection
		// back to the handler that is filling it.
		bool bSettingData = false;

	public:
		CReinfPointsTemplatesWxDialog( CReinfPointsState::CTypedTemplateType *_pTemplates,
																	 CMapInfoEditor *_pMapInfoEditor, int _nPlayer, int _nReinfPoint )
			: CWxToolDialog( nullptr, wxID_ANY, "Typed templates" ),
				pTemplates( _pTemplates ), pMapInfoEditor( _pMapInfoEditor ),
				nPlayer( _nPlayer ), nReinfPoint( _nReinfPoint )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			wxBoxSizer *pTopRow = new wxBoxSizer( wxHORIZONTAL );
			wxButton *const pAdd = NWx::Child<wxButton>( this, wxID_ANY, "Add", wxDefaultPosition,
																									 wxSize( ConvertDialogToPixels( wxSize( 40, 0 ) ).x, -1 ) );
			pRemove = NWx::Child<wxButton>( this, wxID_ANY, "Remove", wxDefaultPosition,
																			wxSize( ConvertDialogToPixels( wxSize( 40, 0 ) ).x, -1 ) );
			pTopRow->Add( pAdd, wxSizerFlags().Border( wxRIGHT, 3 ) );
			pTopRow->Add( pRemove );
			pSizer->Add( pTopRow, wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );

			// LVS_SINGLESEL and the grid lines and full-row select the dialog adds
			// in OnInitDialog. LVS_EX_INFOTIP, which pops a tip over a truncated
			// cell, has no wxListCtrl equivalent and is not reproduced.
			pList = NWx::Child<wxListCtrl>( this, wxID_ANY, wxDefaultPosition,
																			ConvertDialogToPixels( wxSize( 312, 91 ) ),
																			wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES |
																			wxBORDER_SUNKEN );
			// The widths CListCtrl::InsertColumn was given, which are pixels there
			// as they are here.
			pList->InsertColumn( 0, wxString::FromUTF8( RCSTR( "ID" ) ), wxLIST_FORMAT_LEFT, 30 );
			pList->InsertColumn( 1, wxString::FromUTF8( RCSTR( "Type" ) ), wxLIST_FORMAT_LEFT, 200 );
			pList->InsertColumn( 2, wxString::FromUTF8( RCSTR( "Template" ) ), wxLIST_FORMAT_LEFT, 200 );
			pSizer->Add( pList, wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );

			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxALL, 8 ) );
			SetSizerAndFit( pSizer );

			pAdd->Bind( wxEVT_BUTTON, &CReinfPointsTemplatesWxDialog::OnAdd, this );
			pRemove->Bind( wxEVT_BUTTON, &CReinfPointsTemplatesWxDialog::OnRemove, this );
			pList->Bind( wxEVT_LIST_ITEM_SELECTED, &CReinfPointsTemplatesWxDialog::OnRowSelected, this );

			// The list is the template's first control, so it is where the dialog
			// manager leaves the focus. wx would leave it on the Add button.
			pList->SetFocus();
			SetDialogData();
		}

		// GetDialogData: the list is what is read back, not the vector the rows
		// were built from. Nothing here edits a row in place, so the two agree --
		// but reading the list is what the MFC dialog does, so this does too.
		void ReadTemplates() const
		{
			pTemplates->clear();
			pTemplates->resize( pList->GetItemCount() );
			for ( int i = 0; i < static_cast<int>( pTemplates->size() ); ++i )
			{
				( *pTemplates )[i].szTemplateType = std::string( pList->GetItemText( i, 1 ).utf8_str() );
				( *pTemplates )[i].szTemplate = std::string( pList->GetItemText( i, 2 ).utf8_str() );
			}
		}

	private:
		void SetDialogData()
		{
			bSettingData = true;
			pList->DeleteAllItems();
			for ( int i = 0; i < static_cast<int>( pTemplates->size() ); ++i )
			{
				const long nRow = pList->InsertItem( i, wxString() );
				pList->SetItem( nRow, 0, wxString::FromUTF8( fmt::format( "{}", i ).c_str() ) );
				pList->SetItem( nRow, 1, wxString::FromUTF8( ( *pTemplates )[i].szTemplateType.c_str() ) );
				pList->SetItem( nRow, 2, wxString::FromUTF8( ( *pTemplates )[i].szTemplate.c_str() ) );
				pList->SetItemData( nRow, i );
			}
			if ( ( nSelected >= 0 ) && ( nSelected < pList->GetItemCount() ) )
			{
				pList->SetItemState( nSelected, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			}
			pRemove->Enable( nSelected != -1 );
			bSettingData = false;
		}

		void OnRowSelected( wxListEvent &rEvent )
		{
			if ( bSettingData )
			{
				return;
			}
			nSelected = rEvent.GetIndex();
			// The MFC handler rebuilds the whole list here. It cannot change what
			// is in it -- the rows come from the same vector -- so what it amounts
			// to is this, the Remove button following the selection.
			pRemove->Enable( nSelected != -1 );
		}

		void OnAdd( wxCommandEvent & )
		{
			// The add dialog's owner is the main frame, which is the window the
			// MFC dialog passed it as well.
			if ( NReinfPointsTemplates::Add( Singleton<IMainFrameContainer>()->GetMainWindow(), pTemplates,
																			 pMapInfoEditor, nPlayer, nReinfPoint ) )
			{
				SetDialogData();
			}
		}

		void OnRemove( wxCommandEvent & )
		{
			// IDS_MIMO_DELETE_OBJECT_MESSAGE, under the application's title, with
			// No as the default button as MB_DEFBUTTON2 makes it.
			wxMessageDialog question( this, RCSTR( "Are you sure you want to delete the selected object?" ),
																wxString::FromUTF8(
																	Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() ),
																wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			if ( question.ShowModal() != wxID_YES )
			{
				return;
			}
			NReinfPointsTemplates::Remove( pTemplates, pMapInfoEditor, nPlayer, nReinfPoint,
																		 static_cast<int>( nSelected ) );
			nSelected = -1;
			SetDialogData();
		}
	};


	// "Add Typed Template", IDD_DLG_REINFPTS_ADD_TEMPLATE: the type of one typed
	// template and the template itself, written into the node named by szNode.
	class CAddTemplateWxDialog : public CWxToolDialog
	{
		wxChoice *pType = nullptr;
		wxTextCtrl *pTemplate = nullptr;

		CMapInfoEditor *pMapInfoEditor = nullptr;
		std::string szNode;

	public:
		CAddTemplateWxDialog( const std::string &rszNode, CMapInfoEditor *_pMapInfoEditor )
			: CWxToolDialog( nullptr, wxID_ANY, "Add Typed Template" ),
				pMapInfoEditor( _pMapInfoEditor ), szNode( rszNode )
		{
			std::string szCurrentType;
			CManipulatorManager::GetValue( &szCurrentType, pMapInfoEditor->GetViewManipulator(),
																		 szNode + ".Type" );

			wxFlexGridSizer *pGrid = new wxFlexGridSizer( 2, 6, 6 );
			pGrid->AddGrowableCol( 1, 1 );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Type" ),
									wxSizerFlags().CentreVertical() );
			// CBS_DROPDOWNLIST: a list to pick from, nothing to type. Not sorted,
			// and the order matters: the type the node already has goes in first,
			// then every other mnemonic in value order, and the first entry is the
			// one selected. A sorted control would put a different type under the
			// selection.
			pType = NWx::Child<wxChoice>( this, wxID_ANY, wxDefaultPosition,
																		wxSize( ConvertDialogToPixels( wxSize( 139, 0 ) ).x, -1 ) );
			pType->Append( wxString::FromUTF8( szCurrentType.c_str() ) );
			for ( int i = 0; i < typeReinforcementMnemonics.Size(); ++i )
			{
				if ( typeReinforcementMnemonics.GetMnemonic( i ) != szCurrentType )
				{
					pType->Append( wxString::FromUTF8( typeReinforcementMnemonics.GetMnemonic( i ).c_str() ) );
				}
			}
			pType->SetSelection( 0 );
			pGrid->Add( pType, wxSizerFlags().Expand() );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Template" ),
									wxSizerFlags().CentreVertical() );
			wxBoxSizer *pTemplateRow = new wxBoxSizer( wxHORIZONTAL );
			// Empty, whatever the node holds: the MFC dialog reads .Template in
			// OnInitDialog and never puts it in the box, so a template is typed or
			// browsed for, never edited. The dialog only ever opens on a node that
			// has just been created, where there is nothing to show anyway.
			pTemplate = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(), wxDefaultPosition,
																					wxSize( ConvertDialogToPixels( wxSize( 113, 0 ) ).x, -1 ) );
			pTemplateRow->Add( pTemplate, wxSizerFlags( 1 ).CentreVertical() );
			wxButton *const pBrowse = NWx::Child<wxButton>( this, wxID_ANY, "...", wxDefaultPosition,
																										 wxSize( ConvertDialogToPixels( wxSize( 13, 0 ) ).x, -1 ) );
			pTemplateRow->Add( pBrowse, wxSizerFlags().CentreVertical().Border( wxLEFT, 4 ) );
			pGrid->Add( pTemplateRow, wxSizerFlags().Expand() );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pGrid, wxSizerFlags( 1 ).Expand().Border( wxALL, 8 ) );
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );

			pBrowse->Bind( wxEVT_BUTTON, &CAddTemplateWxDialog::OnBrowse, this );
			// OK is this template's first control, and a DEFPUSHBUTTON, so that is
			// where the focus starts -- not on the type list, which is where wx
			// would put it.
			if ( wxWindow *const pOk = FindWindow( wxID_OK ) )
			{
				pOk->SetFocus();
			}
		}

		// OnBnClickedOk: the type as its mnemonic, the template as typed. Both
		// through the manipulator, the template with the flag the MFC dialog
		// passes.
		void Store() const
		{
			CManipulatorManager::SetValue( std::string( pType->GetStringSelection().utf8_str() ),
																		 pMapInfoEditor->GetViewManipulator(), szNode + ".Type", true );
			CManipulatorManager::SetValue( std::string( pTemplate->GetValue().utf8_str() ),
																		 pMapInfoEditor->GetViewManipulator(), szNode + ".Template", true );
		}

	private:
		void OnBrowse( wxCommandEvent & )
		{
			// The editor's own link browser, which is still an MFC dialog and is
			// reached through an interface that names no toolkit. It owns itself
			// to the main frame, as it does for every other caller.
			const std::string szTemplate = szNode + ".Template";
			std::string szLink;
			if ( Singleton<IMainFrameContainer>()->Get()->BrowseLink(
						 &szLink, "",
						 dynamic_cast<const SPropertyDesc*>( pMapInfoEditor->GetViewManipulator()->GetDesc( szTemplate ) ),
						 false, true ) )
			{
				pTemplate->SetValue( wxString::FromUTF8( szLink.c_str() ) );
			}
		}
	};
}


namespace NReinfPointsTemplates
{
	bool Run( IWidget *pParent, CReinfPointsState::CTypedTemplateType *pTemplates,
							CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint )
	{
		if ( pTemplates == 0 || pMapInfoEditor == 0 )
		{
			return false;
		}
		CReinfPointsTemplatesWxDialog dialog( pTemplates, pMapInfoEditor, nPlayer, nReinfPoint );
		NWxModal::CentreOver( &dialog, pParent );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		dialog.ReadTemplates();
		return true;
	}
}


namespace NReinfPointsAddTemplate
{
	bool Run( IWidget *pParent, const std::string &rszNode, CMapInfoEditor *pMapInfoEditor )
	{
		if ( pMapInfoEditor == 0 )
		{
			return false;
		}
		CAddTemplateWxDialog dialog( rszNode, pMapInfoEditor );
		NWxModal::CentreOver( &dialog, pParent );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		dialog.Store();
		return true;
	}
}

