#include "stdafx.h"
#include "ED_B2_M1Dll.h"
#include "MapInfoAIMarkersSelectDlg.h"

#include <cstdint>

//
//
//	CMapInfoAIMarkersSelectDlg
//
//

BEGIN_MESSAGE_MAP( CMapInfoAIMarkersSelectDlg, CDialog )
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT, OnBnClickedButtonDefault)
END_MESSAGE_MAP()

CMapInfoAIMarkersSelectDlg::CMapInfoAIMarkersSelectDlg( 
		CMapInfoEditorSettings::SAIMarkerSettings *_pMarkerSettings, int _nPlayersNumber ) :
	CDialog( IDD_MAPINFO_AI_MARKERS, ::AfxGetMainWnd() ),
	pMarkerSettings( _pMarkerSettings ),
	nPlayersNumber( _nPlayersNumber )
{
}

CMapInfoAIMarkersSelectDlg::~CMapInfoAIMarkersSelectDlg()
{
}

void CMapInfoAIMarkersSelectDlg::DoDataExchange( CDataExchange *pDX )
{
	CDialog::DoDataExchange( pDX );
	//
	DDX_Control( pDX, IDC_COMBO_PLAYER, cbPlayer );
	DDX_Control( pDX, IDC_CHECK_SELECTION, btnChkSelection );
	DDX_Control( pDX, IDC_LIST_UNIT_TYPE, lcUnitTypes );
}

INT_PTR CMapInfoAIMarkersSelectDlg::DoModal()
{
	AfxSetResourceHandle( theEDB2M1Instance );
	INT_PTR res = CDialog::DoModal();
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	return res;
}

BOOL CMapInfoAIMarkersSelectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	lcUnitTypes.ModifyStyle( 0,	LVS_ALIGNLEFT|WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_NOCOLUMNHEADER, 0 ); 
	lcUnitTypes.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES ); 
	CRect r;
	lcUnitTypes.GetClientRect( &r );
	lcUnitTypes.InsertColumn( 0, 0, LVCFMT_LEFT, r.Width() );

	for ( int i = 0; i < nPlayersNumber; ++i )
	{
		int nItem = cbPlayer.InsertString( 0, StrFmt( "%d", i+1 ) );
		cbPlayer.SetItemData( nItem, i );	
	}
	if ( nPlayersNumber > 0 )
		cbPlayer.SetCurSel(0);

	btnChkSelection.SetCheck( 0 );

	SetDD();

	return TRUE;
}

void CMapInfoAIMarkersSelectDlg::OnOK()
{
	GetDD();	// сохранить настройки фильтра
	CDialog::OnOK();
}

void CMapInfoAIMarkersSelectDlg::OnCancel()
{
	CDialog::OnCancel();
}

bool CMapInfoAIMarkersSelectDlg::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	return false;
}

bool CMapInfoAIMarkersSelectDlg::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapInfoAIMarkersSelectDlg::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapInfoAIMarkersSelectDlg::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}

void CMapInfoAIMarkersSelectDlg::OnDestroy()
{
	CDialog::OnDestroy();
}

void CMapInfoAIMarkersSelectDlg::SetDD()
{
	if ( !pMarkerSettings )
		return;
	// фильтр по типам объектов
	lcUnitTypes.DeleteAllItems();
	for ( int i = 0; i < pMarkerSettings->objTypeMarkers.size(); ++i )
	{
		int nItem = lcUnitTypes.InsertItem( i, pMarkerSettings->objTypeMarkers[i].szObjTypeName.c_str() );
		lcUnitTypes.SetCheck( nItem, pMarkerSettings->objTypeMarkers[i].bShowMarker ? 1 : 0 );
	}
	//
	btnChkSelection.SetCheck( pMarkerSettings->bForSelectionOnly ? 1 : 0 );
	//
	cbPlayer.SelectString( 0, StrFmt( "%d", pMarkerSettings->nPlayer ) );
	//
}

void CMapInfoAIMarkersSelectDlg::GetDD()
{
	if ( !pMarkerSettings )
		return;
	// фильтр по типам объектов
	for ( int i = 0; i < lcUnitTypes.GetItemCount(); ++i )
	{
		pMarkerSettings->objTypeMarkers[i].bShowMarker = lcUnitTypes.GetCheck( i );
	}
	//
	pMarkerSettings->bForSelectionOnly = btnChkSelection.GetCheck();
	//
	CString s;
	cbPlayer.GetWindowText( s );
	pMarkerSettings->nPlayer = atoi(s);
	//
}

void CMapInfoAIMarkersSelectDlg::OnBnClickedButtonDefault()
{
	if ( !pMarkerSettings )
		return;
	//
	pMarkerSettings->SetDefault();
	SetDD();
}


