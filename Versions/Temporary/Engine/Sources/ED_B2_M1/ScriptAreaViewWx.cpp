#include "stdafx.h"

#include "ScriptAreaView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/radiobut.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <vector>

// The script area palette, in wx: the tenth, and the first that needs no
// command dispatch of its own at all.
//
// The nine before it either answered extra commands or belonged to the
// edit-parameter family, whose two halves return bool and take a reference.
// This one answers ID_WINDOW_GET_DIALOG_DATA and ID_WINDOW_SET_DIALOG_DATA and
// nothing else, so CPaletteCommands is inherited whole and neither palette has
// a HandleCommand or an UpdateCommand in it. That is what the template was
// written for.
//
// It is also the first with a **report list rather than an icon list**, and a
// multiple selection: the areas the script can refer to, by name and by shape,
// with Delete and Select acting on however many rows are picked.
//
// The one thing that is deliberately not a copy of the original: the column
// widths. CScriptAreaWindow works them out once in OnInitDialog, from the
// client width the dialog template happens to have at that moment, and never
// again -- so the columns stay where they were while the palette is resized
// around them. Here they are recomputed on every size, from the same 80% and
// 19% the original uses.

namespace
{
	class CScriptAreaWxWindow : public CWxHostWindow, public CScriptAreaCommands
	{
		wxRadioButton *pCircle = nullptr;
		wxRadioButton *pRectangle = nullptr;
		wxButton *pDelete = nullptr;
		wxButton *pSelect = nullptr;
		wxListCtrl *pAreas = nullptr;

		// The same guard, under the same name: filling the list raises the
		// selection events a user does, and the state must not hear about them.
		bool bIsDataBeginSet = false;
		SScriptAreaWindowData dialogData;

	public:
		CScriptAreaWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_SCRIPT_AREA_WINDOW, this );
		}

		virtual ~CScriptAreaWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_SCRIPT_AREA_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pHostRoot = Root();

			// Scrolled for the reason the other palettes are: three fixed rows
			// and a list that takes the slack.
			wxScrolledWindow *const pRoot = NWx::Child<wxScrolledWindow>( pHostRoot, wxID_ANY );
			pRoot->SetScrollRate( 0, 8 );
			wxBoxSizer *pHostSizer = new wxBoxSizer( wxVERTICAL );
			pHostSizer->Add( pRoot, wxSizerFlags( 1 ).Expand() );
			pHostRoot->SetSizer( pHostSizer );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Script area type:" ),
									 wxSizerFlags().Expand() );

			wxBoxSizer *pTypeRow = new wxBoxSizer( wxHORIZONTAL );
			pCircle = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Circle", wxDefaultPosition,
																					 wxDefaultSize, wxRB_GROUP );
			pTypeRow->Add( pCircle );
			pRectangle = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Rectangle" );
			pTypeRow->Add( pRectangle, wxSizerFlags().Border( wxLEFT, 8 ) );
			pSizer->Add( pTypeRow, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			wxBoxSizer *pButtonRow = new wxBoxSizer( wxHORIZONTAL );
			pDelete = NWx::Child<wxButton>( pRoot, wxID_ANY, "Delete" );
			pButtonRow->Add( pDelete );
			pSelect = NWx::Child<wxButton>( pRoot, wxID_ANY, "Select" );
			pButtonRow->Add( pSelect, wxSizerFlags().Border( wxLEFT, 4 ) );
			pSizer->Add( pButtonRow, wxSizerFlags().Expand().Border( wxTOP, 4 ) );

			// LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT and no
			// LVS_SINGLESEL: more than one area can be picked at a time, which
			// is the point of the Delete and Select buttons.
			//
			// The original also asks for LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT.
			// wx sets full-row select on every list control it makes and draws
			// its own rules for wxLC_HRULES and wxLC_VRULES, which is the same
			// two things by another route.
			pAreas = NWx::Child<wxListCtrl>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																			 wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN );
			pAreas->InsertColumn( 0, "Area name", wxLIST_FORMAT_LEFT );
			pAreas->InsertColumn( 1, "Type", wxLIST_FORMAT_LEFT );
			pAreas->SetMinSize( wxSize( -1, 160 ) );
			pSizer->Add( pAreas, wxSizerFlags( 1 ).Expand().Border( wxTOP, 4 ) );

			pRoot->SetSizer( pSizer );
			pRoot->FitInside();

			pCircle->Bind( wxEVT_RADIOBUTTON, &CScriptAreaWxWindow::OnRadioCircle, this );
			pRectangle->Bind( wxEVT_RADIOBUTTON, &CScriptAreaWxWindow::OnRadioRectangle, this );
			pDelete->Bind( wxEVT_BUTTON, &CScriptAreaWxWindow::OnButtonDelete, this );
			pSelect->Bind( wxEVT_BUTTON, &CScriptAreaWxWindow::OnButtonSelect, this );
			// LVN_ITEMCHANGED covers both directions, so both wx events bind to
			// the same handler.
			pAreas->Bind( wxEVT_LIST_ITEM_SELECTED, &CScriptAreaWxWindow::OnAreaSelectionChanged, this );
			pAreas->Bind( wxEVT_LIST_ITEM_DESELECTED, &CScriptAreaWxWindow::OnAreaSelectionChanged, this );
			pAreas->Bind( wxEVT_SIZE, &CScriptAreaWxWindow::OnAreaListSize, this );

			UpdateControls();
			return true;
		}

		//	CScriptAreaCommands
		virtual void GetDialogData( SScriptAreaWindowData *pData )
		{
			( *pData ) = dialogData;
		}

		virtual void SetDialogData( const SScriptAreaWindowData *pData )
		{
			bIsDataBeginSet = true;
			dialogData = *pData;
			//
			if ( pData->eChangeMask & SScriptAreaWindowData::CHANGE_AREA_TYPE )
			{
				// The original clears both and then sets one, because MFC's
				// radio buttons are not grouped by the dialog manager here.
				// wxRB_GROUP does the clearing.
				if ( pData->eAreaType == NDb::EAT_RECTANGLE )
				{
					pRectangle->SetValue( true );
				}
				else if ( pData->eAreaType == NDb::EAT_CIRCLE )
				{
					pCircle->SetValue( true );
				}
			}
			//
			if ( pData->eChangeMask & SScriptAreaWindowData::CHANGE_AREAS )
			{
				pAreas->DeleteAllItems();
				for ( size_t nArea = 0; nArea < pData->scriptAreaList.size(); ++nArea )
				{
					const SScriptAreaWindowData::SScriptArea &rArea = pData->scriptAreaList[nArea];
					const long nItem = pAreas->InsertItem( nArea, wxString::FromUTF8( rArea.szName.c_str() ) );
					pAreas->SetItemData( nItem, rArea.nScriptAreaID );
					switch ( rArea.eType )
					{
						case NDb::EAT_CIRCLE:
							pAreas->SetItem( nItem, 1, "circle" );
							break;
						case NDb::EAT_RECTANGLE:
							pAreas->SetItem( nItem, 1, "rect" );
							break;
					}
				}
			}
			//
			if ( pData->eChangeMask & SScriptAreaWindowData::CHANGE_SELECTION )
			{
				for ( long nItem = 0; nItem < pAreas->GetItemCount(); ++nItem )
				{
					const unsigned nScriptAreaID = static_cast<unsigned>( pAreas->GetItemData( nItem ) );
					bool bSelected = false;
					for ( size_t nSelected = 0; nSelected < pData->selectedScriptAreaIDList.size(); ++nSelected )
					{
						if ( pData->selectedScriptAreaIDList[nSelected] == nScriptAreaID )
						{
							bSelected = true;
							break;
						}
					}
					pAreas->SetItemState( nItem, bSelected ? wxLIST_STATE_SELECTED : 0,
																wxLIST_STATE_SELECTED );
				}
			}
			//
			bIsDataBeginSet = false;
		}

	private:
		// Delete and Select do nothing without a selection, and say so.
		void UpdateControls()
		{
			const bool bEnable = ( pAreas->GetSelectedItemCount() > 0 );
			pDelete->Enable( bEnable );
			pSelect->Enable( bEnable );
		}

		// Whatever is selected, as the ids the state knows areas by.
		void CollectSelection()
		{
			dialogData.selectedScriptAreaIDList.clear();
			long nItem = pAreas->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			while ( nItem >= 0 )
			{
				dialogData.selectedScriptAreaIDList.push_back(
						static_cast<unsigned>( pAreas->GetItemData( nItem ) ) );
				nItem = pAreas->GetNextItem( nItem, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			}
		}

		// One command, with eChangeMask already saying what it is about. The
		// state reads the whole struct back and decides.
		void NotifyHandler()
		{
			if ( bIsDataBeginSet )
			{
				return;	// чтобы сообщения не шли в момент установки свойств контролов
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_AREA_STATE,
																														ID_SCRIPT_AREA_WINDOW_UI_EVENT, 0 );
		}

		void OnRadioCircle( wxCommandEvent& )
		{
			dialogData.eAreaType = NDb::EAT_CIRCLE;
			dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_AREA_TYPE;
			NotifyHandler();
		}

		void OnRadioRectangle( wxCommandEvent& )
		{
			dialogData.eAreaType = NDb::EAT_RECTANGLE;
			dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_AREA_TYPE;
			NotifyHandler();
		}

		void OnButtonDelete( wxCommandEvent& )
		{
			CollectSelection();
			dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_DEL_SEL;
			NotifyHandler();
			UpdateControls();
		}

		void OnButtonSelect( wxCommandEvent& )
		{
			CollectSelection();
			dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_SELECTION;
			NotifyHandler();
		}

		void OnAreaSelectionChanged( wxListEvent& )
		{
			UpdateControls();
		}

		// The 80/19 split the original computes once in OnInitDialog, applied
		// every time the list changes width instead.
		void OnAreaListSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			const int nWidth = pAreas->GetClientSize().GetWidth();
			if ( nWidth > 0 )
			{
				pAreas->SetColumnWidth( 0, (int)( nWidth * 0.8 ) );
				pAreas->SetColumnWidth( 1, (int)( nWidth * 0.19 ) );
			}
		}
	};
}


namespace NScriptAreaView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CScriptAreaWxWindow *pWindow = pTabWindow->AddNewTab( new CScriptAreaWxWindow() );
		if ( pWindow == 0 )
		{
			return 0;
		}
		if ( !pWindow->Build( pTabWindow ) )
		{
			// Left in the tab list deliberately: it is the list's to delete.
			return 0;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
