#pragma once

struct SCallStackEntry;
namespace NBSU
{
void ListView_AddColumn( HWND hwnd, const string &strCaption, int nWidth );
int ListView_AddItem( HWND hwnd, const string &strText, LPARAM lParam, int nItem = 0 );
string GetFileName( const string &strFullPath );

void FillStackList( HWND hwndCallStack, const vector<SCallStackEntry> &entries );

//void AddListBoxItem( HWND hWnd, const char *pszString, const void *pItemData );
//void AddCallStackItem( HWND hWnd, const SCallStackEntry *pEntry );

//const void* GetListBoxItem( HWND hWnd, int nItemIndex );
//const void* GetListBoxCurrentItem( HWND hWnd );
//const SCallStackEntry* GetCallStackItem( HWND hWnd, int nItemIndex );
//const SCallStackEntry* GetCurrentCallStackItem( HWND hWnd );

bool SetWindowText( HWND hwndDlg, const int nElementID, const char *pszString );
//bool SetWindowText( HWND hwndDlg, const int nElementID, const int nValue );

void* SetWindowUserData( HWND hwndDlg, const int nElementID, void *pUserData );
void* GetWindowUserData( HWND hwndDlg, const int nElementID );
void* SetDlgUserData( HWND hwndDlg, void *pUserData );
void* GetDlgUserData( HWND hwndDlg );

//int GetCheckButtonState( HWND hwndDlg, const int nElementID );

void WriteReportToFile( const char *pszFileName, const char *pszCondition, const char *pszDescription, 
	const vector<SCallStackEntry> &entries );
}

