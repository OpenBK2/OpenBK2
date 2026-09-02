#include "stdafx.h"
#include <fmt/format.h>

#include "ScriptCameraWindow.h"

#include <cstdint>

//
//
//		SCRIPT CAMERA WINDOW
//
//

BEGIN_MESSAGE_MAP(CScriptCameraWindow, CResizeDialog)
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_BN_CLICKED( IDC_SMOV_BUTTON_ADD, OnBnClickedScriptCameraAdd )
	ON_BN_CLICKED( IDC_SMOV_BUTTON_DEL, OnBnClickedScriptCameraDelete )
	ON_BN_CLICKED( IDC_SMOV_BUTTON_SAVE, OnBnClickedScriptCameraSave )
	ON_BN_CLICKED( IDC_SMOV_BUTTON_RUN, OnBnClickedScriptCameraRun )
	ON_NOTIFY( NM_DBLCLK, IDC_SMOV_LIST, OnNMDblclkListScriptCameras )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_SMOV_LIST, OnLvnItemchangedListScriptCameras )
	ON_EN_CHANGE( IDC_SMOV_YAW_EDIT, OnChangeYaw )
	ON_EN_CHANGE( IDC_SMOV_PITCH_EDIT, OnChangePitch )
	ON_EN_CHANGE( IDC_SMOV_FOV_EDIT, OnChangeFOV )
END_MESSAGE_MAP()


CScriptCameraWindow::CScriptCameraWindow( CWnd *pParentWindow )
	:	CResizeDialog( CScriptCameraWindow::IDD, pParentWindow ),
		bIsDataSetting( true ),
		nYawTimerID( 0 ),
		nPitchTimerID( 0 ),
		nFOVTimerID( 0 )
{
	SetControlStyle( IDC_SMOV_BUTTON_ADD, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_SMOV_BUTTON_DEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_SMOV_BUTTON_SAVE, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_SMOV_BUTTON_RUN, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_SMOV_LIST_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_SMOV_LIST, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDC_SMOV_YAW_LABEL_LEFT, ANCHORE_LEFT_BOTTOM );
	SetControlStyle( IDC_SMOV_YAW_EDIT, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDC_SMOV_YAW_LABEL_RIGHT, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_SMOV_PITCH_LABEL_LEFT, ANCHORE_LEFT_BOTTOM );
	SetControlStyle( IDC_SMOV_PITCH_EDIT, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDC_SMOV_PITCH_LABEL_RIGHT, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDC_SMOV_FOV_LABEL_LEFT, ANCHORE_LEFT_BOTTOM );
	SetControlStyle( IDC_SMOV_FOV_EDIT, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDC_SMOV_FOV_LABEL_RIGHT, ANCHORE_RIGHT_BOTTOM );
	//
	dialogData.nCurrentCamera = -1;
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SCRIPT_CAMERA_WINDOW, this );
}


CScriptCameraWindow::~CScriptCameraWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SCRIPT_CAMERA_WINDOW );
}


BOOL CScriptCameraWindow::OnInitDialog()
{
	bIsDataSetting = true;
	CResizeDialog::OnInitDialog();

	lcCameras.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	lcCameras.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	lcCameras.EnableToolTips( TRUE );

	CRect rect;
	lcCameras.GetClientRect( &rect );
	lcCameras.InsertColumn( 1, RCSTR("ID"), LVCFMT_LEFT, rect.Width()*0.13f );
	lcCameras.InsertColumn( 2, RCSTR("Position"), LVCFMT_LEFT, rect.Width()*0.42f );
	lcCameras.InsertColumn( 3, RCSTR("FOV"), LVCFMT_LEFT, rect.Width()*0.15f );
	lcCameras.InsertColumn( 4, RCSTR("Yaw"), LVCFMT_LEFT, rect.Width()*0.15f );
	lcCameras.InsertColumn( 5, RCSTR("Pitch"), LVCFMT_LEFT, rect.Width()*0.15f );
	//
	ShowManualControls( NGlobal::GetVar("enable_movie_manual_controls", 0) == 1 );
	bIsDataSetting = false;

	return TRUE;
}


void CScriptCameraWindow::ShowManualControls( bool bShow )
{
	if ( CWnd* pwndStatic = GetDlgItem( IDC_SMOV_YAW_LABEL_LEFT ) )
		pwndStatic->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
	if ( CWnd* pwndEdit = GetDlgItem( IDC_SMOV_YAW_EDIT ) )
		pwndEdit->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
	if ( CWnd* pwndStatic = GetDlgItem( IDC_SMOV_YAW_LABEL_RIGHT ) )
		pwndStatic->ShowWindow( bShow ? SW_SHOW : SW_HIDE );

	if ( CWnd* pwndStatic = GetDlgItem( IDC_SMOV_PITCH_LABEL_LEFT ) )
		pwndStatic->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
	if ( CWnd* pwndEdit = GetDlgItem( IDC_SMOV_PITCH_EDIT ) )
		pwndEdit->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
	if ( CWnd* pwndStatic = GetDlgItem( IDC_SMOV_PITCH_LABEL_RIGHT ) )
		pwndStatic->ShowWindow( bShow ? SW_SHOW : SW_HIDE );

	if ( CWnd* pwndStatic = GetDlgItem( IDC_SMOV_FOV_LABEL_LEFT ) )
		pwndStatic->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
	if ( CWnd* pwndEdit = GetDlgItem( IDC_SMOV_FOV_EDIT ) )
		pwndEdit->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
	if ( CWnd* pwndStatic = GetDlgItem( IDC_SMOV_FOV_LABEL_RIGHT ) )
		pwndStatic->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
}


void CScriptCameraWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_SMOV_LIST, lcCameras );
	DDX_Control( pDX, IDC_SMOV_BUTTON_SAVE, btnSave );
	DDX_Control( pDX, IDC_SMOV_BUTTON_DEL, btnDel );
	DDX_Control( pDX, IDC_SMOV_BUTTON_RUN, btnRun );
}


bool CScriptCameraWindow::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	SScriptCameraWindowData *pData = reinterpret_cast<SScriptCameraWindowData*>( dwData );
	NI_VERIFY( pData, "SScriptCameraWindowData::HandleCommand(): dwData == 0", return false );

	switch ( nCommandID )
	{
		case ID_WINDOW_GET_DIALOG_DATA:
		{
			GetDialogData( pData );
			return true;
		}
		//
		case ID_WINDOW_SET_DIALOG_DATA:
		{
			SetDialogData( pData );
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_SHOW_MANUAL_CONTROLS:
		{
			ShowManualControls( dwData > 0 );
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_GET_YAW:
		{
			if ( dwData != 0 )
			{
				CString strText;
				GetDlgItemText( IDC_SMOV_YAW_EDIT, strText );
				float fYaw = ( *(float*)( dwData ) );
				if ( (sscanf(strText, "%g", &fYaw) == 1) && (fYaw > 0) && (fYaw <= 179) )
				{
					*(float*)( dwData ) = fYaw;
				}
			}
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_GET_PITCH:
		{
			if ( dwData != 0 )
			{
				CString strText;
				GetDlgItemText( IDC_SMOV_PITCH_EDIT, strText );
				float fPitch = ( *(float*)( dwData ) );
				if ( ( sscanf( strText, "%g", &fPitch ) == 1 ) &&
					( fPitch > 0 ) && 
					( fPitch <= 179 ) )
				{
					*(float*)( dwData ) = fPitch;
				}
			}
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_GET_FOV:
		{
			if ( dwData != 0 )
			{
				CString strText;
				GetDlgItemText( IDC_SMOV_FOV_EDIT, strText );
				int nFOV = ( *(int*)( dwData ) );
				if ( ( sscanf( strText, "%d", &nFOV ) == 1 ) &&
					( nFOV > 0 ) && 
					( nFOV <= 179 ) )
				{
					*(int*)( dwData ) = nFOV;
				}
			}
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_SET_YAW:
		{
			if ( dwData > 0 )
			{
				bIsDataSetting = true;
				SetDlgItemText( IDC_SMOV_YAW_EDIT, fmt::format("{:g}", *(float*)(dwData)).c_str() );
				bIsDataSetting = false;
			}
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_SET_PITCH:
		{
			if ( dwData > 0 )
			{
				bIsDataSetting = true;
				SetDlgItemText( IDC_SMOV_PITCH_EDIT, fmt::format( "{:g}", *(float*)( dwData ) ).c_str() );
				bIsDataSetting = false;
			}
			return true;
		}
		//
		case ID_SCRIPT_CAMERA_SET_FOV:
		{
			if ( dwData > 0 )
			{
				bIsDataSetting = true;
				SetDlgItemText( IDC_SMOV_FOV_EDIT, std::to_string(  *(int*)( dwData ) ).c_str() );
				bIsDataSetting = false;
			}
			return true;
		}
	}

	return false;
}


bool CScriptCameraWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CScriptCameraWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CScriptCameraWindow::UpdateCommand(), pbCheck == 0" );

	switch( nCommandID ) 
	{
	case ID_WINDOW_GET_DIALOG_DATA:
	case ID_WINDOW_SET_DIALOG_DATA:
	case ID_SCRIPT_CAMERA_SHOW_MANUAL_CONTROLS:
	case ID_SCRIPT_CAMERA_GET_YAW:
	case ID_SCRIPT_CAMERA_SET_YAW:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCRIPT_CAMERA_GET_PITCH:
	case ID_SCRIPT_CAMERA_SET_PITCH:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCRIPT_CAMERA_GET_FOV:
	case ID_SCRIPT_CAMERA_SET_FOV:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}
	//
	return false;
}


void CScriptCameraWindow::NotifyHandler()
{
	if ( bIsDataSetting )
		return;	// to disable messages during controls setting

	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE, 
																												ID_SCRIPT_CAMERA_WINDOW_UI_EVENT, 0 );
}


// useful NotifyHandler
void CScriptCameraWindow::NotifyHandler( SScriptCameraWindowData::EScriptCameraLastAction eAction )
{
	SetLastAction( eAction );
	NotifyHandler();
	SetLastAction( SScriptCameraWindowData::SCA_NO_ACTIONS );
}


void CScriptCameraWindow::OnBnClickedScriptCameraAdd()
{
	NotifyHandler( SScriptCameraWindowData::SCA_CAMERA_ADD );
}


void CScriptCameraWindow::OnBnClickedScriptCameraSave()
{
	NotifyHandler( SScriptCameraWindowData::SCA_CAMERA_SAVE);
}


void CScriptCameraWindow::OnBnClickedScriptCameraDelete()
{
	NotifyHandler( SScriptCameraWindowData::SCA_CAMERA_DELETE );
}


void CScriptCameraWindow::OnLvnItemchangedListScriptCameras( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	(*pResult) = 0;

	if ( (pNMLV->uChanged & LVIF_STATE) && 
			 (pNMLV->uNewState & LVIS_SELECTED) &&
			 !(pNMLV->uOldState & LVIS_SELECTED) )
	{
		dialogData.nCurrentCamera = pNMLV->iItem;
		btnDel.EnableWindow( dialogData.nCurrentCamera != -1 );
		btnSave.EnableWindow( dialogData.nCurrentCamera != -1 );

		NotifyHandler( SScriptCameraWindowData::SCA_CAMERA_CHANGE );
	}
}


void CScriptCameraWindow::OnNMDblclkListScriptCameras( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	(*pResult) = 0;

	if ( pNMLV->iItem >= 0 )
	{
		NotifyHandler( SScriptCameraWindowData::SCA_CAMERA_JUMP );
	}
}


void CScriptCameraWindow::OnBnClickedScriptCameraRun()
{
	NotifyHandler( SScriptCameraWindowData::SCA_CAMERA_RUN );
}


void CScriptCameraWindow::SetDialogData( const SScriptCameraWindowData *pData )
{
	if ( !pData )
		return;

	bIsDataSetting = true;
  //
	dialogData = *pData;

	lcCameras.DeleteAllItems();
	int i = 0;
	for ( std::vector<NCamera::CCameraPlacement>::const_iterator it = dialogData.scriptCameras.begin(); it < dialogData.scriptCameras.end(); ++it, ++i )
	{
		int nItem = lcCameras.InsertItem( i, "" );
		lcCameras.SetItemText( nItem, 0, fmt::format("{}", it->szName).c_str() );
		lcCameras.SetItemText( nItem, 1, fmt::format("{:.0f} : {:.0f} : {:.0f}", it->vPosition.x, it->vPosition.y, it->vPosition.z).c_str() );
		lcCameras.SetItemText( nItem, 2, fmt::format("{:.0f}", it->fFOV).c_str() );
		lcCameras.SetItemText( nItem, 3, fmt::format("{:.0f}", it->fYaw).c_str() );
		lcCameras.SetItemText( nItem, 4, fmt::format("{:.0f}", it->fPitch).c_str() );
		lcCameras.SetItemData( nItem, i );
	}
	lcCameras.SetItemState( dialogData.nCurrentCamera, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );

	btnDel.EnableWindow( dialogData.nCurrentCamera != -1 );
	btnSave.EnableWindow( dialogData.nCurrentCamera != -1 );
	btnRun.EnableWindow( !dialogData.scriptCameras.empty() );

	SetLastAction( SScriptCameraWindowData::SCA_NO_ACTIONS );
	//
	bIsDataSetting = false;
}


void CScriptCameraWindow::GetDialogData( SScriptCameraWindowData *pData )
{
	if ( !pData )
		return;

	for ( int i = 0; i < lcCameras.GetItemCount(); ++i )
	{
		if ( lcCameras.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
		{
			pData->nCurrentCamera = i;
			break;
		}
	}
	pData->eLastAction = dialogData.eLastAction;
}


void CScriptCameraWindow::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	switch ( nChar )
	{
		case VK_RETURN:
		{
			OnBnClickedScriptCameraSave();
			break;
		}
		//
		case VK_INSERT:
		{
			OnBnClickedScriptCameraAdd();
			break;
		}
		//
		case VK_DELETE:
		{
			OnBnClickedScriptCameraDelete();
			break;
		}
	}
}


void CScriptCameraWindow::OnChangeYaw()
{
	if ( !bIsDataSetting )
	{
		SetYawTimer();
	}
}


void CScriptCameraWindow::OnChangePitch()
{
	if ( !bIsDataSetting )
	{
		SetPitchTimer();
	}
}


void CScriptCameraWindow::OnChangeFOV()
{
	if ( !bIsDataSetting )
	{
		SetFOVTimer();
	}
}


void CScriptCameraWindow::OnTimer( UINT_PTR nIDEvent ) 
{
  if ( nIDEvent == GetYawTimerID() )
		OnYawTimer();
	else if ( nIDEvent == GetPitchTimerID() )
		OnPitchTimer();
	else if ( nIDEvent == GetFOVTimerID() )
		OnFOVTimer();

	CResizeDialog::OnTimer( nIDEvent );
}


void CScriptCameraWindow::SetYawTimer()
{
  KillYawTimer();
  nYawTimerID = SetTimer( GetYawTimerID(), GetYawTimerInterval(), 0 );
}


void CScriptCameraWindow::SetPitchTimer()
{
  KillPitchTimer();
  nPitchTimerID = SetTimer( GetPitchTimerID(), GetPitchTimerInterval(), 0 );
}


void CScriptCameraWindow::SetFOVTimer()
{
  KillFOVTimer();
  nFOVTimerID = SetTimer( GetFOVTimerID(), GetFOVTimerInterval(), 0 );
}


void CScriptCameraWindow::KillYawTimer()
{
  if ( nYawTimerID != 0 )
	{
		KillTimer( nYawTimerID );
	}
  nYawTimerID = 0;
}


void CScriptCameraWindow::KillPitchTimer()
{
  if ( nPitchTimerID != 0 )
	{
		KillTimer( nPitchTimerID );
	}
  nPitchTimerID = 0;
}


void CScriptCameraWindow::KillFOVTimer()
{
  if ( nFOVTimerID != 0 )
	{
		KillTimer( nFOVTimerID );
	}
  nFOVTimerID = 0;
}


void CScriptCameraWindow::OnYawTimer()
{
	KillYawTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE, ID_SCRIPT_CAMERA_GET_YAW, 0 );
}


void CScriptCameraWindow::OnPitchTimer()
{
	KillPitchTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE, ID_SCRIPT_CAMERA_GET_PITCH, 0 );
}


void CScriptCameraWindow::OnFOVTimer()
{
	KillFOVTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE, ID_SCRIPT_CAMERA_GET_FOV, 0 );
}




