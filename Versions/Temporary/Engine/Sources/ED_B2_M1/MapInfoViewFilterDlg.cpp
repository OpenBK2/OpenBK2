#include "stdafx.h"

#include "ED_B2_M1Dll.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapInfoViewFilterDlg.h"
#include "MapInfoViewFilter.h"
#include "MapEditorLib/ShellFont.h"

#include <cstdint>

//
//
//	CMapInfoViewFilterDlg
//
//

BEGIN_MESSAGE_MAP( CMapInfoViewFilterDlg, CDialog )
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT, OnBnClickedButtonDefault)
	ON_CBN_SELCHANGE(IDC_COMBO_GRID_SIZE, OnCbnSelchangeComboGridSize)
	ON_BN_CLICKED(IDC_CHECK_GRID, OnBnClickedCheckGrid)
	ON_BN_CLICKED(IDC_CHECK_BB, OnBnClickedCheckBB)
	ON_BN_CLICKED(IDC_CHECK_WF, OnBnClickedCheckWF)
	ON_BN_CLICKED(IDC_CHECK_TERRAF, OnBnClickedCheckTerraF)
	ON_BN_CLICKED(IDC_CHECK_SHADOWSF, OnBnClickedCheckShadowsF)
	ON_BN_CLICKED(IDC_CHECK_WARFOGF, OnBnClickedCheckWarFogF)
	ON_BN_CLICKED(IDC_CHECK_STATSF, OnBnClickedCheckStatsF)
	ON_BN_CLICKED(IDC_CHECK_MIPMAPF, OnBnClickedCheckMipmapF)
	ON_BN_CLICKED(IDC_CHECK_OVERDRAWF, OnBnClickedCheckOverdrawF)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OBJ_TYPES, OnLvnItemchangedListObjTypes)
END_MESSAGE_MAP()

// The parent was always ::AfxGetMainWnd(); it now comes through the boundary,
// which passes the main window, so it is the same window.
CMapInfoViewFilterDlg::CMapInfoViewFilterDlg( CWnd *pParent, CMapInfoEditorSettings::SViewFilterData *_pViewFilter )
	: CDialog( IDD_DLG_MAPINFO_VIEW_FILTER, pParent ),
	bIsDataSetting( false ),
	pViewFilter( _pViewFilter )
{
	defViewFilter = (*pViewFilter);
}


void CMapInfoViewFilterDlg::DoDataExchange( CDataExchange *pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_LIST_OBJ_TYPES, objTypesList );
	DDX_Control( pDX, IDC_CHECK_WF, chkWireFrame );
	DDX_Control( pDX, IDC_CHECK_BB, chkBoundingBoxes );
	DDX_Control( pDX, IDC_CHECK_TERRAF, chkShowTerrain );
	DDX_Control( pDX, IDC_CHECK_SHADOWSF, chkShowShadows );
	DDX_Control( pDX, IDC_CHECK_WARFOGF, chkShowWarfog );
	DDX_Control( pDX, IDC_CHECK_STATSF, chkShowStats );
	DDX_Control( pDX, IDC_CHECK_GRID, chkGrid );
	DDX_Control( pDX, IDC_CHECK_MIPMAPF, chkMipmap );
	DDX_Control( pDX, IDC_CHECK_OVERDRAWF, chkOverdraw );
	DDX_Control( pDX, IDC_COMBO_GRID_SIZE, comboGridSize );
}

INT_PTR CMapInfoViewFilterDlg::DoModal()
{
	AfxSetResourceHandle( theEDB2M1Instance );
	INT_PTR res = CDialog::DoModal();
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	return res;
}

BOOL CMapInfoViewFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Not a CResizeDialog, so it does not get the shell font from there.
	NEditorFont::ApplyShellFont( this );
	//
	objTypesList.ModifyStyle( 0,	LVS_ALIGNLEFT|WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_NOCOLUMNHEADER, 0 ); 
	objTypesList.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES ); 
	CRect r;
	objTypesList.GetClientRect( &r );
	objTypesList.InsertColumn( 0, 0, LVCFMT_LEFT, 200 );
	//
	comboGridSize.InsertString( 0, RCSTR("Visual tile") );
	comboGridSize.InsertString( 1, RCSTR("AI tile") );
	comboGridSize.SelectString( 0, RCSTR("Visual tile") );
	//
	SetDialogData();
	//
	return TRUE;
}

void CMapInfoViewFilterDlg::OnOK()
{
	GetDialogData();
	CDialog::OnOK();
}

void CMapInfoViewFilterDlg::OnCancel()
{
	(*pViewFilter) = defViewFilter;
	Apply();
	CDialog::OnCancel();
}

bool CMapInfoViewFilterDlg::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	return false;
}

bool CMapInfoViewFilterDlg::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapInfoViewFilterDlg::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapInfoViewFilterDlg::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}

void CMapInfoViewFilterDlg::OnDestroy()
{
	CDialog::OnDestroy();
}

void CMapInfoViewFilterDlg::SetDialogData()
{
	if ( !pViewFilter )
		return;

	bIsDataSetting = true;
	// фильтр по типам объектов
	objTypesList.DeleteAllItems();
	for ( int i = 0; i < pViewFilter->objTypeFilter.size(); ++i )
	{
		int nItem = objTypesList.InsertItem( i, pViewFilter->objTypeFilter[i].szObjTypeName.c_str() );
		objTypesList.SetCheck( nItem, pViewFilter->objTypeFilter[i].bShow ? 1 : 0 );
	}
	//
	chkGrid.SetCheck( pViewFilter->bShowGrid ? 1 : 0 );
	chkBoundingBoxes.SetCheck( pViewFilter->bShowBBoxes  ? 1 : 0 );
	chkWireFrame.SetCheck( pViewFilter->bWireFrame ? 1 : 0 );
	chkShowTerrain.SetCheck( pViewFilter->bShowTerrain ? 1 : 0 );
	chkShowShadows.SetCheck( pViewFilter->bShowShadows ? 1 : 0 );
	chkShowWarfog.SetCheck( pViewFilter->bShowWarfog ? 1 : 0 );
	chkShowStats.SetCheck( pViewFilter->bShowStats ? 1 : 0 );
	chkMipmap.SetCheck( pViewFilter->bMipmap ? 1 : 0 );
	chkOverdraw.SetCheck( pViewFilter->bOverdraw ? 1 : 0 );
	comboGridSize.SelectString( 0, pViewFilter->szGridSize.c_str() );
	//
	bIsDataSetting = false;
}

void CMapInfoViewFilterDlg::GetDialogData()
{
	if ( bIsDataSetting )
		return;

	if ( !pViewFilter )
		return;

	pViewFilter->objTypeFilter.clear();
	for ( int i = 0; i < objTypesList.GetItemCount(); ++i )
	{
		CMapInfoEditorSettings::SViewFilterData::SObjTypeFilter tf;
		tf.szObjTypeName = (const char*)objTypesList.GetItemText( i, 0 );
		tf.bShow = objTypesList.GetCheck( i );
		pViewFilter->objTypeFilter.push_back( tf );
	}
	//
	pViewFilter->bShowGrid = chkGrid.GetCheck();
	pViewFilter->bShowBBoxes = chkBoundingBoxes.GetCheck();
	pViewFilter->bWireFrame = chkWireFrame.GetCheck();
	pViewFilter->bShowTerrain = chkShowTerrain.GetCheck();
	pViewFilter->bShowShadows = chkShowShadows.GetCheck();
	pViewFilter->bShowWarfog = chkShowWarfog.GetCheck();
	pViewFilter->bShowStats = chkShowStats.GetCheck();
	pViewFilter->bMipmap = chkMipmap.GetCheck();
	pViewFilter->bOverdraw = chkOverdraw.GetCheck();
	//
	CString szTmp;
	comboGridSize.GetWindowText( szTmp );
	pViewFilter->szGridSize = (const char*)szTmp;
	//
	Apply();
}

void CMapInfoViewFilterDlg::OnBnClickedButtonDefault()
{
	if ( !pViewFilter )
		return;
	//
	pViewFilter->SetDefault();
	SetDialogData();
	GetDialogData();
}

void CMapInfoViewFilterDlg::OnLvnItemchangedListObjTypes(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	GetDialogData();
}

// The command moved to NMapInfoViewFilter::Apply so the wx dialog sends it the
// same way.
void CMapInfoViewFilterDlg::Apply()
{
	NMapInfoViewFilter::Apply();
}


