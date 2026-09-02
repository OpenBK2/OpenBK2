#include "stdafx.h"
#include <fmt/format.h>

#include "ScriptCameraRun.h"
#include "ScriptCameraRun.h"

//
//
//		RUN ScriptMovie dialog
//
//

CScriptCameraRunDlg::CScriptCameraRunDlg(	CWnd* _pParentWindow, SScriptCameraRunDlgData *_pDialogData )
	: CResizeDialog( CScriptCameraRunDlg::IDD, _pParentWindow ),
	pDialogData( _pDialogData ),
	bSpeed( false )
{
	NI_ASSERT( !pDialogData->scriptCameras.empty(), "There are no cameras in scene\n" );
}

void CScriptCameraRunDlg::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_SCRUN_COMBO_START, cbStartCam );
	DDX_Control( pDX, IDC_SCRUN_COMBO_FINISH, cbFinishCam );
	DDX_Control( pDX, IDC_SCRUN_COMBO_TYPE, cbType );
	DDX_Control( pDX, IDC_SCRUN_EDIT_TIME, edTime );
	DDX_Control( pDX, IDC_SCRUN_EDIT_LSPEED, edLSpeed );
	DDX_Control( pDX, IDC_SCRUN_EDIT_ASPEED, edASpeed );
	DDX_Control( pDX, IDC_SCRUN_EDIT_TARGET, edTarget );
	DDX_Control( pDX, IDC_SCRUN_EDIT_SPLINE1, edSpline1 );
	DDX_Control( pDX, IDC_SCRUN_EDIT_SPLINE2, edSpline2 );
}

BOOL CScriptCameraRunDlg::OnInitDialog()
	{
	CResizeDialog::OnInitDialog();

	// CAMERAS
	cbStartCam.ResetContent();
	cbFinishCam.ResetContent();
	int i = 0;
	for ( std::vector<NCamera::CCameraPlacement>::const_iterator it = pDialogData->scriptCameras.begin(); it < pDialogData->scriptCameras.end(); ++it, ++i )
	{
		const NCamera::CCameraPlacement &camera = (*it);
		//
		const int nStringNumber = cbStartCam.AddString( camera.szName.c_str() );
		cbFinishCam.AddString( camera.szName.c_str() );
		cbStartCam.SetItemData( nStringNumber, i );
		cbFinishCam.SetItemData( nStringNumber, i );
	}

	cbStartCam.SelectString( 0, pDialogData->GetStartCamera().szName.c_str() );
	cbFinishCam.SelectString( 0, pDialogData->GetFinishCamera().szName.c_str() );

	// TYPE
	cbType.ResetContent();
	for ( int i = 0; i < typeScriptCameraRunTypeMnemonics.Size(); ++i )
	{
		int nItem = cbType.AddString( typeScriptCameraRunTypeMnemonics.GetMnemonic(i).c_str() );
		cbType.SetItemData( nItem, i );
	}
	cbType.SelectString( 0, typeScriptCameraRunTypeMnemonics.GetMnemonic(pDialogData->eRunType).c_str() );

	SetDialogData( *pDialogData );

	SetControlsMask();

	return true;
}

void CScriptCameraRunDlg::OnOK()
{
	CString szTime, szLinSpeed, szAngSpeed, szTarget, szSpline1, szSpline2;

	const int nType = cbType.GetCurSel();

	pDialogData->eRunType = (NDb::EScriptCameraRunType)nType;
	pDialogData->nStartCamera = cbStartCam.GetCurSel();
	pDialogData->nFinishCamera = cbFinishCam.GetCurSel();

	float fTime = 0;
	float fLinSpeed = 0;
	float fAngSpeed = 0;
	int nTarget = -1;
	float fSpline1 = 0;
	float fSpline2 = 0;

	GetDlgItemText( IDC_SCRUN_EDIT_TIME, szTime );
	sscanf( szTime, "%g", &fTime );

	szScriptText = "@SCRun";
	if ( fTime <= 0 )
	{
		GetDlgItemText( IDC_SCRUN_EDIT_LSPEED, szLinSpeed );
		sscanf( szLinSpeed, "%g", &fLinSpeed );
		NI_ASSERT( fLinSpeed > 0, "Inalid params" );
		if ( fLinSpeed <= 0 )
		{
			CResizeDialog::OnCancel();
			return;
		}
		szScriptText += fmt::format( "Speed(\"{}\", \"{}\", {:g}", pDialogData->GetStartCamera().szName, pDialogData->GetFinishCamera().szName, fLinSpeed ).c_str();
	}
	else
	{
		szScriptText += fmt::format( "Time(" ).c_str();// \"%s\", \"%s\", %g", pDialogData->GetStartCamera().szName, pDialogData->GetFinishCamera().szName, fTime );
		szScriptText += fmt::format( "\"{}\", ", pDialogData->GetStartCamera().szName ).c_str();
		szScriptText += fmt::format( "\"{}\", ", pDialogData->GetFinishCamera().szName, fTime ).c_str();
		szScriptText += fmt::format( "{:g}", fTime ).c_str();
	}

	switch ( pDialogData->eRunType )
	{
	case NDb::SCRT_DIRECT_ROTATE:
		{
			GetDlgItemText( IDC_SCRUN_EDIT_ASPEED, szAngSpeed );
			sscanf( szAngSpeed, "%g", &fAngSpeed );
			szScriptText += fmt::format( ", {}, {:g}", nType, fAngSpeed ).c_str();
		}
		break;
		//
	case NDb::SCRT_DIRECT_FOLLOW:
		{
			GetDlgItemText( IDC_SCRUN_EDIT_TARGET, szTarget );
			sscanf( szTarget, "%d", &nTarget );
			szScriptText += fmt::format( ", {}, {}", nType, nTarget ).c_str();
		}
		break;
		//
	case NDb::SCRT_SPLINE:
		{
			GetDlgItemText( IDC_SCRUN_EDIT_SPLINE1, szSpline1 );
			GetDlgItemText( IDC_SCRUN_EDIT_SPLINE2, szSpline2 );
			sscanf( szSpline1, "%g", &fSpline1 );
			sscanf( szSpline2, "%g", &fSpline2 );
			szScriptText += fmt::format( ", {}, {:g} {:g}", nType, fSpline1, fSpline2 ).c_str();
		}
		break;
	}

	pDialogData->fTime = fTime;
	pDialogData->fLSpeed = fLinSpeed;
	pDialogData->fASpeed = fAngSpeed;
	pDialogData->nTargetScriptID = nTarget;
	pDialogData->fSpline1 = fSpline1;
	pDialogData->fSpline2 = fSpline2;
 
	szScriptText += ")\n";
	CopyScriptToClipboard();

	CResizeDialog::OnOK();
}

BEGIN_MESSAGE_MAP(CScriptCameraRunDlg, CResizeDialog)
	ON_CBN_SELCHANGE(IDC_SCRUN_COMBO_TYPE, OnCbnSelchangeScamTypeCombo)
	ON_EN_UPDATE(IDC_SCRUN_EDIT_TIME, OnEnUpdateScrunEditTime)
END_MESSAGE_MAP()

void CScriptCameraRunDlg::OnCbnSelchangeScamTypeCombo()
{
	const int nType = cbType.GetCurSel();
	pDialogData->eRunType = (NDb::EScriptCameraRunType)nType;
	const int nStart = cbStartCam.GetCurSel();

	SetDialogData( *pDialogData );
	SetControlsMask();
}

void CScriptCameraRunDlg::CopyScriptToClipboard()
{
	OpenClipboard();
	EmptyClipboard();
	HGLOBAL hand = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, szScriptText.StringLength(szScriptText) );
	if ( hand )
	{
		char *ptr = static_cast<char *>( GlobalLock(hand) );
		memcpy( ptr, szScriptText, szScriptText.StringLength(szScriptText) );
		GlobalUnlock( hand );
	}
	SetClipboardData( CF_TEXT, hand );
}

void CScriptCameraRunDlg::SetControlsMask()
{
	edTime.EnableWindow( !bSpeed );
	edLSpeed.EnableWindow( bSpeed );
	edASpeed.EnableWindow( 0 );
	edTarget.EnableWindow( 0 );
	edSpline1.EnableWindow( 0 );
	edSpline2.EnableWindow( 0 );

	switch ( pDialogData->eRunType )
	{
	case NDb::SCRT_DIRECT_MOVE:
		break;
		//
	case NDb::SCRT_DIRECT_ROTATE:
		edASpeed.EnableWindow( 1 );
		break;
		//
	case NDb::SCRT_DIRECT_FOLLOW:
		edTarget.EnableWindow( 1 );
		break;
		//
	case NDb::SCRT_SPLINE:
		edSpline1.EnableWindow( 1 );
		edSpline2.EnableWindow( 1 );
		break;
	}
}

void CScriptCameraRunDlg::SetDialogData( const SScriptCameraRunDlgData &rDialogData )
{
	SetDlgItemText( IDC_SCRUN_EDIT_TIME, fmt::format("{:g}", rDialogData.fTime).c_str() );
	SetDlgItemText( IDC_SCRUN_EDIT_LSPEED, fmt::format("{:g}", rDialogData.fLSpeed).c_str() );
	SetDlgItemText( IDC_SCRUN_EDIT_ASPEED, "" );
	SetDlgItemText( IDC_SCRUN_EDIT_TARGET, "" );
	SetDlgItemText( IDC_SCRUN_EDIT_SPLINE1, "" );
	SetDlgItemText( IDC_SCRUN_EDIT_SPLINE2, "" );

	switch ( rDialogData.eRunType )
	{
	case NDb::SCRT_DIRECT_MOVE:
		break;
		//
	case NDb::SCRT_DIRECT_ROTATE:
		SetDlgItemText( IDC_SCRUN_EDIT_ASPEED, fmt::format("{:g}", rDialogData.fASpeed).c_str() );
		break;
		//
	case NDb::SCRT_DIRECT_FOLLOW:
		SetDlgItemText( IDC_SCRUN_EDIT_TARGET, fmt::format("{}", rDialogData.nTargetScriptID).c_str() );
		break;
		//
	case NDb::SCRT_SPLINE:
		SetDlgItemText( IDC_SCRUN_EDIT_SPLINE1, fmt::format("{:g}", rDialogData.fSpline1).c_str() );
		SetDlgItemText( IDC_SCRUN_EDIT_SPLINE2, fmt::format("{:g}", rDialogData.fSpline2).c_str() );
		break;
	}
}

void CScriptCameraRunDlg::OnEnUpdateScrunEditTime()
{
	pDialogData->nStartCamera = cbStartCam.GetCurSel();
	pDialogData->nFinishCamera = cbFinishCam.GetCurSel();

	float fTime = 0;
	CString szTime;
	GetDlgItemText( IDC_SCRUN_EDIT_TIME, szTime );
	sscanf( szTime, "%f", &fTime );

	if ( fTime > 0 )
	{
		float fLSpeed = fabs(pDialogData->GetStartCamera().vPosition - pDialogData->GetFinishCamera().vPosition)/fTime;
		SetDlgItemText( IDC_SCRUN_EDIT_LSPEED, fmt::format("{:g}", fLSpeed).c_str() );
	}
}



