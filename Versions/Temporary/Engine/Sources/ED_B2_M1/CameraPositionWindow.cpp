#include "stdafx.h"
#include <fmt/format.h>

#include "CommandHandlerDefines.h"
#include "CameraPositionWindow.h"

#include <cstdint>

//
//		
//		CAMERA POSITION WINDOW
//
//

BEGIN_MESSAGE_MAP(CCameraPositionWindow, CResizeDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_OW_PLAYER_COMBO_BOX, OnCbnSelchangeOwPlayerComboBox)
	ON_BN_CLICKED(IDC_OW_SET_BUTTON, OnBnClickedButtonSave)
	ON_BN_CLICKED(IDC_OW_POSITION_ONLY_RADIO, OnBnClickedParamType)
	ON_BN_CLICKED(IDC_OW_ALL_PARAMS_RADIO, OnBnClickedParamType)
END_MESSAGE_MAP()


CCameraPositionWindow::CCameraPositionWindow( CWnd *pParentWindow )
	:	CResizeDialog( CCameraPositionWindow::IDD, pParentWindow )
{
	SetControlStyle( IDC_OW_PLAYER_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_OW_PLAYER_COMBO_BOX, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_OW_SET_BUTTON, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_OW_POSITION_ONLY_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_OW_ALL_PARAMS_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_CAMERA_POSITION_WINDOW, this );
}


CCameraPositionWindow::~CCameraPositionWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_CAMERA_POSITION_WINDOW );
}


BOOL CCameraPositionWindow::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	return TRUE;
}


void CCameraPositionWindow::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CCameraPositionWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_OW_PLAYER_COMBO_BOX, wndPalyerComboBox );
}


void CCameraPositionWindow::GetDialogData( SCameraPositionWindowData *pData )
{
	if ( !bIsDataSetting )
	{
		pData->Clear();
		//
		pData->nPlayerCount = wndPalyerComboBox.GetCount();
		const int nStringNumber = wndPalyerComboBox.GetCurSel();
		if ( nStringNumber >= 0 )
		{
			pData->nPlayerIndex = wndPalyerComboBox.GetItemData( nStringNumber );
		}
		else
		{
			pData->nPlayerIndex = -1;
		}
		pData->bAllParams = ( GetCheckedRadioButton( IDC_OW_POSITION_ONLY_RADIO, IDC_OW_ALL_PARAMS_RADIO ) == IDC_OW_ALL_PARAMS_RADIO );
	}
}


void CCameraPositionWindow::SetDialogData( const SCameraPositionWindowData *pData )
{
	bIsDataSetting = true;
	wndPalyerComboBox.ResetContent();
	for ( int nPlayerIndex = 0; nPlayerIndex < pData->nPlayerCount; ++nPlayerIndex )
	{
		const int nStringNumber = wndPalyerComboBox.AddString( std::to_string(  nPlayerIndex ).c_str() );
		wndPalyerComboBox.SetItemData( nStringNumber, nPlayerIndex );
	}
	wndPalyerComboBox.SelectString( 0, fmt::format("{}", pData->nPlayerIndex).c_str() );
	CheckRadioButton( IDC_OW_POSITION_ONLY_RADIO, IDC_OW_ALL_PARAMS_RADIO, pData->bAllParams ? IDC_OW_ALL_PARAMS_RADIO : IDC_OW_POSITION_ONLY_RADIO );
	bIsDataSetting = false;
}


bool CCameraPositionWindow::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	SCameraPositionWindowData *pData = reinterpret_cast<SCameraPositionWindowData*>(dwData);
	switch( nCommandID ) 
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
	}

	return false;
}


bool CCameraPositionWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CCameraPositionWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CCameraPositionWindow::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
	case ID_WINDOW_SET_DIALOG_DATA:
	case ID_WINDOW_GET_DIALOG_DATA:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}
}


void CCameraPositionWindow::OnCbnSelchangeOwPlayerComboBox()
{
	if ( bIsDataSetting )
		return;

	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_STATE, 
																												ID_CPE_ON_PLAYER_CHANGED, 0 );
}


void CCameraPositionWindow::OnBnClickedButtonSave()
{
	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand(	CHID_CAMERA_POSITION_STATE, 
																												ID_CPW_ON_SAVE, 0 );
}


void CCameraPositionWindow::OnBnClickedParamType()
{
	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand(	CHID_CAMERA_POSITION_STATE, ID_CPW_PARAM_TYPE_CHANGED, 0 );
}



