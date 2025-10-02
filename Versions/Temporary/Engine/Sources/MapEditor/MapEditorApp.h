#if !defined(__MAP_EDITOR_APP__)
#define __MAP_EDITOR_APP__
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


struct SCursomToolBarInfo
{
	CString strName;
	int nCount;
	const UINT *pButtons;

	SCursomToolBarInfo() : nCount( 0 ), pButtons( 0 ) {}
	SCursomToolBarInfo( const SCursomToolBarInfo &rCursomToolBarInfo ) 
		: strName( rCursomToolBarInfo.strName ),
			nCount( rCursomToolBarInfo.nCount ),
			pButtons( rCursomToolBarInfo.pButtons ) {}
	SCursomToolBarInfo& operator=( const SCursomToolBarInfo &rCursomToolBarInfo )
	{
		if( &rCursomToolBarInfo != this )
		{
			strName = rCursomToolBarInfo.strName;
			nCount = rCursomToolBarInfo.nCount;
			pButtons = rCursomToolBarInfo.pButtons;
		}
		return *this;
	}
};
typedef vector<SCursomToolBarInfo> CCursomToolBarInfoList;

namespace NVFS
{
	interface IVFS;
	interface IFileCreator;
}

interface IEditorModule;

class CEditorApp : public CWinApp
{
	class CMainFrame *pMainFrame;
	CObj<NVFS::IVFS> pMainVFS;
	CObj<NVFS::IFileCreator> pMainFileCreator;
	//
	void CreateUserDataSingleton();
	bool CreateSingletons();
	void DestroySingletons();
	bool ParseCommandLine( const string &rszCommandLine );
	
	void RegisterEditors();
protected:
	void SetMapFileName( const string &szMapFileName );
public:
	CEditorApp();
	
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//
	virtual BOOL SaveAllModified();
	//
	afx_msg void OnAppAbout();
	afx_msg void OnHelp();

	virtual void LoadMapEditorModule( const string &szModuleName ) = 0;
	virtual void UnloadMapEditorModule() = 0;
	virtual const vector<IEditorModule*>& GetEditorModules() = 0;
	virtual bool GameXInitialize() = 0;
	virtual void GameXPostStorageInitialize() = 0;
	virtual const struct SECBtnMapEntry* GetToolbarButtonsMap() const = 0;
	virtual void GetCursomToolBarsInfo( CCursomToolBarInfoList *pCursomToolBarInfoList ) const = 0;
	virtual void CreateMenus( interface IMainFrame *pMainFrame ) const = 0;
	//
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__MAP_EDITOR_APP__)

