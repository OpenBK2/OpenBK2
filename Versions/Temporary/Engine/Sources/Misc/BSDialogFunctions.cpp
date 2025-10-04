#include "stdafx.h"
#include "BSDialogFunctions.h"
#include "MemoryLib/SymAccess.h"
#include <CommCtrl.h>

namespace NBSU
{

//inline char *CC( const string &str )
//{
//	return const_cast<char*>(str.c_str());
//}

void ListView_AddColumn( HWND hwnd, const string &strCaption, int nWidth )
{
	LV_COLUMN  Column;
	memset( &Column, 0, sizeof( LV_COLUMN) );

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	Column.pszText = const_cast<char*>(strCaption.c_str());
	Column.cx = nWidth;
	//Column.iSubItem = nNumber;

	ListView_InsertColumn( hwnd, 1000, &Column );
}

int ListView_AddItem( HWND hwnd, const string &strText, LPARAM lParam, int nItem )
{
	LVITEM Item;
	memset( &Item, 0, sizeof(Item) );
	Item.mask = LVIF_PARAM | LVIF_TEXT;
	Item.iItem = nItem;
	Item.lParam = lParam;
	Item.pszText = const_cast<char*>(strText.c_str());
	Item.cchTextMax = strText.size();
	return ListView_InsertItem( hwnd, &Item );
}

string GetFileName( const string &strFullPath )
{
	return strFullPath.substr( strFullPath.rfind( '\\' ) + 1 );
}

void FillStackList( HWND hwndCallStack, const vector<SCallStackEntry> &entries )
{
	SendMessage( hwndCallStack, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)LVS_EX_FULLROWSELECT );
	RECT rc;
	GetClientRect( hwndCallStack, &rc );
	int nPart10 = (rc.right - rc.left) / 10;
	ListView_AddColumn( hwndCallStack, "File", nPart10 * 3 );	// 0
	ListView_AddColumn( hwndCallStack, "Method", nPart10 * 5 );	// 1
	ListView_AddColumn( hwndCallStack, "Line", nPart10 * 2 );	// 2
	ListView_AddColumn( hwndCallStack, "Module", nPart10 * 3 );	// 3
	ListView_AddColumn( hwndCallStack, "Param1", nPart10 * 2);	// 4
	ListView_AddColumn( hwndCallStack, "Param2", nPart10 * 2 );
	ListView_AddColumn( hwndCallStack, "Param3", nPart10 * 2 );
	ListView_AddColumn( hwndCallStack, "Param4", nPart10 * 2 );
	for ( int i = 0; i < entries.size(); ++i )
	{
		const SCallStackEntry &e = entries[i];
		char buf[100];
		int nNewLine = ListView_AddItem( hwndCallStack, GetFileName( e.szFile.szStr ), LPARAM(&e), 2000000 );
		ListView_SetItemText( hwndCallStack, nNewLine, 1, const_cast<char*>( e.szFunc.szStr ) );
		itoa( e.nLine, buf, 10 );
		ListView_SetItemText( hwndCallStack, nNewLine, 2, buf );
		CSymString szModule;
		GetSymEngine().GetSymbol( e.dwAddress, &szModule, 0, 0, 0 );
		ListView_SetItemText( hwndCallStack, nNewLine, 3, szModule.szStr );
//		buf[0] = '0';
//		buf[1] = 'x';
//		itoa( pos->params[0], &buf[2], 16 );
//		ListView_SetItemText( hwndCallStack, nNewLine, 4, buf );
//		itoa( pos->params[1], &buf[2], 16 );
//		ListView_SetItemText( hwndCallStack, nNewLine, 5, buf );
//		itoa( pos->params[2], &buf[2], 16 );
//		ListView_SetItemText( hwndCallStack, nNewLine, 6, buf );
//		itoa( pos->params[3], &buf[2], 16 );
//		ListView_SetItemText( hwndCallStack, nNewLine, 7, buf );
	}
}

//void AddListBoxItem( HWND hWnd, const char *pszString, const void *pItemData )
//{
//  int nItem = SendMessage( hWnd, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>( pszString ) );
//  SendMessage( hWnd, LB_SETITEMDATA, nItem, reinterpret_cast<LPARAM>( pItemData ) );
//}
//void AddCallStackItem( HWND hWnd, const SCallStackEntry *pEntry )
//{
//	AddListBoxItem( hWnd, pEntry->pszFunctionName, pEntry );
//}

//const void* GetListBoxItem( HWND hWnd, int nItemIndex )
//{
//	long nVal = SendMessage( hWnd, LB_GETITEMDATA, nItemIndex, 0 );
//	if ( nVal == LB_ERR )
//		return 0;
//	else
//		return reinterpret_cast<const void*>( nVal );
//}
//const void* GetListBoxCurrentItem( HWND hWnd )
//{
//	int nItem = SendMessage( hWnd, LB_GETCURSEL, 0, 0 );
//	if ( nItem == LB_ERR )
//		return 0;
//	else
//		return GetListBoxItem( hWnd, nItem );
//}
//const SCallStackEntry* GetCallStackItem( HWND hWnd, int nItemIndex )
//{
//	return reinterpret_cast<const SCallStackEntry*>( GetListBoxItem(hWnd, nItemIndex) );
//}
//const SCallStackEntry* GetCurrentCallStackItem( HWND hWnd )
//{
//	return reinterpret_cast<const SCallStackEntry*>( GetListBoxCurrentItem(hWnd) );
//}

bool SetWindowText( HWND hwndDlg, const int nElementID, const char *pszString )
{
	return SetWindowText( GetDlgItem(hwndDlg, nElementID), pszString ) != FALSE;
}
//bool SetWindowText( HWND hwndDlg, const int nElementID, const int nValue )
//{
//	char buff[32];
//	sprintf( buff, "%d", nValue );
//	return SetWindowText( hwndDlg, nElementID, buff );
//}

void* SetWindowUserData( HWND hwndDlg, const int nElementID, void *pUserData )
{
	return reinterpret_cast<void*>( SetWindowLong( GetDlgItem(hwndDlg, nElementID), 
		                                             GWL_USERDATA, reinterpret_cast<LONG>(pUserData) ) );
}
void* GetWindowUserData( HWND hwndDlg, const int nElementID )
{
	return reinterpret_cast<void*>( GetWindowLong( GetDlgItem(hwndDlg, nElementID), GWL_USERDATA ) );
}
void* SetDlgUserData( HWND hwndDlg, void *pUserData )
{
	return reinterpret_cast<void*>( SetWindowLong( hwndDlg, DWL_USER, reinterpret_cast<LONG>(pUserData) ) );
}
void* GetDlgUserData( HWND hwndDlg )
{
	return reinterpret_cast<void*>( GetWindowLong( hwndDlg, DWL_USER ) );
}

//int GetCheckButtonState( HWND hwndDlg, const int nElementID )
//{
//	return SendMessage( GetDlgItem(hwndDlg, nElementID), BM_GETCHECK, 0, 0 );
//}

void WriteReportToFile( const char *pszFileName, const char *pszCondition, const char *pszDescription, 
												const vector<SCallStackEntry> &entries )
{
	char buffer[1024];
	// create file name
	{
		const int nSize = ::GetModuleFileName( 0, buffer, 1024 );
		char *ptr = buffer;
		for ( char *it = buffer + nSize - 1; it > buffer; --it )
		{
			if ( *it == '\\' ) 
			{
				ptr = it + 1;
				*(it + 1) = 0;
				break;
			}
		}
		NI_ASSERT( ptr != buffer, "Can't compoe name - can't find path" );
		strcpy( ptr, pszFileName );
	}
	//
	if ( FILE *file = fopen(buffer, "at") ) 
	{
		fprintf( file, "%s\n", pszCondition );
		fprintf( file, "%s\n\n", pszDescription );
		for ( int i = 0; i < entries.size(); ++i )
		{
			const SCallStackEntry &e = entries[i];
			fprintf( file, "%s(%d): %s\n", e.szFile.szStr, e.nLine, e.szFunc.szStr );
		}
		fprintf( file, "\n\n" );
		//
		fflush( file );
		fclose( file );
	}
}

}
