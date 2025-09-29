#include "stdafx.h"

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "commandhandlerdefines.h"
#include "StringResources.h"
#include "ScriptAreaWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//	SCRIPT AREA WINDOW
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CScriptAreaWindow, CResizeDialog )
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_AREA_NAMES, OnItemchangedAreaList)
	ON_BN_CLICKED( IDC_BUTTON_DEL, OnButtonDel )
	ON_BN_CLICKED( IDC_RADIO_CIRCLE, OnRadioCircle )
	ON_BN_CLICKED( IDC_RADIO_RECT, OnRadioRectangle )
	ON_BN_CLICKED( IDC_BUTTON_SELECT, OnButtonSelect )
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CScriptAreaWindow::CScriptAreaWindow( CWnd* pParentWindow )
	: CResizeDialog( CScriptAreaWindow::IDD, pParentWindow ),
	bIsDataBeginSet( false )
{
	SetControlStyle( IDC_LIST_AREA_NAMES, ANCHORE_LEFT_TOP | ANCHORE_HOR_CENTER | RESIZE_HOR_VER );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SCRIPT_AREA_WINDOW, this );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CScriptAreaWindow::~CScriptAreaWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SCRIPT_AREA_WINDOW );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_RADIO_CIRCLE, rbnCircle );
	DDX_Control( pDX, IDC_RADIO_RECT, rbnRectangle );
	DDX_Control( pDX, IDC_LIST_AREA_NAMES, lcAreas );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CScriptAreaWindow::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	//
	lcAreas.ModifyStyle( 0, LVS_ALIGNLEFT, 0 ); 
	lcAreas.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT ); 
	//
	CRect r;
	lcAreas.GetClientRect( &r );
	lcAreas.InsertColumn( 0, RCSTR("Area name"), LVCFMT_LEFT, r.Width() * 0.8 );
	lcAreas.InsertColumn( 1, RCSTR("Type"), LVCFMT_LEFT, r.Width() * 0.19 );
	//
	UpdateControls();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::UpdateControls()
{
	bool bEnable = false;
	for ( int i = 0; i < lcAreas.GetItemCount(); ++i )
	{
		if ( lcAreas.GetItemState( i, LVIS_SELECTED ) & LVIS_SELECTED ) 
		{
			bEnable = true;
			break;
		}
	}
	if ( CWnd *pwndButton = GetDlgItem( IDC_BUTTON_DEL ) )
	{
		pwndButton->EnableWindow( bEnable );
	}
	if ( CWnd *pwndButton = GetDlgItem( IDC_BUTTON_SELECT ) )
	{
		pwndButton->EnableWindow( bEnable );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::OnItemchangedAreaList( NMHDR* pNMHDR, LRESULT* pResult )
{
	UpdateControls();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CScriptAreaWindow::HandleCommand( UINT nCommandID, DWORD dwData )
{
	SScriptAreaWindowData *pData = reinterpret_cast<SScriptAreaWindowData*>( dwData );
	//
	NI_ASSERT( pData, "CScriptAreaWindow::HandleCommand(): dwData == 0" );
	if ( !pData )
		return false;
	//
	switch ( nCommandID )
	{
	case ID_WINDOW_GET_DIALOG_DATA:
		{
			GetDialogData( pData );
			return true;
		}
		break;
	case ID_WINDOW_SET_DIALOG_DATA:
		{
			SetDialogData( pData );
			return true;
		}
		break;
	}
	//
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CScriptAreaWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CScriptAreaWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CScriptAreaWindow::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
	case ID_WINDOW_GET_DIALOG_DATA:
	case ID_WINDOW_SET_DIALOG_DATA:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}
	//
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::OnDestroy()
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::GetDialogData( SScriptAreaWindowData *pData )
{
	*pData = dialogData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::SetDialogData( const SScriptAreaWindowData *pData )
{
	bIsDataBeginSet = true;
	dialogData = *pData;
	//
	if ( pData->eChangeMask & SScriptAreaWindowData::CHANGE_AREA_TYPE )
	{
		rbnCircle.SetCheck( BST_UNCHECKED );
		rbnRectangle.SetCheck( BST_UNCHECKED );
		//
		if ( pData->eAreaType == NDb::EAT_CIRCLE )
		{
			rbnCircle.SetCheck( BST_CHECKED );
			rbnRectangle.SetCheck( BST_UNCHECKED );
		}
		else if ( pData->eAreaType == NDb::EAT_RECTANGLE )
		{
			rbnCircle.SetCheck( BST_UNCHECKED );
			rbnRectangle.SetCheck( BST_CHECKED );
		}
	}
	//
	if ( pData->eChangeMask & SScriptAreaWindowData::CHANGE_AREAS )
	{
		lcAreas.DeleteAllItems();
		for ( int i = 0; i < pData->scriptAreaList.size(); ++i )
		{
			const SScriptAreaWindowData::SScriptArea *pA = &pData->scriptAreaList[i];
			//
			int nItem = lcAreas.InsertItem( i, pA->szName.c_str(), -1 );
			lcAreas.SetItemData( nItem, pA->nScriptAreaID );
			//
			switch ( pA->eType )
			{
			case NDb::EAT_CIRCLE:
				lcAreas.SetItemText( nItem, 1, RCSTR("circle") );
				break;
			case NDb::EAT_RECTANGLE:
				lcAreas.SetItemText( nItem, 1, RCSTR("rect") );
				break;
			}
		}
	}
	//
	if ( pData->eChangeMask & SScriptAreaWindowData::CHANGE_SELECTION )
	{
		for ( int i = 0; i < lcAreas.GetItemCount(); ++i )
		{
			lcAreas.SetItemState( i, ~LVIS_SELECTED, LVIS_SELECTED );
			//
			UINT nScriptAreaID = lcAreas.GetItemData( i );
			for ( int a = 0; a < pData->selectedScriptAreaIDList.size(); ++a )
			{
				if ( pData->selectedScriptAreaIDList[a] == nScriptAreaID )
				{
					lcAreas.SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
				}
			}
		}
	}
	//
	bIsDataBeginSet = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::OnButtonDel()
{
	dialogData.selectedScriptAreaIDList.clear();
	for ( int i = 0; i < lcAreas.GetItemCount(); ++i )
	{
		if ( lcAreas.GetItemState( i, LVIS_SELECTED ) & LVIS_SELECTED ) 
		{
			dialogData.selectedScriptAreaIDList.push_back( lcAreas.GetItemData( i ) );
		}
	}
	//
	dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_DEL_SEL;
	NotifyHandler();
	UpdateControls();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::OnRadioCircle()
{
	dialogData.eAreaType = NDb::EAT_CIRCLE;
	dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_AREA_TYPE;
	NotifyHandler();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::OnRadioRectangle()
{
	dialogData.eAreaType = NDb::EAT_RECTANGLE;
	dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_AREA_TYPE;
	NotifyHandler();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::NotifyHandler()
{
	if ( bIsDataBeginSet )
		return;	// чтобы сообщения не шли в момент установки свойств контролов
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_AREA_STATE, ID_SCRIPT_AREA_WINDOW_UI_EVENT, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CScriptAreaWindow::OnButtonSelect()
{
	dialogData.selectedScriptAreaIDList.clear();
	for ( int i = 0; i < lcAreas.GetItemCount(); ++i )
	{
		if ( lcAreas.GetItemState( i, LVIS_SELECTED ) & LVIS_SELECTED ) 
		{
			dialogData.selectedScriptAreaIDList.push_back( lcAreas.GetItemData(i) );
		}
	}
	//
	dialogData.eChangeMask = SScriptAreaWindowData::CHANGE_SELECTION;
	NotifyHandler();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
