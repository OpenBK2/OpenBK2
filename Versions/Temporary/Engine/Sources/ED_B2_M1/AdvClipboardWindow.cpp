#include "stdafx.h"
/**
#include "ResourceDefines.h"
#include "CommandHandlerDefines.h"
#include "StringResources.h"
#include "AdvClipboardWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//
//		ADVANCED CLIPBOARD WINDOW
//
//

BEGIN_MESSAGE_MAP(CAdvClipboardWindow, CResizeDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_COPY, OnBnClickedButtonCopy)
	ON_BN_CLICKED(IDC_BUTTON_PASTE, OnBnClickedButtonPaste)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_CLIP, OnBnClickedButtonSaveClip)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CLIPS, OnLvnItemchangedListClips)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_CLIP, OnBnClickedButtonLoadClip)
END_MESSAGE_MAP()


CAdvClipboardWindow::CAdvClipboardWindow( CWnd *pParentWindow )
	:	CResizeDialog( CAdvClipboardWindow::IDD, pParentWindow ),
	eLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS )
{
	SetControlStyle( IDC_BUTTON_SAVE_CLIP, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_BUTTON_LOAD_CLIP, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_LIST_CLIPS, ANCHORE_LEFT | ANCHORE_RIGHT | ANCHORE_BOTTOM | RESIZE_HOR_VER );

	Singleton<ICommandHandlerContainer>()->Set( CHID_ADV_CLIPBOARD_WINDOW, this );
}


CAdvClipboardWindow::~CAdvClipboardWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_ADV_CLIPBOARD_WINDOW );
}


BOOL CAdvClipboardWindow::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	//
	lcPasteSettings.ModifyStyle( 0, LVS_ALIGNLEFT, 0 ); 
	lcPasteSettings.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES ); 
	//
	CRect rect;
	lcPasteSettings.GetClientRect( &rect );
	lcPasteSettings.InsertColumn( 0, RCSTR("Paste"), LVCFMT_LEFT, rect.Width() * 1.0f );
	//
	int nItemIdx = 0;
	int nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("terrain tiles + spots"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("terrain heights + cliffs"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("mech units + squads"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("buildings"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("objects + fences + entrenchments"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("bridges + roads"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	nItem = lcPasteSettings.InsertItem( nItemIdx, RCSTR("rivers + lakes + islands"), -1 );
	lcPasteSettings.SetItemData( nItem, nItemIdx );
	lcPasteSettings.SetCheck( nItem, TRUE );
	++nItemIdx;
	//
	return TRUE;
}


void CAdvClipboardWindow::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CAdvClipboardWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_LIST_CLIPS, lcPasteSettings );
}


void CAdvClipboardWindow::NotifyHandler()
{
	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_ADV_CLIPBOARD_STATE, 
																												ID_ADV_CLIPBOARD_WINDOW_CHANGE_STATE, 
																												0 
	);
}


void CAdvClipboardWindow::GetDialogData( SAdvClipboardWindowData *pData )
{
	if ( !pData )
		return;

	pData->Clear();
	pData->eLastAction = eLastAction;
	//
	int nItemIdx = 0;
	pData->pasteSettings.bPasteTerrainTilesSpots = lcPasteSettings.GetCheck( nItemIdx++ );
	pData->pasteSettings.bPasteHeightsCrags = lcPasteSettings.GetCheck( nItemIdx++ );
	pData->pasteSettings.bPasteUnitsSquads = lcPasteSettings.GetCheck( nItemIdx++ );
	pData->pasteSettings.bPasteBuildings = lcPasteSettings.GetCheck( nItemIdx++ );
	pData->pasteSettings.bPasteObjectsFencesEntrenchmentes = lcPasteSettings.GetCheck( nItemIdx++ );
	pData->pasteSettings.bPasteBridgeRoads = lcPasteSettings.GetCheck( nItemIdx++ );
	pData->pasteSettings.bPasteRiversLakesIslands = lcPasteSettings.GetCheck( nItemIdx++ );
}


void CAdvClipboardWindow::SetDialogData( const SAdvClipboardWindowData *pData )
{
	SetLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS );
	//
	int nItemIdx = 0;
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteTerrainTilesSpots );
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteHeightsCrags );
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteUnitsSquads );
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteBuildings );
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteObjectsFencesEntrenchmentes );
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteBridgeRoads );
	lcPasteSettings.SetCheck( nItemIdx++, pData->pasteSettings.bPasteRiversLakesIslands );
}


bool CAdvClipboardWindow::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID ) 
	{
	case ID_WINDOW_GET_DIALOG_DATA:
		{
			SAdvClipboardWindowData *pData = reinterpret_cast<SAdvClipboardWindowData*>( dwData );
			GetDialogData( pData );
			return true;
		}
		break;
	case ID_WINDOW_SET_DIALOG_DATA:
		{
			SAdvClipboardWindowData *pData = reinterpret_cast<SAdvClipboardWindowData*>( dwData );
			SetDialogData( pData );
			return true;
		}
		break;
	}

	return false;
}


bool CAdvClipboardWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAdvClipboardWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAdvClipboardWindow::UpdateCommand(), pbCheck == 0" );
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


void CAdvClipboardWindow::OnBnClickedButtonCopy()
{
	SetLastAction( SAdvClipboardWindowData::LA_COPY );
	NotifyHandler();
	SetLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS );
}


void CAdvClipboardWindow::OnBnClickedButtonPaste()
{
	SetLastAction( SAdvClipboardWindowData::LA_PASTE );
	NotifyHandler();
	SetLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS );
}


void CAdvClipboardWindow::OnBnClickedButtonSaveClip()
{
	SetLastAction( SAdvClipboardWindowData::LA_SAVE_CLIP );
	NotifyHandler();
	SetLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS );
}


void CAdvClipboardWindow::OnLvnItemchangedListClips(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	SetLastAction( SAdvClipboardWindowData::LA_SEL_CLIP_CHANGE );
	NotifyHandler();
	SetLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS );
}


void CAdvClipboardWindow::OnBnClickedButtonLoadClip()
{
	SetLastAction( SAdvClipboardWindowData::LA_LOAD_CLIP );
	NotifyHandler();
	SetLastAction( SAdvClipboardWindowData::LA_NO_ACTIONS );
}
/**/


