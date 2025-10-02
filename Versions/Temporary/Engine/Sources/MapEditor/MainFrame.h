#pragma once

#include "MapEditorSingleton.h"
#include "MainFrameParams.h"

#include "DW_GDBBrowser.h"
#include "DW_PropertyBrowser.h"
#include "DW_Log.h"
#include "ProgressDialog.h"



#define TOOLBARS_COUNT 6
//
#define TOOLBAR_MAIN_ELEMENTS_COUNT 5
#define TOOLBAR_SELECTION_ELEMENTS_COUNT 9
#define TOOLBAR_CC_ELEMENTS_COUNT 2
#define TOOLBAR_OBJECT_ELEMENTS_COUNT 12
#define TOOLBAR_PC_ELEMENTS_COUNT 16
#define TOOLBAR_VIEW_ELEMENTS_COUNT 3
//
#define STATUSBAR_ELEMENTS 3
//
#define DOCKING_WINDOWS_COUNT 3


class CMainFrame : public SECWorkbook, public IMainFrame, public ICommandHandler
{
	static const int WM_SECTOOLBARWNDNOTIFY;
	//
	static const UINT TOOLBAR_ID[TOOLBARS_COUNT];
	static const UINT TOOLBAR_CONTROL_ID[TOOLBARS_COUNT];
	static const UINT TOOLBAR_CONTROL_ID_TO_ARRANGE;
	static const UINT TOOLBAR_NAME_ID[TOOLBARS_COUNT];
	static const DWORD TOOLBAR_STYLE[TOOLBARS_COUNT];
	static const bool TOOLBAR_SHOW[TOOLBARS_COUNT];
	//
	static const UINT TOOLBAR_MAIN_ELEMENTS_ID[TOOLBAR_MAIN_ELEMENTS_COUNT];
	static const UINT TOOLBAR_SELECTION_ELEMENTS_ID[TOOLBAR_SELECTION_ELEMENTS_COUNT];
	static const UINT TOOLBAR_CC_ELEMENTS_ID[TOOLBAR_CC_ELEMENTS_COUNT];
	static const UINT TOOLBAR_OBJECT_ELEMENTS_ID[TOOLBAR_OBJECT_ELEMENTS_COUNT];
	static const UINT TOOLBAR_PC_ELEMENTS_ID[TOOLBAR_PC_ELEMENTS_COUNT];
	static const UINT TOOLBAR_VIEW_ELEMENTS_ID[TOOLBAR_VIEW_ELEMENTS_COUNT];
	//
	static const DWORD TOOLBAR_ELEMENTS_COUNT[TOOLBARS_COUNT];
	static const UINT* TOOLBAR_ELEMENTS_ID[TOOLBARS_COUNT];
	//
	static const UINT STATUSBAR_INDICATORS_ID[STATUSBAR_ELEMENTS];
	static const UINT STATUSBAR_INDICATORS_SIZE[STATUSBAR_ELEMENTS];
	//
	static const UINT DOCKING_WINDOWS_DOCK_STYLE[DOCKING_WINDOWS_COUNT];
	static const UINT DOCKING_WINDOWS_DOCK_PLACE[DOCKING_WINDOWS_COUNT];
	static const float DOCKING_WINDOWS_RATE[DOCKING_WINDOWS_COUNT];
	static const int DOCKING_WINDOWS_WIDTH[DOCKING_WINDOWS_COUNT];
	
	DECLARE_DYNAMIC(CMainFrame)
	//
	CString strHelpFilePath;
	SECStatusBar wndStatusBar;
	CProgressDialog progressDialog;
	HWND hwndPreviousFocusedWindow;
	CMapEditorSingletonApp mapEditorSingletonApp;
	SMainFrameParams params;

	list<CDWGDBBrowser*> gdbBrowserList;						// Окнa базы
	CDWPropertyBrowser wndPropertyBrowser;					// Окно для показывания свойств обьекта в базе
	CDWLog wndLog;																	// Окно лога
	int nFreeToolbarID;
	SSWTParams currentSWTParams;

protected:
	//
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnClose();
	//
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnEndSession( BOOL bEnding );
	//
	afx_msg BOOL OnCopyData( CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	//
	afx_msg	LRESULT OnSECToolBarNotify( WPARAM wParam, LPARAM lParam );
	//
	afx_msg void OnUserCommand( UINT nCommandID );
	afx_msg void OnUpdateUserCommand( CCmdUI *pCmdUI );
	//
	afx_msg void OnToolsCustomize();
	//
	afx_msg void OnViewToolBar( UINT nCommandID );
	afx_msg void OnUpdateViewToolBar( CCmdUI *pCmdUI );
	//
	//afx_msg void OnShowDWGDBBrowser();
	//afx_msg void OnUpdateShowDWGDBBrowser( CCmdUI* pCmdUI );
	afx_msg void OnDWGDBBrowserNew();
	afx_msg void OnDWGDBBrowserRemove();
	afx_msg void OnDWGDBBrowserWindow( UINT nCommandID );
	afx_msg void OnUpdateDWGDBBrowserNew( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDWGDBBrowserRemove( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDWGDBBrowserWindow( CCmdUI *pCmdUI );
	//
	afx_msg void OnShowDWPropertyBrowser();
	afx_msg void OnShowDWLog();
	afx_msg void OnUpdateShowDWPropertyBrowser( CCmdUI* pCmdUI );
	afx_msg void OnUpdateShowDWLog( CCmdUI* pCmdUI );

	virtual BOOL PreCreateWindow(CREATESTRUCT &rCreateStruct );
	//virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam);

public:
	CMainFrame();
	~CMainFrame();
	//
	afx_msg void OnHelpContents();
	afx_msg void OnHelpAbout();
	afx_msg void OnUpdateHelpContents( CCmdUI *pCmdUI );

	//IMainFrame
	bool GetToolBarButtonLeftBottomPos( const CTPoint<int> &rMousePoint,
																			UINT nButtonID,
																			CTPoint<int> *pLeftBottomPos );
	SECWorksheet* CreateChildFrame( UINT nResource );
	bool SetChildFrameWindowContents( SECWorksheet* _pwndChildFrame, class CWnd *pwndContents );
	SECControlBar* CreateControlBar( UINT *pnID,
																	 const CString &rstrTitle,
																	 const UINT nStyle,
																	 const UINT nPlace,
																	 const float fRate,
																	 const int nWidth );
	bool SetControlBarWindowContents( SECControlBar* _pwndDockingWindow, class CWnd *pwndContents );
	bool AddMenuResources( vector<UINT> &rMenuIDList );
	void ShowMenu( const UINT nResourceID );
	bool AddToolBarResource( const UINT nStandartResourceID, const UINT nLargeResourceID );
	void CreateToolBar( UINT *pnID,
											const CString &rstrTitle,
											const UINT nButtonCount,
											const UINT* pButtonIDMap,
											const DWORD dwAlignment,
											const UINT nStyle,
											const bool bDocked,
											const bool bVisible,
											const bool bMainToolBar );
	SECCustomToolBar* GetToolBar( UINT nID );
	void SetStatusBarText( int nPaneIndex, const string &szText );
	void SetWindowTitle( const SSWTParams &rSWTParams );
	//
	void Log( ELogOutputType eLogOutputType, const string &szText );
	void ClearLog();
	//
	void OpenResource( const string &rszResourceName );
	void SaveObjectStorage( int nGDBBrowserID );
	void RestoreObjectStorage();
	bool BrowseLink( string *pszResult, const string &rszInitialValue, const SPropertyDesc* pPropertyDesc, bool bMultiRef, bool bEnableEdit );
	bool BrowseForObject( CDBID *pObjectDBID, string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty );
	bool SaveChanges(  bool bShowConfirmDialog );
	void ReloadData();
	//
	void CreateProgressDialog();
	void DestroyProgressDialog();
	//
	void SetProgressDialogTitle( const string &rszTitle );
	void SetProgressDialogMessage( const string &rszMessage );
	void SetProgressDialogRange( int nStart, int nFinish );
	void SetProgressDialogPosition( int nPosition );
	void IterateProgressDialogPosition();
	//
	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	//
	DECLARE_MESSAGE_MAP()
};


