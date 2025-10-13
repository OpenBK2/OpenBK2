#include "stdafx.h"
#include "resource.h"
#include "MemoryLib/SymAccess.h"
#include "BSUtil.h"
#include "BSDialogFunctions.h"

#if BOOST_OS_WINDOWS
#include <CommCtrl.h>
#endif

#include <cstdint>

//#include "CallStack.h"
//#include "DialogFunctions.h"
//#include "WndUtils.h"

namespace NBSU
{
static int  nSplitter = -1;
static bool isMore = false;
static bool needSaveLog = true;


static BOOL CALLBACK ReportAssertionDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );
static bool ReportAssertionDlgProcCommand( HWND hwndDlg, uint16_t wCtrlID, uint16_t wNotifCode, LPARAM lParam );

struct SAssertionDlgParams
{
	const char *pszFileName;
	int nLineNumber;
	std::string szCondition;
	std::string szDescription;
	const std::vector<SCallStackEntry> &entries;
	SIgnoresList &ignores;
	const char *pszExtInfo;
	//
	SAssertionDlgParams( const char *pszNewFileName, int nNewLineNumber, 
		const char *pszNewCondition, const char *pszNewDescription, 
		const std::vector<SCallStackEntry> &_entries, SIgnoresList &newIgnores, const char *szExtInfo )
		: pszFileName( pszNewFileName ), nLineNumber( nNewLineNumber ), 
		szCondition( pszNewCondition ), szDescription( pszNewDescription ), 
		entries(_entries), ignores( newIgnores ), pszExtInfo( szExtInfo ) {  }
};

EBSUReport ShowAssertionDlg( HINSTANCE hInstance, HWND hWnd,
																		const char *pszFileName, int nLineNumber,
																		const char *_pszCondition, const char *_pszDescription, 
																		const std::vector<SCallStackEntry> &entries, SIgnoresList &ignores,
																		const char *pszExtInfo )
{
	char szLineNumber[64];
	itoa( nLineNumber, szLineNumber, 10 );
	std::string szDescription = std::string( pszFileName ) + "(" + szLineNumber + "): " + _pszDescription;

	WriteReportToFile( "error.txt", _pszCondition, szDescription.c_str(), entries );
	// remember old cursor before dialog box call
	HCURSOR hCursor = GetCursor();
	// fill start params and execute dialog
	SAssertionDlgParams params( pszFileName, nLineNumber, _pszCondition, szDescription.c_str(), entries, ignores, pszExtInfo );
	int nRetVal = DialogBoxParam( hInstance,
		"IDD_ASSERTION_REPORT",
		hWnd,
		reinterpret_cast<DLGPROC>( ReportAssertionDlgProc ),
		reinterpret_cast<LPARAM>( &params ) );
	// restore old cursor after message box usage
	SetCursor( hCursor );

	switch ( nRetVal )
	{
	case IDC_ASSERTION_DEBUG:
		return BSU_DEBUG;
	case IDC_ASSERTION_ABORT:
		return BSU_ABORT;
	case IDC_ASSERTION_CONTINUE:
		return BSU_CONTINUE;
	case IDC_ASSERTION_IGNORE:
		return BSU_IGNORE;
	default:
		return BSU_DEBUG;
	}
}

inline SAssertionDlgParams *GetParams( LPARAM lParam )
{
	return reinterpret_cast<SAssertionDlgParams*>(lParam);
}

static BOOL CALLBACK ReportAssertionDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam ) 
{ 
	switch (message)
	{ 
	case WM_INITDIALOG:
		{			
			// Calculate splitter
			RECT rcDlg, rc;
			GetWindowRect( hwndDlg, &rcDlg );
			GetWindowRect( GetDlgItem( hwndDlg, IDC_STATIC_SPLITTER ), &rc );
			nSplitter = rcDlg.bottom - rcDlg.top - rc.top;
			if ( !isMore )
				rcDlg.bottom -= nSplitter;
			MoveWindow( hwndDlg, (GetSystemMetrics(SM_CXFULLSCREEN) - rcDlg.right + rcDlg.left) / 2,
				(GetSystemMetrics(SM_CYFULLSCREEN) - rcDlg.bottom + rcDlg.top) / 2,
				rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, true );

			SAssertionDlgParams *pParams = GetParams(lParam);
			SetDlgUserData( hwndDlg, pParams );

			SetWindowText( hwndDlg, IDC_ASSERTION_CONDITION, pParams->szCondition.c_str() );
			SetWindowText( hwndDlg, IDC_ASSERTION_DESCRIPTION, pParams->szDescription.c_str() );
			CheckDlgButton( hwndDlg, IDC_ASSERTION_SAVE_LOG, needSaveLog );

			HWND hwndCallStack = GetDlgItem( hwndDlg, IDC_ASSERTION_CALLSTACK );
			FillStackList( hwndCallStack, pParams->entries );

			// Fill ingnore list
			SetWindowUserData( hwndDlg, IDC_ASSERTION_IGNORES_LIST, &(pParams->ignores) );
			HWND hwndList = GetDlgItem( hwndDlg, IDC_ASSERTION_IGNORES_LIST );
			SendMessage( hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)LVS_EX_FULLROWSELECT );
			GetClientRect( hwndList, &rc );
			int nPart10 = (rc.right - rc.left) / 10;
			ListView_AddColumn( hwndList, "File", nPart10 * 3 );
			ListView_AddColumn( hwndList, "Method", nPart10 * 3 );
			ListView_AddColumn( hwndList, "Line", nPart10 );
			ListView_AddColumn( hwndList, "Condition", nPart10 * 3 );
			for( SIgnoresList::iterator it = pParams->ignores.begin(); it != pParams->ignores.end(); ++it )
			{
				int nNewLine = ListView_AddItem( hwndList, GetFileName(it->szFileName), LPARAM(&(*it)) );
				ListView_SetItemText( hwndList, nNewLine, 3, const_cast<char*>(it->szCondition.c_str()) );
				ListView_SetItemText( hwndList, nNewLine, 1, const_cast<char*>(it->szFunctionName.c_str()) );
				char buf[100];
				itoa( it->nLineNumber, buf, 10 );
				ListView_SetItemText( hwndList, nNewLine, 2, buf );
			}
		}
		break;

	case WM_NOTIFY:
		{
			int idCtrl = (int) wParam;
			if ( IDC_ASSERTION_IGNORES_LIST == idCtrl )
			{
				LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
				HWND hwndList = GetDlgItem( hwndDlg, IDC_ASSERTION_IGNORES_LIST );
				switch( lpnmitem->hdr.code )
				{
				case NM_DBLCLK:
					{
						LVITEM Item;
						memset( &Item, 0, sizeof(Item) );
						Item.mask = LVIF_PARAM | LVIF_TEXT;
						Item.iItem = lpnmitem->iItem;
						SIgnoresList *pIgnores = reinterpret_cast<SIgnoresList*>( GetWindowUserData( hwndDlg, IDC_ASSERTION_IGNORES_LIST ) );
						ListView_GetItem( hwndList, &Item );
						SIgnoresEntry* pIEnt = reinterpret_cast<SIgnoresEntry*>(Item.lParam);
						pIgnores->remove(*pIEnt);
						ListView_DeleteItem( hwndList, lpnmitem->iItem );
					}
					break;
				}
			}
		}
		break;

	case WM_COMMAND:
		return ReportAssertionDlgProcCommand( hwndDlg, LOWORD(wParam), HIWORD(wParam), lParam );
	} 
	return false;
}

bool ReportAssertionDlgProcCommand( HWND hwndDlg, uint16_t wCtrlID, uint16_t wNotifCode, LPARAM lParam )
{
	switch(wCtrlID)
	{ 
	case IDC_ASSERTION_DETAILS:
		{
			SAssertionDlgParams *pParams = reinterpret_cast<SAssertionDlgParams*>( GetDlgUserData( hwndDlg ) );
			if ( pParams->pszExtInfo )
				MessageBox( hwndDlg, pParams->pszExtInfo, "Details", MB_OK | MB_ICONINFORMATION );
		}
		break;
	case IDC_BUTTON_MORE:
		{
			RECT rc;
			GetWindowRect( hwndDlg, &rc );
			rc.bottom += ( isMore = !isMore ) ? nSplitter : -nSplitter;
			MoveWindow( hwndDlg, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, true );
			SetWindowText( hwndDlg, IDC_BUTTON_MORE, isMore ? "&More <<" : "&More >>" );
		}
		break;
	case IDC_ASSERTION_SAVE_LOG:
		needSaveLog = IsDlgButtonChecked( hwndDlg, IDC_ASSERTION_SAVE_LOG ) == 1;
		break;

		//case IDC_ASSERTION_DETAILS:
		//{
		//	SExternalDebugAllocStat str[4];
		//	str[0].pszFileName = "AAA";
		//	str[1].pszFileName = "BB";
		//	str[2].pszFileName = "AA";
		//	str[3].pszFileName = "BB";
		//	CDialogMemory dlg( str, 4 );
		//	dlg.DoModal( g_hInstance );
		//}
		//break;

	case IDC_ASSERTION_DEBUG:
	case IDC_ASSERTION_ABORT:
	case IDC_ASSERTION_CONTINUE:
		EndDialog( hwndDlg, wCtrlID );
		return true;

	case IDC_ASSERTION_IGNORE: // add current assertion to the ignores list
		{
			SIgnoresList *pIgnores = reinterpret_cast<SIgnoresList*>( GetWindowUserData( hwndDlg, IDC_ASSERTION_IGNORES_LIST ) );
			if ( pIgnores )
			{
				SIgnoresEntry ignore;
				//GetCheckButtonState( hwndDlg, IDC_ASSERTION_IGNORE_THIS );
				SAssertionDlgParams *pParams = reinterpret_cast<SAssertionDlgParams*>( GetDlgUserData( hwndDlg ) );
				if ( pParams )
				{
					if ( !pParams->entries.empty() )
						ignore.szFunctionName = pParams->entries[0].szFunc.szStr;
					else
						ignore.szFunctionName = "<UNKNOWN>";
					ignore.szCondition = pParams->szCondition;
					ignore.szFileName = pParams->pszFileName;
					ignore.nLineNumber = pParams->nLineNumber;
					ignore.dwFlags = IGNORE_THIS;
					//ignore.dwFlags = ( GetCheckButtonState( hwndDlg, IDC_ASSERTION_IGNORE_THIS     ) == BST_CHECKED ? IGNORE_THIS     : 0 ) |
					//								 ( GetCheckButtonState( hwndDlg, IDC_ASSERTION_IGNORE_NON_THIS ) == BST_CHECKED ? IGNORE_NON_THIS : 0 ) |
					//								 ( GetCheckButtonState( hwndDlg, IDC_ASSERTION_IGNORE_FILE     ) == BST_CHECKED ? IGNORE_FILE     : 0 ) |
					//								 ( GetCheckButtonState( hwndDlg, IDC_ASSERTION_IGNORE_NON_FILE ) == BST_CHECKED ? IGNORE_NON_FILE : 0 ) |
					//								 ( GetCheckButtonState( hwndDlg, IDC_ASSERTION_IGNORE_ALL      ) == BST_CHECKED ? IGNORE_ALL      : 0 );

					pIgnores->push_back( ignore );
				}
			}
		}
		EndDialog( hwndDlg, IDC_ASSERTION_CONTINUE );
		return true;
	case IDC_ASSERTION_EMERGENCY:
		//NBSU::ExecuteEmergencyCommands();
		EndDialog( hwndDlg, IDC_ASSERTION_ABORT );
		return true;
	}
	return false;
}

}
