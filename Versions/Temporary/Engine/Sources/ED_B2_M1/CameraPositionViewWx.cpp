#include "stdafx.h"

#include "CameraPositionView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <string>

// The start-camera-positions palette, in wx: the first piece of ED_B2_M1 to
// move, and the first thing that is neither a modal dialog nor a docking pane.
//
// A palette is a child window the tab control owns -- CDefault3DTabWindow
// deletes every tab it holds through a CWnd*, in its destructor and in
// RemoveAllTabs -- so this has to *be* a CWnd, and the wx content lives inside
// it. CWxHostWindow is that arrangement, and the teardown ordering it exists
// for is the whole reason it is not written out here: wx's window procedure is
// on the host's handle, so the wx side comes down on WM_DESTROY while the
// handle is still alive.
//
// What is not duplicated: the two commands CMapInfoState drives the palette
// with are dispatched by CCameraPositionCommands, shared with the MFC palette,
// so only the reading and writing of controls is written twice. That is the
// part that genuinely differs.

namespace
{
	class CCameraPositionWxWindow : public CWxHostWindow, public CCameraPositionCommands
	{
		wxChoice *pPlayers = nullptr;
		wxRadioButton *pPositionOnly = nullptr;
		wxRadioButton *pAllParams = nullptr;
		// The same guard the MFC palette has: filling the controls raises the
		// same change events the user does, and those must not be reported back
		// as edits.
		bool bIsDataSetting = false;

	public:
		CCameraPositionWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_CAMERA_POSITION_WINDOW, this );
		}

		virtual ~CCameraPositionWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_CAMERA_POSITION_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();

			// The anchors from CCameraPositionWindow's constructor, translated:
			//   IDC_OW_PLAYER_LABEL    ANCHORE_LEFT_TOP              -> fixed
			//   IDC_OW_PLAYER_COMBO_BOX ANCHORE_LEFT_TOP | RESIZE_HOR -> proportion 1
			//   IDC_OW_SET_BUTTON      ANCHORE_RIGHT_TOP             -> fixed, at the right
			//   IDC_OW_POSITION_ONLY_RADIO  ... | RESIZE_HOR         -> Expand
			//   IDC_OW_ALL_PARAMS_RADIO     ... | RESIZE_HOR         -> Expand
			//
			// RESIZE_HOR on a control sharing a row with fixed ones is the row's
			// proportion rather than plain Expand, which is what makes the combo
			// take the slack and the Set button stay put on the right.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Player:" ),
								 wxSizerFlags().CentreVertical().Border( wxRIGHT, 4 ) );
			pPlayers = NWx::Child<wxChoice>( pRoot, wxID_ANY, wxDefaultPosition,
																			 wxDefaultSize, 0, nullptr, wxCB_SORT );
			pRow->Add( pPlayers, wxSizerFlags( 1 ).CentreVertical() );
			wxButton *pSet = NWx::Child<wxButton>( pRoot, wxID_ANY, "Set" );
			pRow->Add( pSet, wxSizerFlags().CentreVertical().Border( wxLEFT, 4 ) );
			pSizer->Add( pRow, wxSizerFlags().Expand() );

			// wxRB_GROUP on the first, which is what WS_GROUP says in the
			// template: the two are one group and exactly one is set.
			pPositionOnly = NWx::Child<wxRadioButton>( pRoot, wxID_ANY,
																								 "Set Only Position for Camera",
																								 wxDefaultPosition, wxDefaultSize,
																								 wxRB_GROUP );
			pSizer->Add( pPositionOnly, wxSizerFlags().Expand().Border( wxTOP, 6 ) );
			pAllParams = NWx::Child<wxRadioButton>( pRoot, wxID_ANY,
																							"Set All Params for Camera" );
			pSizer->Add( pAllParams, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			pRoot->SetSizer( pSizer );

			pPlayers->Bind( wxEVT_CHOICE, &CCameraPositionWxWindow::OnPlayerChanged, this );
			pSet->Bind( wxEVT_BUTTON, &CCameraPositionWxWindow::OnSet, this );
			pPositionOnly->Bind( wxEVT_RADIOBUTTON, &CCameraPositionWxWindow::OnParamType, this );
			pAllParams->Bind( wxEVT_RADIOBUTTON, &CCameraPositionWxWindow::OnParamType, this );
			return true;
		}

		//	CCameraPositionCommands
		virtual void GetDialogData( SCameraPositionWindowData *pData )
		{
			if ( bIsDataSetting || pData == 0 )
			{
				return;
			}
			pData->Clear();
			pData->nPlayerCount = pPlayers->GetCount();
			// The client data, not the position: the list is sorted, so the two
			// are not the same number. Same reason Open MOD carries an index.
			const int nSelection = pPlayers->GetSelection();
			pData->nPlayerIndex = ( nSelection == wxNOT_FOUND ) ? -1 : PlayerAt( nSelection );
			pData->bAllParams = pAllParams->GetValue();
		}

		virtual void SetDialogData( const SCameraPositionWindowData *pData )
		{
			if ( pData == 0 )
			{
				return;
			}
			bIsDataSetting = true;
			pPlayers->Clear();
			for ( int nPlayer = 0; nPlayer < pData->nPlayerCount; ++nPlayer )
			{
				pPlayers->Append( wxString::FromUTF8( std::to_string( nPlayer ).c_str() ),
													reinterpret_cast<void*>( static_cast<uintptr_t>( nPlayer ) ) );
			}
			for ( unsigned nPos = 0; nPos < pPlayers->GetCount(); ++nPos )
			{
				if ( PlayerAt( nPos ) == pData->nPlayerIndex )
				{
					pPlayers->SetSelection( nPos );
					break;
				}
			}
			if ( pData->bAllParams )
			{
				pAllParams->SetValue( true );
			}
			else
			{
				pPositionOnly->SetValue( true );
			}
			bIsDataSetting = false;
		}

	private:
		int PlayerAt( unsigned nPos ) const
		{
			return (int)reinterpret_cast<uintptr_t>( pPlayers->GetClientData( nPos ) );
		}

		// The same three commands the MFC palette posts, to the same state.
		void OnPlayerChanged( wxCommandEvent& )
		{
			if ( bIsDataSetting )
			{
				return;
			}
			CWaitCursor wcur;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_STATE,
																														ID_CPE_ON_PLAYER_CHANGED, 0 );
		}

		void OnSet( wxCommandEvent& )
		{
			CWaitCursor wcur;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_STATE,
																														ID_CPW_ON_SAVE, 0 );
		}

		void OnParamType( wxCommandEvent& )
		{
			CWaitCursor wcur;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_STATE,
																														ID_CPW_PARAM_TYPE_CHANGED, 0 );
		}
	};
}


namespace NCameraPositionView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		// AddNewTab with a pointer rather than a null one: the template only
		// allocates when handed nothing, and this needs building before it is
		// registered. Either way the tab list owns it from here.
		CCameraPositionWxWindow *pWindow =
				pTabWindow->AddNewTab( new CCameraPositionWxWindow() );
		if ( pWindow == 0 )
		{
			return 0;
		}
		if ( !pWindow->Build( pTabWindow ) )
		{
			// Left in the tab list deliberately: it is the list's to delete, and
			// removing it here would be the only place that ever did.
			return 0;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
