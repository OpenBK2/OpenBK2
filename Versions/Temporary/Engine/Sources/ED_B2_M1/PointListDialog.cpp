#include "stdafx.h"
#include <fmt/format.h>

#include "Misc/2Darray.h"
#include "CommandHandlerDefines.h"
#include "SeasonMnemonics.h"
#include "PointListDialog.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

#include <zconf.h>

BEGIN_MESSAGE_MAP(CPointListDialog, CResizeDialog)
	ON_WM_DESTROY()
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_SETTING_SELECT_COMBO, OnCbnSelchangeSettingSelectCombo)
	ON_BN_CLICKED(IDC_CHECK_PROPMASK, OnBnClickedCheckPropmask)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_POINTS_LIST, OnLvnItemchangedPointsList)
	ON_BN_CLICKED(IDC_CHECK_PASSABILITY, OnBnClickedCheckPassability)
END_MESSAGE_MAP()


CPointListDialog::CPointListDialog( unsigned _nInstanceID, const CString &rszLabel )
	:	CResizeDialog( CPointListDialog::IDD ),
	szLabel( rszLabel ), 
	nInstanceID( _nInstanceID ),
	nSelectedIndex( -1 ),
	bIsDataSetting( false )
{
	SetControlStyle( IDC_POINTS_LIST, ANCHORE_LEFT_TOP | ANCHORE_HOR_CENTER | RESIZE_HOR_VER );
	SetControlStyle( IDC_SETTING_SELECT_COMBO, ANCHORE_LEFT | RESIZE_HOR );
	SetControlStyle( IDC_CHECK_PROPMASK, ANCHORE_LEFT );
	SetControlStyle( IDC_CHECK_PASSABILITY, ANCHORE_LEFT );

	// Was a push onto the static list and a Set of CHID_POINTS_LIST_DIALOG to
	// this; NPointListView keeps both now.
	NPointListView::Register( this );
}


CPointListDialog::~CPointListDialog()
{
	NPointListView::Unregister( this );
}


BOOL CPointListDialog::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	CRect rect;
	pointsList.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	pointsList.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	pointsList.EnableToolTips( TRUE );
	pointsList.GetClientRect( &rect );
	pointsList.InsertColumn( 0, "Points", LVCFMT_CENTER, rect.Width() );
	
	for ( int i = 0; i < typeSeasonMnemonics.Size(); ++i )
	{
		int nIdx = seasonCombo.AddString( typeSeasonMnemonics.GetMnemonic(i).c_str() ); 
		seasonCombo.SetItemData( nIdx, i );
	}
	
	return TRUE;
}


void CPointListDialog::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CPointListDialog::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_POINTS_LIST, pointsList );
	DDX_Control( pDX, IDC_SETTING_SELECT_COMBO, seasonCombo );
	DDX_Control( pDX, IDC_CHECK_PASSABILITY, chkPass );
	DDX_Control( pDX, IDC_CHECK_PROPMASK, chkPropMask );
}


void CPointListDialog::OnCbnSelchangeSettingSelectCombo()
{
	NotifyHandler();
}


void CPointListDialog::OnBnClickedCheckPropmask()
{
	NotifyHandler();
}


void CPointListDialog::OnBnClickedCheckPassability()
{
	NotifyHandler();
}


void CPointListDialog::OnLvnItemchangedPointsList( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	(*pResult) = 0;

	if (	(pNMLV->uChanged & LVIF_STATE) && 
				(pNMLV->uNewState & LVIS_SELECTED) &&
				!(pNMLV->uOldState & LVIS_SELECTED) )
	{
		nSelectedIndex = pNMLV->iItem;
		NotifyHandler();
	}
}


void CPointListDialog::NotifyHandler()
{
	if ( bIsDataSetting )
		return;

	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_POINTS_LIST_DLG_LISTENER, 
																												ID_POINTS_LIST_DLG_CHANGE_STATE, 
																												nInstanceID );
}


void CPointListDialog::GetDialogData( SPointListDialogData *pData )
{
	NI_ASSERT( !bIsDataSetting, "CPointListDialog::GetDialogData :: Trying to get dialog data during controls setup" );

	pData->nInstanceID = nInstanceID;
	pData->nNumPoints = pointsList.GetItemCount();  
	//pData->nSelectedPoint = -1;

	//for ( int i = 0; i < pointsList.GetItemCount(); ++i )
	//{
	//	if ( pointsList.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
	//	{
	//		pData->nSelectedPoint = i;
	//		break;
	//	}
	//}

	pData->nSelectedPoint = nSelectedIndex;

	pData->bChkPassability = ( chkPass.GetCheck() == BST_CHECKED );
	pData->bChkPropmask = ( chkPropMask.GetCheck() == BST_CHECKED );

	pData->eSeason = static_cast<NDb::ESeason>(-1);
	int nSelIdx = seasonCombo.GetCurSel();
	if ( nSelIdx != -1 )
		pData->eSeason = static_cast<NDb::ESeason>(seasonCombo.GetItemData( nSelIdx ));
}


void CPointListDialog::SetDialogData( const SPointListDialogData *pData )
{
	bIsDataSetting = true;

	if ( nInstanceID != pData->nInstanceID )
	{
		NI_ASSERT( 0, "nInstanceID != pData->nInstanceID" );
		return;
	}

	pointsList.DeleteAllItems();
	if ( nInstanceID == 4 ) // damage levels
	{
		const int nIdx = pointsList.InsertItem( 0, "Full health" );
		pointsList.SetItemData( nIdx, 0 );
		for ( int i = 0; i < pData->nNumPoints; ++i )
		{
			int nIdx = pointsList.InsertItem( i+1, fmt::format("{} {}", (const char*)szLabel, i).c_str() );
			pointsList.SetItemData( nIdx, i+1 );
		}
	}
	else	// all other dialogs
	{
		for ( int i = 0; i < pData->nNumPoints; ++i )
		{
			int nIdx = pointsList.InsertItem( i, fmt::format("{} {:3d}", (const char*)szLabel, i).c_str() );
			pointsList.SetItemData( nIdx, i );
		}
	}
	if ( pData->nNumPoints > 0 )
		pointsList.SetItemState( pData->nSelectedPoint, LVIS_SELECTED, LVIS_SELECTED );	//
	chkPass.SetCheck( pData->bChkPassability ? BST_CHECKED : BST_UNCHECKED );
	chkPropMask.SetCheck( pData->bChkPropmask ? BST_CHECKED : BST_UNCHECKED );
	//
	int nCount = seasonCombo.GetCount();
	for ( int i = 0; i < nCount; ++i )
	{
		NDb::ESeason e = static_cast<NDb::ESeason>( seasonCombo.GetItemData(i) );
		if ( e == pData->eSeason )
		{
			seasonCombo.SelectString( 0, typeSeasonMnemonics.GetMnemonic(e).c_str() );
			break;
		}
	}
	// The other lists' seasons follow this one. The loop this replaces also set
	// chkPass and chkPropMask once per other list -- but this list's, not the
	// other's, to the values already set above, so it did nothing and is gone.
	// The other lists' check boxes have never followed.
	NPointListView::FollowSeason( this, pData->eSeason );

	bIsDataSetting = false ;
}


// What the loop above did to each of the other lists, by name as it did it.
void CPointListDialog::FollowSeason( NDb::ESeason eSeason )
{
	seasonCombo.SelectString( 0, typeSeasonMnemonics.GetMnemonic( eSeason ).c_str() );
}


// HandleCommand and UpdateCommand were here, answering for every list at
// once; they are NPointListView's dispatch now, in PointListViewMfc.cpp.



