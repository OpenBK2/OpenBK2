#include "stdafx.h"

#include "ScriptCameraView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include <fmt/format.h>

#include "ResourceDefines.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
#include "System/GlobalVars.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include <cstdio>
#include <string>

// The script camera palette, in wx: the eleventh, and the last of the palettes
// that live in the map info editor's shortcut bar.
//
// A saved-camera list for mission scripts: each row is a camera placement the
// script can cut or pan to, by ID, position, FOV, yaw and pitch. The palette
// shows the list and a Save button; the state does the rest.
//
// **Three of its buttons are not drawn, because nobody has ever seen them.**
// IDD_TAB_MI_SCRIPT_MOVIES has Add, Delete and Run, each six dialog units square
// and NOT WS_VISIBLE, tucked into the top right corner -- and nothing ever shows
// them. CScriptCameraWindow enables and disables them in SetDialogData, which
// changes nothing a user can see, and their click handlers are unreachable.
// Adding, deleting and running cameras happen elsewhere in the editor. Creating
// three hidden wx buttons to have something to call Enable on would be
// reproducing the template, not the palette, so they are not here.
//
// **Its keyboard shortcuts were never connected either.** OnKeyDown maps Return
// to Save, Insert to Add and Delete to Delete, and there is no ON_WM_KEYDOWN in
// the message map -- nor would a dialog receive the keys if there were, because
// they go to the focused control. Not reproduced, for the same reason.
//
// What is reproduced exactly:
//
//   * The manual controls. Three rows -- yaw, pitch and FOV -- that exist in
//     the template but are hidden unless the global variable
//     enable_movie_manual_controls is 1. Typing in one starts a 500 ms timer,
//     restarted by every keystroke, and when it finally fires the palette tells
//     the state which value changed; the state then asks for it with a float or
//     int pointer and the palette parses the box into it. A debounce, in 2005.
//
//   * The null check at the top of HandleCommand. The original's NI_VERIFY
//     refuses every command whose dwData is zero -- which includes
//     ID_SCRIPT_CAMERA_SHOW_MANUAL_CONTROLS with a zero meaning "hide", so that
//     command can only ever show them. Nothing in the tree sends it, so the
//     quirk is unobservable, and it is kept rather than tidied.
//
//   * Selecting nCurrentCamera after a refill, including -1. That value is
//     guarded here and was not there, and the two behave the same: asked to set
//     LVIS_SELECTED | LVIS_FOCUSED on item -1, a real list view refuses outright
//     -- "focus all" is refused on any list and "select all" on an LVS_SINGLESEL
//     one. Measured, not assumed: a SysListView32 made with the template's
//     styles returns FALSE and selects nothing, where the same call without the
//     focus bit on a multi-select list selects every row.

namespace
{
	typedef SScriptCameraWindowData SWindowData;

	// GetYawTimerInterval() and its two siblings, which the original spells as
	// member functions returning the same literal.
	const int MANUAL_CONTROL_DEBOUNCE = 500;


	class CScriptCameraWxWindow : public CWxHostWindow, public CScriptCameraCommands
	{
		// One per manual control. A one-shot wxTimer with a Notify(), which does
		// not point at any wx window and so cannot outlive one; the command it
		// sends says which value changed.
		class CDebounceTimer : public wxTimer
		{
			CScriptCameraWxWindow *pOwner;
			unsigned nCommandID;

		public:
			CDebounceTimer( CScriptCameraWxWindow *_pOwner, unsigned _nCommandID )
				: pOwner( _pOwner ), nCommandID( _nCommandID ) {}
			virtual void Notify() { pOwner->OnDebounce( nCommandID ); }
		};

		wxButton *pSave = nullptr;
		wxListCtrl *pCameras = nullptr;
		wxTextCtrl *pYaw = nullptr;
		wxTextCtrl *pPitch = nullptr;
		wxTextCtrl *pFOV = nullptr;
		wxSizer *pManualControls = nullptr;
		wxSizer *pSizer = nullptr;
		wxScrolledWindow *pRoot = nullptr;

		CDebounceTimer yawTimer;
		CDebounceTimer pitchTimer;
		CDebounceTimer fovTimer;

		SWindowData dialogData;
		// True from construction until the controls are built, as in the
		// original, and raised again around anything that fills them.
		bool bIsDataSetting = true;

	public:
		CScriptCameraWxWindow()
			: yawTimer( this, ID_SCRIPT_CAMERA_GET_YAW ),
				pitchTimer( this, ID_SCRIPT_CAMERA_GET_PITCH ),
				fovTimer( this, ID_SCRIPT_CAMERA_GET_FOV )
		{
			dialogData.nCurrentCamera = -1;
			Singleton<ICommandHandlerContainer>()->Set( CHID_SCRIPT_CAMERA_WINDOW, this );
		}

		virtual ~CScriptCameraWxWindow()
		{
			StopTimers();
			Singleton<ICommandHandlerContainer>()->Remove( CHID_SCRIPT_CAMERA_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}

			// Scrolled, as the others: a Save row, a label, the list taking the
			// slack, and up to three fixed rows under it. This is the palette
			// that showed the scrolled root had to rule out horizontal layout --
			// see CreateScrolledRoot.
			pRoot = CreateScrolledRoot();

			pSizer = new wxBoxSizer( wxVERTICAL );

			pSave = NWx::Child<wxButton>( pRoot, wxID_ANY, "Save" );
			pSizer->Add( pSave );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Script Camera List:" ),
									 wxSizerFlags().Expand().Border( wxTOP, 4 ) );

			// LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS from the template,
			// and LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT from OnInitDialog: wx
			// sets full-row select itself and draws its own rules for
			// wxLC_HRULES and wxLC_VRULES. LVS_EX_INFOTIP, the third, is wx's
			// LVS_EX_LABELTIP near enough -- a tip for a label that does not fit.
			pCameras = NWx::Child<wxListCtrl>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																				 wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES |
																				 wxLC_VRULES | wxBORDER_SUNKEN );
			pCameras->InsertColumn( 0, "ID" );
			pCameras->InsertColumn( 1, "Position" );
			pCameras->InsertColumn( 2, "FOV" );
			pCameras->InsertColumn( 3, "Yaw" );
			pCameras->InsertColumn( 4, "Pitch" );
			pCameras->SetMinSize( wxSize( -1, 120 ) );
			pSizer->Add( pCameras, wxSizerFlags( 1 ).Expand().Border( wxTOP, 2 ) );

			// The three manual rows, anchored to the bottom in the template,
			// which in a sizer is simply "after the list". A flex grid so the
			// labels line up, with the edit column taking the width.
			wxFlexGridSizer *pGrid = new wxFlexGridSizer( 3, wxSize( 4, 2 ) );
			pGrid->AddGrowableCol( 1 );
			pYaw = AddManualRow( pGrid, "Yaw:" );
			pPitch = AddManualRow( pGrid, "Pitch:" );
			pFOV = AddManualRow( pGrid, "FOV (1...179):" );
			pManualControls = pGrid;
			pSizer->Add( pGrid, wxSizerFlags().Expand().Border( wxTOP, 4 ) );

			pRoot->SetSizer( pSizer );

			pSave->Bind( wxEVT_BUTTON, &CScriptCameraWxWindow::OnSave, this );
			pCameras->Bind( wxEVT_LIST_ITEM_SELECTED, &CScriptCameraWxWindow::OnCameraSelected, this );
			// A double click on a row, and only that. wxEVT_LIST_ITEM_ACTIVATED
			// would also fire on Return, which the original never did: its
			// handler is NM_DBLCLK, and its Return handler is the unconnected
			// OnKeyDown.
			pCameras->Bind( wxEVT_LEFT_DCLICK, &CScriptCameraWxWindow::OnCameraDoubleClick, this );
			pCameras->Bind( wxEVT_SIZE, &CScriptCameraWxWindow::OnCameraListSize, this );
			pYaw->Bind( wxEVT_TEXT, &CScriptCameraWxWindow::OnYawChanged, this );
			pPitch->Bind( wxEVT_TEXT, &CScriptCameraWxWindow::OnPitchChanged, this );
			pFOV->Bind( wxEVT_TEXT, &CScriptCameraWxWindow::OnFOVChanged, this );

			ShowManualControls( NGlobal::GetVar( "enable_movie_manual_controls", 0 ) == 1 );
			bIsDataSetting = false;
			return true;
		}

		//	CScriptCameraCommands
		virtual void GetDialogData( SWindowData *pData )
		{
			if ( pData == 0 )
			{
				return;
			}
			// Only these two, as in the original: the camera list is the
			// state's, and a caller that wants it already has it. With nothing
			// selected nCurrentCamera is left as the caller passed it.
			const long nItem = pCameras->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			if ( nItem >= 0 )
			{
				pData->nCurrentCamera = static_cast<int>( nItem );
			}
			pData->eLastAction = dialogData.eLastAction;
		}

		virtual void SetDialogData( const SWindowData *pData )
		{
			if ( pData == 0 )
			{
				return;
			}
			bIsDataSetting = true;
			dialogData = *pData;

			pCameras->DeleteAllItems();
			for ( size_t nCamera = 0; nCamera < dialogData.scriptCameras.size(); ++nCamera )
			{
				const NCamera::CCameraPlacement &rCamera = dialogData.scriptCameras[nCamera];
				const long nItem = pCameras->InsertItem( nCamera, wxString::FromUTF8( fmt::format( "{}", rCamera.szName ).c_str() ) );
				pCameras->SetItem( nItem, 1, fmt::format( "{:.0f} : {:.0f} : {:.0f}", rCamera.vPosition.x, rCamera.vPosition.y, rCamera.vPosition.z ).c_str() );
				pCameras->SetItem( nItem, 2, fmt::format( "{:.0f}", rCamera.fFOV ).c_str() );
				pCameras->SetItem( nItem, 3, fmt::format( "{:.0f}", rCamera.fYaw ).c_str() );
				pCameras->SetItem( nItem, 4, fmt::format( "{:.0f}", rCamera.fPitch ).c_str() );
				pCameras->SetItemData( nItem, static_cast<long>( nCamera ) );
			}
			// Guarded where the original is not; see the note at the top for why
			// that changes nothing.
			if ( ( dialogData.nCurrentCamera >= 0 ) && ( dialogData.nCurrentCamera < pCameras->GetItemCount() ) )
			{
				const long nStateMask = wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED;
				pCameras->SetItemState( dialogData.nCurrentCamera, nStateMask, nStateMask );
			}

			// On nCurrentCamera rather than on whether the row exists, which is
			// what the original does.
			pSave->Enable( dialogData.nCurrentCamera != -1 );

			dialogData.eLastAction = SWindowData::SCA_NO_ACTIONS;
			bIsDataSetting = false;
		}

		//	ICommandHandler
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			// The original's NI_VERIFY, which refuses every command with a zero
			// dwData before looking at which command it is; see the note at the
			// top.
			if ( dwData == 0 )
			{
				return false;
			}
			switch ( nCommandID )
			{
				case ID_SCRIPT_CAMERA_SHOW_MANUAL_CONTROLS:
					ShowManualControls( true );
					return true;
				case ID_SCRIPT_CAMERA_GET_YAW:
					ReadAngle( pYaw, reinterpret_cast<float*>( dwData ) );
					return true;
				case ID_SCRIPT_CAMERA_GET_PITCH:
					ReadAngle( pPitch, reinterpret_cast<float*>( dwData ) );
					return true;
				case ID_SCRIPT_CAMERA_GET_FOV:
				{
					// %d, not %g: FOV travels as an int.
					int *pFOVValue = reinterpret_cast<int*>( dwData );
					const std::string szText( pFOV->GetValue().utf8_str() );
					int nFOV = *pFOVValue;
					if ( ( sscanf( szText.c_str(), "%d", &nFOV ) == 1 ) && ( nFOV > 0 ) && ( nFOV <= 179 ) )
					{
						*pFOVValue = nFOV;
					}
					return true;
				}
				case ID_SCRIPT_CAMERA_SET_YAW:
					WriteQuietly( pYaw, fmt::format( "{:g}", *reinterpret_cast<const float*>( dwData ) ) );
					return true;
				case ID_SCRIPT_CAMERA_SET_PITCH:
					WriteQuietly( pPitch, fmt::format( "{:g}", *reinterpret_cast<const float*>( dwData ) ) );
					return true;
				case ID_SCRIPT_CAMERA_SET_FOV:
					WriteQuietly( pFOV, std::to_string( *reinterpret_cast<const int*>( dwData ) ) );
					return true;
				default:
					return CScriptCameraCommands::HandleCommand( nCommandID, dwData );
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			NI_ASSERT( pbEnable != 0, "CScriptCameraWxWindow::UpdateCommand(), pbEnable == 0" );
			NI_ASSERT( pbCheck != 0, "CScriptCameraWxWindow::UpdateCommand(), pbCheck == 0" );
			//
			switch ( nCommandID )
			{
				case ID_SCRIPT_CAMERA_SHOW_MANUAL_CONTROLS:
				case ID_SCRIPT_CAMERA_GET_YAW:
				case ID_SCRIPT_CAMERA_SET_YAW:
				case ID_SCRIPT_CAMERA_GET_PITCH:
				case ID_SCRIPT_CAMERA_SET_PITCH:
				case ID_SCRIPT_CAMERA_GET_FOV:
				case ID_SCRIPT_CAMERA_SET_FOV:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				default:
					return CScriptCameraCommands::UpdateCommand( nCommandID, pbEnable, pbCheck );
			}
			return false;
		}

		void OnDebounce( unsigned nCommandID )
		{
			// One-shot, so there is nothing to kill: the original's
			// KillYawTimer() here was only ever turning off a repeating timer
			// after its first tick.
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE, nCommandID, 0 );
		}

	protected:
		virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
		{
			if ( message == WM_DESTROY )
			{
				// Before the wx side comes down: a debounce firing after the
				// palette is gone would ask the state to read a box that no
				// longer exists.
				StopTimers();
			}
			return CWxHostWindow::WindowProc( message, wParam, lParam );
		}

	private:
		wxTextCtrl* AddManualRow( wxFlexGridSizer *pGrid, const char *pszLabel )
		{
			pGrid->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, pszLabel ),
									wxSizerFlags().CentreVertical() );
			wxTextCtrl *pEdit = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			pGrid->Add( pEdit, wxSizerFlags().Expand() );
			pGrid->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "dg." ),
									wxSizerFlags().CentreVertical() );
			return pEdit;
		}

		void ShowManualControls( bool bShow )
		{
			pSizer->Show( pManualControls, bShow, true );
			pRoot->FitInside();
			pRoot->Layout();
		}

		void StopTimers()
		{
			yawTimer.Stop();
			pitchTimer.Stop();
			fovTimer.Stop();
		}

		// ID_SCRIPT_CAMERA_GET_YAW and _PITCH: the box parsed as %g into what
		// the pointer points at, but only if it is a number in (0, 179]. The
		// caller's value is left alone otherwise.
		static void ReadAngle( const wxTextCtrl *pEdit, float *pValue )
		{
			const std::string szText( pEdit->GetValue().utf8_str() );
			float fValue = *pValue;
			if ( ( sscanf( szText.c_str(), "%g", &fValue ) == 1 ) && ( fValue > 0 ) && ( fValue <= 179 ) )
			{
				*pValue = fValue;
			}
		}

		// A box filled from the state must not start the debounce that reports
		// the box back to the state; bIsDataSetting is what stops it.
		void WriteQuietly( wxTextCtrl *pEdit, const std::string &rszValue )
		{
			bIsDataSetting = true;
			pEdit->SetValue( wxString::FromUTF8( rszValue.c_str() ) );
			bIsDataSetting = false;
		}

		void NotifyHandler( SWindowData::EScriptCameraLastAction eAction )
		{
			dialogData.eLastAction = eAction;
			if ( !bIsDataSetting )
			{
				wxBusyCursor waitCursor;
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE,
																															ID_SCRIPT_CAMERA_WINDOW_UI_EVENT, 0 );
			}
			dialogData.eLastAction = SWindowData::SCA_NO_ACTIONS;
		}

		void Restart( CDebounceTimer &rTimer )
		{
			if ( !bIsDataSetting )
			{
				rTimer.StartOnce( MANUAL_CONTROL_DEBOUNCE );
			}
		}

		void OnSave( wxCommandEvent& )
		{
			NotifyHandler( SWindowData::SCA_CAMERA_SAVE );
		}

		void OnCameraSelected( wxListEvent &rEvent )
		{
			// Not guarded by bIsDataSetting, as in the original: selecting a row
			// while the list is being refilled updates nCurrentCamera and the
			// Save button, and only the notification is held back.
			dialogData.nCurrentCamera = static_cast<int>( rEvent.GetIndex() );
			pSave->Enable( dialogData.nCurrentCamera != -1 );
			NotifyHandler( SWindowData::SCA_CAMERA_CHANGE );
		}

		void OnCameraDoubleClick( wxMouseEvent &rEvent )
		{
			// Let the list do its own double-click handling first, which is what
			// selects the row under the pointer.
			rEvent.Skip();
			int nFlags = 0;
			if ( pCameras->HitTest( rEvent.GetPosition(), nFlags ) >= 0 )
			{
				NotifyHandler( SWindowData::SCA_CAMERA_JUMP );
			}
		}

		// The 13/42/15/15/15 split the original computes once in OnInitDialog,
		// applied every time the list changes width instead.
		void OnCameraListSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			const int nWidth = pCameras->GetClientSize().GetWidth();
			if ( nWidth > 0 )
			{
				static const float COLUMN_SHARE[5] = { 0.13f, 0.42f, 0.15f, 0.15f, 0.15f };
				for ( int nColumn = 0; nColumn < 5; ++nColumn )
				{
					pCameras->SetColumnWidth( nColumn, (int)( nWidth * COLUMN_SHARE[nColumn] ) );
				}
			}
		}

		void OnYawChanged( wxCommandEvent& ) { Restart( yawTimer ); }
		void OnPitchChanged( wxCommandEvent& ) { Restart( pitchTimer ); }
		void OnFOVChanged( wxCommandEvent& ) { Restart( fovTimer ); }
	};
}


namespace NScriptCameraView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CScriptCameraWxWindow *pWindow = pTabWindow->AddNewTab( new CScriptCameraWxWindow() );
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
