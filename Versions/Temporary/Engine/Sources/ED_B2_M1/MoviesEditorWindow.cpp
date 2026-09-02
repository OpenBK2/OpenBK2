#include "stdafx.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/EditParameter.h"
#include "SceneB2/CameraScriptMutators.h"
#include "CommandHandlerDefines.h"
#include "MovEditorSettingsWindow.h"
#include "ED_B2_M1Dll.h"

#include "MoviesEditorWindow.h"

#include <cstdint>

#define MOVED_DEF_SCALING (2.0f)

static float GetSliderPercent( const CSliderCtrl &slider )
{
	int nMin, nMax;
	slider.GetRange( nMin, nMax );

	return (float)(slider.GetPos()) / (nMax - nMin);
}


//
//
//		MOVIES EDITOR WINDOW
//
//

BEGIN_MESSAGE_MAP( CMoviesEditorWindow, CResizeDialog )
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_DMOVED_JUMP_FIRST_KEY_BUTTON, OnBnClickedJumpFirstKey)
	ON_BN_CLICKED(IDC_DMOVED_JUMP_LAST_KEY_BUTTON, OnBnClickedJumpLastKey)
	ON_BN_CLICKED(IDC_DMOVED_STEP_PREV_KEY_BUTTON, OnBnClickedStepPrevKey)
	ON_BN_CLICKED(IDC_DMOVED_STEP_NEXT_KEY_BUTTON, OnBnClickedStepNextKey)
	ON_BN_CLICKED(IDC_DMOVED_STOP_MOVIE_BUTTON, OnBnClickedStopMovie)
	ON_BN_CLICKED(IDC_DMOVED_PLAY_PAUSE_MOVIE_BUTTON, OnBnClickedPlayPauseMovie)
	ON_BN_CLICKED(IDC_DMOVED_ADD_SEQ_BUTTON, OnBnClickedAddSeq)
	ON_BN_CLICKED(IDC_DMOVED_DEL_SEQ_BUTTON, OnBnClickedDelSeq)
	ON_BN_CLICKED(IDC_DMOVED_PLAY_PAUSE_MOVIE_BUTTON, OnBnClickedPlayPauseMovie)
	ON_BN_CLICKED(IDC_DMOVED_SETTINGS_BUTTON, OnBnClickedSettings)
	ON_CBN_SELCHANGE(IDC_DMOVED_MOV_SELECT_COMBO, OnCbnSelchangeComboMovie)
	ON_CBN_SELCHANGE(IDC_MOVED_SPEED_COMBO, OnCbnSelChangeTimerSpeed)
	ON_BN_CLICKED(IDC_DMOVED_CHECK_RESIZE_WND, OnBnClickedDmovedResize)
END_MESSAGE_MAP()


CMoviesEditorWindow::CMoviesEditorWindow( CWnd *pParentWindow )
	:	CResizeDialog( CMoviesEditorWindow::IDD, pParentWindow ),
	bIsDataSetting( false ),
	nMovieMode( MOVIE_IS_STOPPED ),
	nSliderMode( SLI_MODE_NOTHING ),
	bIsSliderEnabled( false )
{
	SetControlStyle( IDC_DMOVED_JUMP_FIRST_KEY_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_JUMP_LAST_KEY_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_STEP_PREV_KEY_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_STEP_NEXT_KEY_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_STOP_MOVIE_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_PLAY_PAUSE_MOVIE_BUTTON, ANCHORE_RIGHT_BOTTOM );
	//
	SetControlStyle( IDC_DMOVED_MOV_LABEL, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_MOV_SELECT_COMBO, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_ADD_SEQ_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_DEL_SEQ_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_SETTINGS_BUTTON, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_CHECK_RESIZE_WND, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_TIME_EDIT, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_MOVED_SPEED_COMBO, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_DMOVED_SPEED_LABEL, ANCHORE_RIGHT_BOTTOM );
	//
	SetControlStyle( IDC_DMOVED_TIME_SLIDER_PLACE, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	//
	SetControlStyle( IDC_DMOVED_TIME_SLIDER, ANCHORE_LEFT_BOTTOM | RESIZE_HOR, 0.5f, 1.0f, 0.7f, 0.5f );
	SetControlStyle( IDC_DMOVED_SCALE_SLIDER, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 1.0f, 0.3f, 0.5f );

	Singleton<ICommandHandlerContainer>()->Set( CHID_MOVIES_EDITOR_WINDOW, this );
}


CMoviesEditorWindow::~CMoviesEditorWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MOVIES_EDITOR_WINDOW );
}


void CMoviesEditorWindow::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_DMOVED_JUMP_FIRST_KEY_BUTTON, buttons[SB_JUMP_FIRST] );
	DDX_Control( pDX, IDC_DMOVED_JUMP_LAST_KEY_BUTTON, buttons[SB_JUMP_LAST] );
	DDX_Control( pDX, IDC_DMOVED_STEP_PREV_KEY_BUTTON, buttons[SB_STEP_PREV] );
	DDX_Control( pDX, IDC_DMOVED_STEP_NEXT_KEY_BUTTON, buttons[SB_STEP_NEXT] );
	DDX_Control( pDX, IDC_DMOVED_STOP_MOVIE_BUTTON, buttons[SB_STOP] );
	DDX_Control( pDX, IDC_DMOVED_PLAY_PAUSE_MOVIE_BUTTON, buttons[SB_PLAY] );
	DDX_Control( pDX, IDC_DMOVED_ADD_SEQ_BUTTON, buttons[SB_ADD_SEQ] );
	DDX_Control( pDX, IDC_DMOVED_DEL_SEQ_BUTTON, buttons[SB_DEL_SEQ] );
	DDX_Control( pDX, IDC_DMOVED_SETTINGS_BUTTON, buttons[SB_SETUP] );
	DDX_Control( pDX, IDC_DMOVED_CHECK_RESIZE_WND, buttons[SB_RESIZE_WND] );
	DDX_Control( pDX, IDC_DMOVED_TIME_SLIDER, wndSliderTime );
	DDX_Control( pDX, IDC_DMOVED_SCALE_SLIDER, wndSliderScale );
	DDX_Control( pDX, IDC_DMOVED_MOV_SELECT_COMBO, wndMovieCombo );
	DDX_Control( pDX, IDC_DMOVED_TIME_SLIDER_PLACE, wndTimeSliderControl );
	DDX_Control( pDX, IDC_DMOVED_TIME_EDIT, wndTimeEdit );
	DDX_Control( pDX, IDC_MOVED_SPEED_COMBO, wndSpeedCombo );
}


bool CMoviesEditorWindow::Create( CWnd *pParentWindow )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MOVIES_EDITOR_WINDOW, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MOVIES_EDITOR_WINDOW, ID_MIMOVED_INSERT_KEY, ID_MIMOVED_DELETE_KEYS );

	return CResizeDialog::Create( IDD, pParentWindow );
}


void CMoviesEditorWindow::Destroy()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MOVIES_EDITOR_WINDOW );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MOVIES_EDITOR_WINDOW );

	CResizeDialog::DestroyWindow();
}


BOOL CMoviesEditorWindow::OnInitDialog()
{
	if ( !CResizeDialog::OnInitDialog() )
		return FALSE;

	bitmaps[SB_JUMP_FIRST].LoadBitmap( IDB_DMOVED_JUMP_FIRST_KEY );
	bitmaps[SB_JUMP_LAST].LoadBitmap( IDB_DMOVED_JUMP_LAST_KEY );
	bitmaps[SB_STEP_PREV].LoadBitmap( IDB_DMOVED_STEP_PREV_KEY );
	bitmaps[SB_STEP_NEXT].LoadBitmap( IDB_DMOVED_STEP_NEXT_KEY );
	bitmaps[SB_STOP].LoadBitmap( IDB_DMOVED_STOP_MOVIE );
	bitmaps[SB_PLAY].LoadBitmap( IDB_DMOVED_PLAY_MOVIE );
	bitmaps[SB_PAUSE].LoadBitmap( IDB_DMOVED_PAUSE_MOVIE );
	bitmaps[SB_ADD_SEQ].LoadBitmap( IDB_DMOVED_ADD_SEQ );
	bitmaps[SB_DEL_SEQ].LoadBitmap( IDB_DMOVED_DEL_SEQ );
	bitmaps[SB_SETUP].LoadBitmap( IDB_DMOVED_SETTINGS );
	bitmaps[SB_RESIZE_WND].LoadBitmap( IDB_DMOVED_CHECK_RESIZE_WND );

	buttons[SB_JUMP_FIRST].SetBitmap( bitmaps[SB_JUMP_FIRST] );
	buttons[SB_JUMP_LAST].SetBitmap( bitmaps[SB_JUMP_LAST] );
	buttons[SB_STEP_PREV].SetBitmap( bitmaps[SB_STEP_PREV] );
	buttons[SB_STEP_NEXT].SetBitmap( bitmaps[SB_STEP_NEXT] );
	buttons[SB_STOP].SetBitmap( bitmaps[SB_STOP] );
	buttons[SB_PLAY].SetBitmap( bitmaps[SB_PLAY] );
	buttons[SB_ADD_SEQ].SetBitmap( bitmaps[SB_ADD_SEQ] );
	buttons[SB_DEL_SEQ].SetBitmap( bitmaps[SB_DEL_SEQ] );
	buttons[SB_SETUP].SetBitmap( bitmaps[SB_SETUP] );
	buttons[SB_RESIZE_WND].SetBitmap( bitmaps[SB_RESIZE_WND] );

	wndTimeSliderControl.SetStartTime( 0.0f );
	wndSliderScale.SetPos( wndSliderScale.GetRangeMax() );

	// timer speed combo
	std::vector<std::string> speeds( 0 );
	for ( int nAnimSpeedIndex = -10; nAnimSpeedIndex <= 10; ++nAnimSpeedIndex )
	{
		if ( nAnimSpeedIndex > 0 )
			speeds.push_back( StrFmt( "+%d", nAnimSpeedIndex ) );
		else
			speeds.push_back( std::to_string(  nAnimSpeedIndex ) );
	}
	SetComboBoxEditParameters( speeds, 10, &wndSpeedCombo, true, true );

	return TRUE;
}


bool CMoviesEditorWindow::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch ( nCommandID )
	{
		case ID_WINDOW_GET_DIALOG_DATA:
		{
			SScriptMovieEditorData *pData = reinterpret_cast<SScriptMovieEditorData*>( dwData );
			NI_VERIFY( pData, "SScriptCameraWindowData::HandleCommand(): dwData == 0", return false );
			//
			GetDialogData( pData );
			return true;
		}
		//
		case ID_WINDOW_SET_DIALOG_DATA:
		{
			SScriptMovieEditorData *pData = reinterpret_cast<SScriptMovieEditorData*>( dwData );
			NI_VERIFY( pData, "SScriptCameraWindowData::HandleCommand(): dwData == 0", return false );
			//
			SetDialogData( *pData );
			return true;
		}
		//
		case ID_MOV_ED_SET_TIMER:
		{
			SetMovieTimer();
			return true;
		}
		//
		case ID_MOV_ED_KILL_TIMER:
		{
			KillMovieTimer();
			return true;
		}
		//
		case ID_MOV_ED_RESET_DIALOG:
		{
			KillMovieTimer();
			return true;
		}
		//
		case ID_MIMOVED_INSERT_KEY:
		{
			NotifyHandler( SScriptMovieEditorData::ME_INSERT_KEY );
			return true;
		}
		//
		case ID_MIMOVED_SAVE_KEY:
		{
			NotifyHandler( SScriptMovieEditorData::ME_SAVE_KEY );
			return true;
		}
		//
		case ID_MIMOVED_KEY_SETTINGS:
		{
			NotifyHandler( SScriptMovieEditorData::ME_KEY_SETTINGS );
			return true;
		}
		//
		case ID_MIMOVED_DELETE_KEYS:
		{
			NotifyHandler( SScriptMovieEditorData::ME_DELETE_KEYS );
			return true;
		}
	}
	return false;
}


bool CMoviesEditorWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMoviesEditorWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMoviesEditorWindow::UpdateCommand(), pbCheck == 0" );

	switch( nCommandID )
	{
		case ID_MIMOVED_INSERT_KEY:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		//
		case ID_MIMOVED_SAVE_KEY:
		{
			(*pbEnable) = wndTimeSliderControl.HasSingleActiveKey();
			(*pbCheck) = false;
			return true;
		}
		//
		case ID_MIMOVED_KEY_SETTINGS:
		{
			(*pbEnable) = wndTimeSliderControl.HasSingleActiveKey();
			(*pbCheck) = false;
			return true;
		}
		//
		case ID_MIMOVED_DELETE_KEYS:
		{
			(*pbEnable) = wndTimeSliderControl.HasActiveKeys();
			(*pbCheck) = false;
			return true;
		}
		//
		default:
			return false;
	}
	return false;
}


void CMoviesEditorWindow::NotifyHandler()
{
	if ( !bIsDataSetting )
	{
		CWaitCursor wcur;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE, ID_SCRIPT_CAMERA_MOV_ED_UI_EVENT, 0 );
	}
}


// useful NotifyHandler wrapper
void CMoviesEditorWindow::NotifyHandler( SScriptMovieEditorData::EMoviesEditorLastAction eAction )
{
	SetLastAction( eAction );
	NotifyHandler();
	SetLastAction( SScriptMovieEditorData::ME_NO_ACTIONS );
}


void CMoviesEditorWindow::GetTimeSliderRect( CRect* pRect ) const
{
	NI_VERIFY( pRect, "Return pointer pRect == 0!", return );

	if ( CWnd *pWnd = GetDlgItem(IDC_DMOVED_TIME_SLIDER_PLACE) )
	{
		pWnd->GetWindowRect( pRect );
		ScreenToClient( pRect );
		return;
	}
	pRect->SetRectEmpty();
}


void CMoviesEditorWindow::OnLButtonDown( unsigned nFlags, CPoint point )
{
	SetCapture();
	CResizeDialog::OnLButtonDown( nFlags, point );

	CRect customRect;
	GetTimeSliderRect( &customRect );
	if ( bIsSliderEnabled && customRect.PtInRect(point) )
	{
		if ( nMovieMode & MOVIE_IS_PLAYING )
			NotifyHandler( SScriptMovieEditorData::ME_PAUSE );

		const CPoint realPoint( point.x - customRect.left, point.y - customRect.top );
		const float fTime = GetTimeUnderCursor( customRect, realPoint );

		if ( nFlags & MK_SHIFT )
		// change selection start and finish
		{
			//wndTimeSliderControl.SetSelectionMode( true );
			wndTimeSliderControl.SetSelectionStart( fTime );
			wndTimeSliderControl.SetSelectionLength( DEF_SEL_RAD );
			wndTimeSliderControl.UpdateSelection();

			nSliderMode |= SLI_MODE_SET_SELECTION;
			//DebugTrace( "MoviesEditor: start setting selection\n" );
		}
		else
		{
			if ( HasSelection() && wndTimeSliderControl.IsPointInsideSelection(fTime) )
			// move selection
			{
				wndTimeSliderControl.SetMoveFrom( fTime );
				wndTimeSliderControl.SetMoveTo( fTime );

				nSliderMode |= SLI_MODE_MOVE_SELECTION;
				//DebugTrace( "MoviesEditor: start moving selected key\n" );
			}
			else
			// just change time
			{
				wndTimeSliderControl.ResetSelection();
				wndTimeSliderControl.UpdateSelection();
				wndTimeSliderControl.SetCursorPos( fTime );
				NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
				NotifyHandler( SScriptMovieEditorData::ME_CLEAR_MARKERS );

				nSliderMode |= SLI_MODE_MOVE_CURSOR;
				//DebugTrace( "MoviesEditor: start simply change time\n" );
			}
		}
	}

	lastPoint = point;
}


void CMoviesEditorWindow::OnLButtonUp( unsigned nFlags, CPoint point )
{
	ReleaseCapture();
	CResizeDialog::OnLButtonUp( nFlags, point );

	if ( bIsSliderEnabled )
	{
		if ( nMovieMode & MOVIE_IS_PLAYING )
			NotifyHandler( SScriptMovieEditorData::ME_PLAY );

		if ( nSliderMode & SLI_MODE_SET_SELECTION )
		{
			nSliderMode = nSliderMode & !SLI_MODE_SET_SELECTION;
			//DebugTrace( "MoviesEditor: stop selection setting\n" );
		}
		if ( nSliderMode & SLI_MODE_MOVE_SELECTION )
		{
			wndTimeSliderControl.SetSelectionStart( wndTimeSliderControl.GetSelectionStart() + wndTimeSliderControl.GetMoveOffset() );
			NotifyHandler( SScriptMovieEditorData::ME_MOVE_KEYS );

			wndTimeSliderControl.SetMoveFrom( 0.0f );
			wndTimeSliderControl.SetMoveTo( 0.0f );
			nSliderMode = nSliderMode & !SLI_MODE_MOVE_SELECTION;
			//DebugTrace( "MoviesEditor: stop moving selection\n" );
		}
		if ( nSliderMode & SLI_MODE_MOVE_CURSOR )
		{
			if ( nMovieMode & !MOVIE_IS_PLAYING )
				NotifyHandler( SScriptMovieEditorData::ME_DRAW_MARKERS );
			NotifyHandler( SScriptMovieEditorData::ME_SELECT_CAMERA );

			nSliderMode = nSliderMode & !SLI_MODE_MOVE_CURSOR;
			//DebugTrace( "MoviesEditor: stop moving cursor\n" );
		}
	}

	lastPoint = point;
}


void CMoviesEditorWindow::OnMouseMove( unsigned nFlags, CPoint point )
{
	CResizeDialog::OnMouseMove( nFlags, point );

	CRect customRect;
	GetTimeSliderRect( &customRect );

	if ( bIsSliderEnabled )
	{
		const CPoint realPoint( point.x - customRect.left, point.y - customRect.top );
		const float fCursorTime = GetTimeUnderCursor( customRect, realPoint );

		if ( nSliderMode & SLI_MODE_MOVE_CURSOR )
		{
			wndTimeSliderControl.ResetSelection();
			wndTimeSliderControl.SetCursorPos( fCursorTime );
			NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
			//DebugTrace( "MoviesEditor: simply change time\n" );
		}
		if ( nSliderMode & SLI_MODE_SPECIAL_MOVE )
		{
			wndTimeSliderControl.ResetSelection();
			wndTimeSliderControl.SetCursorPos( fCursorTime );
			wndTimeSliderControl.UpdateSelection();
			//DebugTrace( "MoviesEditor: special time change\n" );
		}
		if ( nSliderMode & SLI_MODE_SET_SELECTION )
		{
			wndTimeSliderControl.SetSelectionLength( fCursorTime - wndTimeSliderControl.GetSelectionStart() );
			wndTimeSliderControl.SetCursorPos( fCursorTime );
			wndTimeSliderControl.UpdateSelection();
			//DebugTrace( "MoviesEditor: change keys selection\n" );
		}
		if ( nSliderMode & SLI_MODE_MOVE_SELECTION )
		{
			//wndTimeSliderControl.SetSelectionStart( fCursorTime );
			wndTimeSliderControl.SetMoveTo( fCursorTime );
			//DebugTrace( "MoviesEditor: move selection\n" );
		}
	}

	lastPoint = point;
}


void CMoviesEditorWindow::OnMButtonDown( unsigned nFlags, CPoint point )
{
	SetCapture();
	CResizeDialog::OnMButtonDown( nFlags, point );

	// special move cursor
	CRect customRect;
	GetTimeSliderRect( &customRect );
	if ( bIsSliderEnabled && customRect.PtInRect(point) )
	{
		const CPoint realPoint( point.x - customRect.left, point.y - customRect.top );
		const float fTime = GetTimeUnderCursor( customRect, realPoint );

		wndTimeSliderControl.ResetSelection();
		wndTimeSliderControl.SetCursorPos( fTime );
		wndTimeSliderControl.UpdateSelection();

		nSliderMode |= SLI_MODE_SPECIAL_MOVE;
		//DebugTrace( "MoviesEditor: start special move\n" );
		return;
	}

}


void CMoviesEditorWindow::OnMButtonUp( unsigned nFlags, CPoint point )
{
	ReleaseCapture();
	CResizeDialog::OnMButtonUp( nFlags, point );

	if ( bIsSliderEnabled )
	{
		NotifyHandler( SScriptMovieEditorData::ME_SELECT_CAMERA );
		nSliderMode = nSliderMode & !SLI_MODE_SPECIAL_MOVE;
		//DebugTrace( "MoviesEditor: stop special move\n" );
	}
}


BOOL CMoviesEditorWindow::OnMouseWheel( unsigned nFlags, short zDelta, CPoint point )
{
	// special move cursor
	CRect customRect;
	GetTimeSliderRect( &customRect );
	if ( bIsSliderEnabled && customRect.PtInRect(point) )
	{
		const CPoint realPoint( point.x - customRect.left, point.y - customRect.top );
		const float fTime = GetTimeUnderCursor( customRect, realPoint );

	}
	else
	{
		return CResizeDialog::OnMouseWheel( nFlags, zDelta, point );
	}
	//
	return TRUE;
}


void CMoviesEditorWindow::OnTimer( unsigned nIDEvent )
{
	if ( nIDEvent == GetMovieTimerID() )
	{
		UpdateDialogData();
	}
}


void CMoviesEditorWindow::OnBnClickedJumpFirstKey()
{
	wndTimeSliderControl.ResetSelection();
	wndTimeSliderControl.SetCursorPos( wndTimeSliderControl.GetFirstKeyTime() );
	NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
	NotifyHandler( SScriptMovieEditorData::ME_SELECT_CAMERA );
	//DebugTrace( "MoviesEditor: jump first key\n" );
}


void CMoviesEditorWindow::OnBnClickedJumpLastKey()
{
	wndTimeSliderControl.ResetSelection();
	wndTimeSliderControl.SetCursorPos( wndTimeSliderControl.GetLastKeyTime() );
	NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
	NotifyHandler( SScriptMovieEditorData::ME_SELECT_CAMERA );
	//DebugTrace( "MoviesEditor: jump last key\n" );
}


void CMoviesEditorWindow::OnBnClickedStepPrevKey()
{
	wndTimeSliderControl.ResetSelection();
	wndTimeSliderControl.SetCursorPos( wndTimeSliderControl.GetPrevKeyTime() );
	NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
	NotifyHandler( SScriptMovieEditorData::ME_SELECT_CAMERA );
	//DebugTrace( "MoviesEditor: step previous key\n" );
}


void CMoviesEditorWindow::OnBnClickedStepNextKey()
{
	wndTimeSliderControl.ResetSelection();
	wndTimeSliderControl.SetCursorPos( wndTimeSliderControl.GetNextKeyTime() );
	NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
	NotifyHandler( SScriptMovieEditorData::ME_SELECT_CAMERA );
	//DebugTrace( "MoviesEditor: step next key\n" );
}


void CMoviesEditorWindow::OnBnClickedAddSeq()
{
	NotifyHandler( SScriptMovieEditorData::ME_ADD_SEQ );
}


void CMoviesEditorWindow::OnBnClickedDelSeq()
{
	NotifyHandler( SScriptMovieEditorData::ME_DEL_SEQ );
}


void CMoviesEditorWindow::OnBnClickedStopMovie()
{
	NotifyHandler( SScriptMovieEditorData::ME_STOP );

	buttons[SB_PLAY].SetBitmap( bitmaps[SB_PLAY] );
	//wndTimeSliderControl.SetCursorPos( 0.0f );
	nMovieMode ^= MOVIE_IS_PLAYING;

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
}


void CMoviesEditorWindow::OnBnClickedPlayPauseMovie()
{
	if ( nMovieMode & MOVIE_IS_PLAYING )
	{
		NotifyHandler( SScriptMovieEditorData::ME_STOP );
		buttons[SB_PLAY].SetBitmap( bitmaps[SB_PLAY] );
	}
	else
	{
		NotifyHandler( SScriptMovieEditorData::ME_PLAY );
		buttons[SB_PLAY].SetBitmap( bitmaps[SB_STOP] );
	}
	nMovieMode ^= MOVIE_IS_PLAYING;
}


void CMoviesEditorWindow::OnBnClickedSettings()
{
	if ( (dialogData.nActiveMovie >= 0) && (dialogData.nActiveMovie < dialogData.scriptMoviesData.scriptMovieSequences.size()) )
	{
		const NDb::SScriptMovieSequence &seq = dialogData.scriptMoviesData.scriptMovieSequences[dialogData.nActiveMovie];
		fNewLength = seq.GetLength();
	}
	CMovEditorSettingsDlg dlg(	Singleton<IMainFrameContainer>()->GetSECWorkbook(), &fNewLength );

	if ( dlg.DoModal() == IDOK )
	{
		NotifyHandler( SScriptMovieEditorData::ME_RESIZE );
	}
}


void CMoviesEditorWindow::OnBnClickedDmovedResize()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_RESIZE_TO_GAME, buttons[SB_RESIZE_WND].GetCheck() );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
}


void CMoviesEditorWindow::OnCbnSelchangeComboMovie()
{
	CComboBox *pWndMovieCombo = static_cast<CComboBox*>( GetDlgItem(IDC_DMOVED_MOV_SELECT_COMBO) );
	if ( pWndMovieCombo )
	{
		dialogData.nActiveMovie = pWndMovieCombo->GetCurSel();

		NotifyHandler( SScriptMovieEditorData::ME_MOVIE_SWITCH );
	}
}


void CMoviesEditorWindow::OnCbnSelChangeTimerSpeed()
{
	NotifyHandler( SScriptMovieEditorData::ME_CHANGE_SPEED );
}


void CMoviesEditorWindow::GetDialogData( SScriptMovieEditorData *pDialogData )
{
	pDialogData->Clear();
	(*pDialogData) = dialogData;

	pDialogData->nActiveMovie = wndMovieCombo.GetCurSel();
	pDialogData->fCursorTime = (float)( wndTimeSliderControl.GetCursorPos() );
	pDialogData->eLastAction = eLastAction;
	pDialogData->nSpeed = wndSpeedCombo.GetCurSel() - 10;

	pDialogData->activeKeysList.SetSize( wndTimeSliderControl.data.keys.size() );
	pDialogData->activeKeysList.FillZero();
	int i = 0;
	for ( std::vector<SMovieKeyData>::const_iterator itKey = wndTimeSliderControl.data.keys.begin();
																							itKey != wndTimeSliderControl.data.keys.end(); ++itKey, ++i )
	{
		if ( itKey->bActive )
		{
			pDialogData->activeKeysList.SetData( i );
		}
	}
	pDialogData->fMoveValue = wndTimeSliderControl.GetMoveOffset();
	pDialogData->fNewLength = fNewLength;
}


void CMoviesEditorWindow::SetDialogData( const SScriptMovieEditorData &rDialogData )
{
	bIsDataSetting = true;

	dialogData = rDialogData;
	const bool bSequenceExists = ( dialogData.scriptMoviesData.scriptMovieSequences.size() > 0 );

	wndSliderTime.EnableWindow( bSequenceExists );
	wndSliderScale.EnableWindow( bSequenceExists );
	wndMovieCombo.EnableWindow( bSequenceExists );
	buttons[SB_JUMP_FIRST].EnableWindow( bSequenceExists );
	buttons[SB_JUMP_LAST].EnableWindow( bSequenceExists );
	buttons[SB_STEP_PREV].EnableWindow( bSequenceExists );
	buttons[SB_STEP_NEXT].EnableWindow( bSequenceExists );
	buttons[SB_STOP].EnableWindow( bSequenceExists );
	buttons[SB_PLAY].EnableWindow( bSequenceExists );
	buttons[SB_DEL_SEQ].EnableWindow( bSequenceExists );
	buttons[SB_SETUP].EnableWindow( bSequenceExists );

	// movie combo
	wndMovieCombo.ResetContent();
	int i = -1;
	for ( i = 0; i < dialogData.scriptMoviesData.scriptMovieSequences.size(); ++i )
	{
		const int nStringNumber = wndMovieCombo.AddString( StrFmt("%d", i) );
		wndMovieCombo.SetItemData( nStringNumber, i );
	}
	if ( (dialogData.nActiveMovie <= i) && (dialogData.nActiveMovie >= 0) )
		wndMovieCombo.SelectString( 0, StrFmt("%d", dialogData.nActiveMovie) );
	else if ( i > 0 )
		wndMovieCombo.SelectString( 0, "0" );

	// timeline
	if ( (dialogData.nActiveMovie >= 0) && (dialogData.nActiveMovie < dialogData.scriptMoviesData.scriptMovieSequences.size()) )
	{
		bIsSliderEnabled = true;
		const NDb::SScriptMovieSequence &seq = dialogData.scriptMoviesData.scriptMovieSequences[dialogData.nActiveMovie];
		wndTimeSliderControl.SetLength( seq.GetLength() );

		wndTimeSliderControl.data.keys.resize( 0 );
		for ( std::vector<NDb::SScriptMovieKeyPos >::const_iterator itKey = seq.posKeys.begin(); itKey != seq.posKeys.end(); ++itKey )
		{
			SMovieKeyData newKey;

			newKey.fTime = itKey->fStartTime;
			newKey.szCameraName = dialogData.scriptMoviesData.scriptCameraPlacements[itKey->nPositionIndex].szName;
			wndTimeSliderControl.data.keys.push_back( newKey );
		}
		wndTimeSliderControl.RefreshSpacing();
		wndTimeSliderControl.UpdateSelection();
	}
	else
	{
		wndTimeSliderControl.ClearControl();
		bIsSliderEnabled = false;
	}

	wndTimeSliderControl.EnableWindow( bIsSliderEnabled );

	bIsDataSetting = false;
	RedrawWindow();
}


void CMoviesEditorWindow::UpdateDialogData()
{
	if ( nMovieMode & MOVIE_IS_PLAYING )
	{
		const CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder();
		if ( pMoviesHolder )
		{
			const float fCurrTime = pMoviesHolder->GetTime();
			wndTimeSliderControl.SetCursorPos( fCurrTime );
		}
	}
	const float fCurrTime = wndTimeSliderControl.GetCursorPos();
	SetDlgItemText( IDC_DMOVED_TIME_EDIT, StrFmt("%g", fCurrTime) );
}


void CMoviesEditorWindow::KillMovieTimer()
{
	if ( nMovieTimerID != 0 )
		KillTimer( nMovieTimerID );

	nMovieTimerID = 0;
}


void CMoviesEditorWindow::SetMovieTimer()
{
	KillMovieTimer();
	nMovieTimerID = SetTimer( GetMovieTimerID(), GetMovieTimerInterval(), 0 );
}


void CMoviesEditorWindow::ResetDialog()
{
	buttons[SB_PLAY].SetBitmap( bitmaps[SB_PLAY] );
}


void CMoviesEditorWindow::OnHScroll( unsigned nSBCode, unsigned nPos, CScrollBar *pScrollBar )
{
	wndTimeSliderControl.SetStartTime( (wndTimeSliderControl.GetLength() * wndTimeSliderControl.GetScale()) * GetSliderPercent(wndSliderTime) );
	wndTimeSliderControl.SetScale( pow(MOVED_DEF_SCALING, (GetSliderPercent(wndSliderScale) - 1.0f) * MOVED_DEF_SCALING) );
	wndTimeSliderControl.RefreshSpacing();

	NotifyHandler( SScriptMovieEditorData::ME_CHANGE_TIME );
}


void CMoviesEditorWindow::OnContextMenu( CWnd *pWnd, CPoint point )
{
	CResizeDialog::OnContextMenu( pWnd, point );

	CRect customRect;
	GetTimeSliderRect( &customRect );
	CPoint pointClient = point;
	ScreenToClient( &pointClient );
	if ( customRect.PtInRect(pointClient) )
	{
		CMenu mainPopupMenu;
		AfxSetResourceHandle( theEDB2M1Instance );
		mainPopupMenu.LoadMenu( IDM_MAPINFO_CONTEXT_MENU );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		CMenu *pMenu = mainPopupMenu.GetSubMenu( MICM_MOVIES_EDITOR );
		if ( pMenu )
		{
			pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
		mainPopupMenu.DestroyMenu();
		wndTimeSliderControl.UpdateSelection();
	}
}



