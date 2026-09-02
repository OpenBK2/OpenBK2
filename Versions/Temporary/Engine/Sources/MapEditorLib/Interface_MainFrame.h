#pragma once

#include "Interface_UserData.h"

#include <cstdint>

enum ELogOutputType
{
	LT_NORMAL			= 0,
	LT_IMPORTANT	= 1,
	LT_ERROR			= 2
};


#define SWT_MOD				0x00000001
#define SWT_TYPE			0x00000002
#define SWT_OBJECT		0x00000004
#define SWT_PARAMS		0x00000008
#define SWT_MODIFIED	0x00000010
#define SWT_ALL				0xFFffFFff


struct SSWTParams
{
	uint32_t dwFlags;
	//
	std::string szMOD;
	std::string szType;
	std::string szObject;
	std::string szParams;
	bool bModified;
	bool bFillMODFromBase;
	//
	SSWTParams() : dwFlags( SWT_ALL ), bModified( false ), bFillMODFromBase( true ) {}
};


struct ILogger
{
	virtual void Log( ELogOutputType eLogOutputType, const std::string &szText ) = 0;
	virtual void ClearLog() = 0;
};


struct IMainFrame : public ILogger
{
	// Наити координаты только что нажатой кнопки на toolbar
	virtual bool GetToolBarButtonLeftBottomPos( const CTPoint<int> &rMousePoint,
																							unsigned nButtonID,
																							CTPoint<int> *pLeftBottomPos ) = 0;
	// Работа с ChildFrame
	virtual class SECWorksheet* CreateChildFrame( unsigned nResource ) = 0;
	virtual bool SetChildFrameWindowContents( class SECWorksheet* pwndChildWindow, class CWnd *pwndContents ) = 0;
	// Работа с Docking Window
	virtual class SECControlBar* CreateControlBar( unsigned *pnID,
																								 const CString &rstrTitle,
																								 const unsigned nStyle,
																								 const unsigned nPlace,
																								 const float fRate,
																								 const int nWidth ) = 0;
	virtual bool SetControlBarWindowContents( class SECControlBar* pwndDockingWindow, class CWnd *pwndContents ) = 0;
	// Работа с Menu Bar ( 1 - 20 )
	virtual bool AddMenuResources( std::vector<unsigned> &rMenuIDList ) = 0;
	virtual void ShowMenu( const unsigned nResourceID ) = 0;
	// Работа с Tool Bar
	virtual bool AddToolBarResource( const unsigned nStandartResourceID, const unsigned nLargeResourceID ) = 0;
	virtual void CreateToolBar( unsigned *pnID,
															const CString &rstrTitle,
															const unsigned nButtonCount,
															const unsigned* pButtonIDMap,
															const uint32_t dwAlignment,
															const unsigned nStyle,
															const bool bDocked,
															const bool bVisible,
															const bool bMainToolBar ) = 0;
	virtual class SECCustomToolBar* GetToolBar( unsigned nID ) = 0;
	// Работа с Элементами оформления
	virtual void SetStatusBarText( int nPaneIndex, const std::string &szText ) = 0;
	virtual void SetWindowTitle( const SSWTParams &rSWTParams ) = 0;
	// Работа с DB
	virtual void SaveObjectStorage( int nGDBBrowserID ) = 0;
	virtual void RestoreObjectStorage() = 0;
	virtual bool BrowseLink( std::string *pszResult, const std::string &rszInitialValue, const SPropertyDesc* pPropertyDesc, bool bMultiRef, bool bEnableEdit ) = 0;
	virtual bool BrowseForObject( CDBID *pObjectDBID, std::string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty ) = 0;
	//
	virtual void CreateProgressDialog() = 0;
	virtual void DestroyProgressDialog() = 0;
	//
	virtual void SetProgressDialogTitle( const std::string &rszTitle ) = 0;
	virtual void SetProgressDialogMessage( const std::string &rszMessage ) = 0;
	virtual void SetProgressDialogRange( int nStart, int nFinish ) = 0;
	virtual void SetProgressDialogPosition( int nPosition ) = 0;
	virtual void IterateProgressDialogPosition() = 0;
};


struct IMainFrameContainer : public CObjectBase
{
	enum { tidTypeID = 0x140943C1 };
	// служебный метод ( не используется )
	virtual void Set( class CMainFrame* _pMainFrame ) = 0;
	// получить указатель на стандартный инерфейс IManFrame
	virtual IMainFrame* Get() = 0;
	// получить указательна на главный фрейм приложения ( используется в экстренных случаях )
	virtual class SECWorkbook* GetSECWorkbook() = 0;
};


/**
	virtual bool CreateToolbar() = 0;
	virtual void DestroyToolbar() = 0;
	//
	virtual bool CreateChildWindow() = 0;
	virtual void DestoryChildWindow() = 0;
	//
	virtual bool CreateDockingWindow() = 0;
	virtual void DestoryDockingWindow() = 0;
	//
	virtual bool CreateMenu() = 0;
	virtual void DestoryMenu() = 0;
/**/



