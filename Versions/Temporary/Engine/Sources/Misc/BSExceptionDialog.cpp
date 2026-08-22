#include "stdafx.h"
#include "BSExceptionDialog.h"
#include "MemoryLib/SymAccess.h"
#include "BSDialogFunctions.h"
#include "resource.h"

#include "port/cdecl.h"

namespace NBSU
{

static BOOL CALLBACK ReportExceptionDlgProc( HWND hwndDlg, unsigned message, WPARAM wParam, LPARAM lParam );

struct SExceptionDlgInitParams
{
	const char *pszCondition;
	const char *pszDescription;
	const std::vector<SCallStackEntry> &entries;
	const char *pszExtInfo;
	//
	SExceptionDlgInitParams( const char *pszNewCondition, const char *pszNewDescription, 
		const std::vector<SCallStackEntry> &_entries, const char *szExtInfo )
		: pszCondition( pszNewCondition ), pszDescription( pszNewDescription ), 
		entries(_entries), pszExtInfo(szExtInfo) {  }
};
static const SExceptionDlgInitParams *g_pParams = 0;

EBSUReport PORT_CDECL ShowExceptionDlg( HINSTANCE hInstance, HWND hWnd,
																		const char *pszCondition, const char *pszDescription, 
																		const std::vector<SCallStackEntry> &entries, const char *pszExtInfo )
{
	WriteReportToFile( "error.txt", pszCondition, pszDescription, entries );
	// remember old cursor before dialog box call
	HCURSOR hCursor = GetCursor();
	// fill start params and execute dialog
	SExceptionDlgInitParams params( pszCondition, pszDescription, entries, pszExtInfo );
	int nRetVal = DialogBoxParam( hInstance,
		"IDD_EXCEPTION_REPORT",
		hWnd,
		reinterpret_cast<DLGPROC>( ReportExceptionDlgProc ),
		reinterpret_cast<LPARAM>( &params ) );
	// restore old cursor after message box usage
	SetCursor( hCursor );

	switch ( nRetVal )
	{
	case IDC_EXCEPTION_DEBUG:
		return BSU_DEBUG;
	case IDC_EXCEPTION_ABORT:
		return BSU_ABORT;
	default:
		return BSU_DEBUG;
	}
}

static BOOL CALLBACK ReportExceptionDlgProc( HWND hwndDlg, unsigned message, WPARAM wParam, LPARAM lParam ) 
{ 
	switch (message) 
	{ 
	case WM_INITDIALOG:
		{
			SExceptionDlgInitParams *pParams = reinterpret_cast<SExceptionDlgInitParams*>( lParam );
			SetDlgUserData( hwndDlg, pParams );
			HWND hwndCallStackList = GetDlgItem( hwndDlg, IDC_EXCEPTION_CALLSTACK );
			FillStackList( hwndCallStackList, pParams->entries );

			SetDlgItemText( hwndDlg, IDC_STATIC_COND, pParams->pszCondition );
			SetDlgItemText( hwndDlg, IDC_STATIC_DESCR, pParams->pszDescription );
		}
		break;
	case WM_COMMAND: 
		switch ( LOWORD(wParam) ) 
		{ 
		case IDC_EXCEPTION_DEBUG:
		case IDC_EXCEPTION_ABORT:
			EndDialog( hwndDlg, LOWORD(wParam) ); 
			return TRUE; 
		case IDC_EXCEPTION_DETAILS:
			{
				SExceptionDlgInitParams *pParams = reinterpret_cast<SExceptionDlgInitParams*>( GetDlgUserData( hwndDlg ) );
				if ( pParams->pszExtInfo )
					MessageBox( hwndDlg, pParams->pszExtInfo, "Details", MB_OK | MB_ICONINFORMATION );
			}
			break;
		case IDC_EXCEPTION_EMERGENCY:
			//NBSU::ExecuteEmergencyCommands();
			EndDialog( hwndDlg, IDC_EXCEPTION_ABORT ); 
			return TRUE;
		} 
		break;
	} 
	return FALSE; 
}

}
