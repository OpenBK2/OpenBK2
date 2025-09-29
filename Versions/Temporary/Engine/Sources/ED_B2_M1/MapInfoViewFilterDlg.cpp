#include "StdAfx.h"

#include "ED_B2_M1Dll.h"
#include "../MapEditorLib/ResourceDefines.h"
#include "MapInfoViewFilterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//	CMapInfoViewFilterDlg
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CMapInfoViewFilterDlg::CMapInfoViewFilterDlg( CMapInfoEditorSettings *_pMapEditorSettings )
	: CDialog( IDD_DLG_MAPINFO_VIEW_FILTER, ::AfxGetMainWnd() ),
	pMapEditorSettings( _pMapEditorSettings )
{
	defMapEditorSettings = (*pMapEditorSettings);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CMapInfoViewFilterDlg::DoModal()
{
	AfxSetResourceHandle( theEDB2M1Instance );
	INT_PTR res = CDialog::DoModal();
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	return res;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CMapInfoViewFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::OnOK()
{
	GetDialogData();
	CDialog::OnOK();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::OnCancel()
{
	(*pMapEditorSettings) = defMapEditorSettings;
	Apply();
	CDialog::OnCancel();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMapInfoViewFilterDlg::HandleCommand( UINT nCommandID, DWORD dwData )
{
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMapInfoViewFilterDlg::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapInfoViewFilterDlg::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapInfoViewFilterDlg::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::OnDestroy()
{
	CDialog::OnDestroy();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::SetDialogData()
{
	if ( !pMapEditorSettings )
		return;

	bIsDataSetting = true;
	// фильтр по типам объектов
	objTypesList.DeleteAllItems();
	for ( int i = 0; i < pMapEditorSettings->viewFilterData.objTypeFilter.size(); ++i )
	{
		int nItem = objTypesList.InsertItem( i, pMapEditorSettings->viewFilterData.objTypeFilter[i].szObjTypeName.c_str() );
		objTypesList.SetCheck( nItem, pMapEditorSettings->viewFilterData.objTypeFilter[i].bShow ? 1 : 0 );
	}
	//
	chkGrid.SetCheck( pMapEditorSettings->viewFilterData.bShowGrid ? 1 : 0 );
	chkBoundingBoxes.SetCheck( pMapEditorSettings->viewFilterData.bShowBBoxes  ? 1 : 0 );
	chkWireFrame.SetCheck( pMapEditorSettings->viewFilterData.bWireFrame ? 1 : 0 );
	chkShowTerrain.SetCheck( pMapEditorSettings->viewFilterData.bShowTerrain ? 1 : 0 );
	chkShowShadows.SetCheck( pMapEditorSettings->viewFilterData.bShowShadows ? 1 : 0 );
	chkShowWarfog.SetCheck( pMapEditorSettings->viewFilterData.bShowWarfog ? 1 : 0 );
	chkShowStats.SetCheck( pMapEditorSettings->viewFilterData.bShowStats ? 1 : 0 );
	chkMipmap.SetCheck( pMapEditorSettings->viewFilterData.bMipmap ? 1 : 0 );
	chkOverdraw.SetCheck( pMapEditorSettings->viewFilterData.bOverdraw ? 1 : 0 );
	comboGridSize.SelectString( 0, pMapEditorSettings->viewFilterData.szGridSize.c_str() );
	//
	bIsDataSetting = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::GetDialogData()
{
	if ( bIsDataSetting )
		return;

	if ( !pMapEditorSettings )
		return;

	pMapEditorSettings->viewFilterData.objTypeFilter.clear();
	for ( int i = 0; i < objTypesList.GetItemCount(); ++i )
	{
		CMapInfoEditorSettings::SViewFilterData::SObjTypeFilter tf;
		tf.szObjTypeName = (const char*)objTypesList.GetItemText( i, 0 );
		tf.bShow = objTypesList.GetCheck( i );
		pMapEditorSettings->viewFilterData.objTypeFilter.push_back( tf );
	}
	//
	pMapEditorSettings->viewFilterData.bShowGrid = chkGrid.GetCheck();
	pMapEditorSettings->viewFilterData.bShowBBoxes = chkBoundingBoxes.GetCheck();
	pMapEditorSettings->viewFilterData.bWireFrame = chkWireFrame.GetCheck();
	pMapEditorSettings->viewFilterData.bShowTerrain = chkShowTerrain.GetCheck();
	pMapEditorSettings->viewFilterData.bShowShadows = chkShowShadows.GetCheck();
	pMapEditorSettings->viewFilterData.bShowWarfog = chkShowWarfog.GetCheck();
	pMapEditorSettings->viewFilterData.bShowStats = chkShowStats.GetCheck();
	pMapEditorSettings->viewFilterData.bMipmap = chkMipmap.GetCheck();
	pMapEditorSettings->viewFilterData.bOverdraw = chkOverdraw.GetCheck();
	//
	CString szTmp;
	comboGridSize.GetWindowText( szTmp );
	pMapEditorSettings->viewFilterData.szGridSize = (const char*)szTmp;
	//
	Apply();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::OnBnClickedButtonDefault()
{
	if ( !pMapEditorSettings )
		return;
	//
	pMapEditorSettings->viewFilterData.SetDefault();
	SetDialogData();
	GetDialogData();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::OnLvnItemchangedListObjTypes(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	GetDialogData();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapInfoViewFilterDlg::Apply()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, 
																												ID_VIEW_APPLY_MI_FILTER, 
																												0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
