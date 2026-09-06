#pragma once

#include "FrameHandles.h"
#include "MapEditorLib/MfcWidget.h"

#include <list>
#include <map>

#include "MapEditorSingleton.h"
#include "MainFrameParams.h"

#include "DW_GDBBrowser.h"
#include "DW_PropertyBrowser.h"
#include "DW_Log.h"
#include "ProgressDialog.h"

#include <cstdint>

#define TOOLBARS_COUNT 6
//
#define TOOLBAR_MAIN_ELEMENTS_COUNT 7
#define TOOLBAR_SELECTION_ELEMENTS_COUNT 9
#define TOOLBAR_CC_ELEMENTS_COUNT 2
#define TOOLBAR_OBJECT_ELEMENTS_COUNT 12
#define TOOLBAR_PC_ELEMENTS_COUNT 16
#define TOOLBAR_VIEW_ELEMENTS_COUNT 3
//
#define STATUSBAR_ELEMENTS 3
//
#define DOCKING_WINDOWS_COUNT 3


class CMainFrame : public SECWorkbook, public IMainFrame, public ICommandHandler, public IWidget
{
	static const int WM_SECTOOLBARWNDNOTIFY;
	//
	static const unsigned TOOLBAR_ID[TOOLBARS_COUNT];
	static const unsigned TOOLBAR_CONTROL_ID[TOOLBARS_COUNT];
	static const unsigned TOOLBAR_CONTROL_ID_TO_ARRANGE;
	static const unsigned TOOLBAR_NAME_ID[TOOLBARS_COUNT];
	static const uint32_t TOOLBAR_STYLE[TOOLBARS_COUNT];
	static const bool TOOLBAR_SHOW[TOOLBARS_COUNT];
	//
	static const unsigned TOOLBAR_MAIN_ELEMENTS_ID[TOOLBAR_MAIN_ELEMENTS_COUNT];
	static const unsigned TOOLBAR_SELECTION_ELEMENTS_ID[TOOLBAR_SELECTION_ELEMENTS_COUNT];
	static const unsigned TOOLBAR_CC_ELEMENTS_ID[TOOLBAR_CC_ELEMENTS_COUNT];
	static const unsigned TOOLBAR_OBJECT_ELEMENTS_ID[TOOLBAR_OBJECT_ELEMENTS_COUNT];
	static const unsigned TOOLBAR_PC_ELEMENTS_ID[TOOLBAR_PC_ELEMENTS_COUNT];
	static const unsigned TOOLBAR_VIEW_ELEMENTS_ID[TOOLBAR_VIEW_ELEMENTS_COUNT];
	//
	static const uint32_t TOOLBAR_ELEMENTS_COUNT[TOOLBARS_COUNT];
	static const unsigned* TOOLBAR_ELEMENTS_ID[TOOLBARS_COUNT];
	//
	static const unsigned STATUSBAR_INDICATORS_ID[STATUSBAR_ELEMENTS];
	static const unsigned STATUSBAR_INDICATORS_SIZE[STATUSBAR_ELEMENTS];
	//
	static const unsigned DOCKING_WINDOWS_DOCK_STYLE[DOCKING_WINDOWS_COUNT];
	static const unsigned DOCKING_WINDOWS_DOCK_PLACE[DOCKING_WINDOWS_COUNT];
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

	std::list<CDWGDBBrowser*> gdbBrowserList;						// Окнa базы
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
	afx_msg void OnUserCommand( unsigned nCommandID );
	afx_msg void OnUpdateUserCommand( CCmdUI *pCmdUI );
	//
	afx_msg void OnToolsCustomize();
	//
	afx_msg void OnViewToolBar( unsigned nCommandID );
	afx_msg void OnResetGUI();
	afx_msg void OnRegisterXDB();
	afx_msg void OnUpdateViewToolBar( CCmdUI *pCmdUI );
	//
	//afx_msg void OnShowDWGDBBrowser();
	//afx_msg void OnUpdateShowDWGDBBrowser( CCmdUI* pCmdUI );
	afx_msg void OnDWGDBBrowserNew();
	afx_msg void OnDWGDBBrowserRemove();
	afx_msg void OnDWGDBBrowserWindow( unsigned nCommandID );
	afx_msg void OnUpdateDWGDBBrowserNew( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDWGDBBrowserRemove( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDWGDBBrowserWindow( CCmdUI *pCmdUI );
	//
	afx_msg void OnShowDWPropertyBrowser();
	afx_msg void OnShowDWLog();
	afx_msg void OnUpdateShowDWPropertyBrowser( CCmdUI* pCmdUI );
	afx_msg void OnUpdateShowDWLog( CCmdUI* pCmdUI );

	virtual BOOL PreCreateWindow(CREATESTRUCT &rCreateStruct );
	//virtual LRESULT WindowProc( unsigned message, WPARAM wParam, LPARAM lParam);

public:
	CMainFrame();
	~CMainFrame();
	//
	afx_msg void OnHelpContents();
	afx_msg void OnHelpAbout();
	afx_msg void OnUpdateHelpContents( CCmdUI *pCmdUI );

	// IWidget: this frame is what dialogs, message boxes and popup menus are
	// parented on, and IMainFrameContainer hands it out for that.
	DECLARE_CWND_WIDGET();

	// Handles handed out through IMainFrame. Never removed: an editor may
	// still hold one after its window is gone, and IsAlive is how it finds
	// out. Toolbars are keyed by id because the toolbar manager owns them and
	// they outlive individual documents.
	std::list<CDockPanelHandle> dockPanelHandles;
	std::list<CFrameWindowHandle> frameWindowHandles;
	std::map<unsigned, CToolBarHandle> toolBarHandles;

	//IMainFrame
	bool GetToolBarButtonLeftBottomPos( const CTPoint<int> &rMousePoint,
																			unsigned nButtonID,
																			CTPoint<int> *pLeftBottomPos );
	IFrameWindow* CreateChildFrame( unsigned nResource );
	bool SetChildFrameWindowContents( IFrameWindow* pChildFrame, IWidget *pContents );
	IDockPanel* CreateControlBar( unsigned *pnID,
																	 const std::string &rszTitle,
																	 const unsigned nStyle,
																	 const unsigned nPlace,
																	 const float fRate,
																	 const int nWidth );
	bool SetControlBarWindowContents( IDockPanel* pDockPanel, IWidget *pContents );
	bool AddMenuResources( std::vector<unsigned> &rMenuIDList );
	void ShowMenu( const unsigned nResourceID );
	bool AddToolBarResource( const unsigned nStandartResourceID, const unsigned nLargeResourceID );
	void CreateToolBar( unsigned *pnID,
											const std::string &rszTitle,
											const unsigned nButtonCount,
											const unsigned* pButtonIDMap,
											const uint32_t dwAlignment,
											const unsigned nStyle,
											const bool bDocked,
											const bool bVisible,
											const bool bMainToolBar );
	IToolBar* GetToolBar( unsigned nID );
	void SetStatusBarText( int nPaneIndex, const std::string &szText );
	void SetWindowTitle( const SSWTParams &rSWTParams );
	//
	void Log( ELogOutputType eLogOutputType, const std::string &szText );
	void ClearLog();
	//
	void OpenResource( const std::string &rszResourceName );
	void SaveObjectStorage( int nGDBBrowserID );
	void RestoreObjectStorage();
	bool BrowseLink( std::string *pszResult, const std::string &rszInitialValue, const SPropertyDesc* pPropertyDesc, bool bMultiRef, bool bEnableEdit );
	bool BrowseForObject( CDBID *pObjectDBID, std::string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty );
	bool SaveChanges(  bool bShowConfirmDialog );
	void ReloadData();
	//
	void CreateProgressDialog();
	void DestroyProgressDialog();
	//
	void SetProgressDialogTitle( const std::string &rszTitle );
	void SetProgressDialogMessage( const std::string &rszMessage );
	void SetProgressDialogRange( int nStart, int nFinish );
	void SetProgressDialogPosition( int nPosition );
	void IterateProgressDialogPosition();
	//
	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	//
	DECLARE_MESSAGE_MAP()
};


