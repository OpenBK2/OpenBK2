#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "MapEditor_export.h"

struct SCursomToolBarInfo
{
	CString strName;
	int nCount;
	const unsigned *pButtons;

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
typedef std::vector<SCursomToolBarInfo> CCursomToolBarInfoList;

namespace NVFS
{
	struct IVFS;
	struct IFileCreator;
}

struct IEditorModule;

class MAPEDITOR_EXPORT CEditorApp : public CWinApp
{
	class CMainFrame *pMainFrame;
	CObj<NVFS::IVFS> pMainVFS;
	CObj<NVFS::IFileCreator> pMainFileCreator;
	//
	void CreateUserDataSingleton();
	bool CreateSingletons();
	void DestroySingletons();
	bool ParseCommandLine( const std::string &rszCommandLine );
	
	void RegisterEditors();
protected:
	void SetMapFileName( const std::string &szMapFileName );
public:
	CEditorApp();
	
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//
	virtual BOOL SaveAllModified();
	//
	afx_msg void OnAppAbout();
	afx_msg void OnHelp();

	virtual void LoadMapEditorModule( const std::string &szModuleName ) = 0;
	virtual void UnloadMapEditorModule() = 0;
	virtual const std::vector<IEditorModule*>& GetEditorModules() = 0;
	virtual bool GameXInitialize() = 0;
	virtual void GameXPostStorageInitialize() = 0;
	virtual const struct SECBtnMapEntry* GetToolbarButtonsMap() const = 0;
	virtual void GetCursomToolBarsInfo( CCursomToolBarInfoList *pCursomToolBarInfoList ) const = 0;
	virtual void CreateMenus( struct IMainFrame *pMainFrame ) const = 0;
	//
	DECLARE_MESSAGE_MAP()
};


