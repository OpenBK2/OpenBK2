#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "MapEditor_export.h"


namespace NVFS
{
	struct IVFS;
	struct IFileCreator;
}

struct IEditorModule;

class MAPEDITOR_EXPORT CEditorApp : public CWinApp
{
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

	virtual void LoadMapEditorModule( const std::string &szModuleName ) = 0;
	virtual void UnloadMapEditorModule() = 0;
	virtual const std::vector<IEditorModule*>& GetEditorModules() = 0;
	virtual bool GameXInitialize() = 0;
	virtual void GameXPostStorageInitialize() = 0;
	virtual void CreateMenus( struct IMainFrame *pMainFrame ) const = 0;
	//
	DECLARE_MESSAGE_MAP()
};


