#pragma once

#include "MapEditor_export.h"

#include <string>
#include <vector>


namespace NVFS
{
	struct IVFS;
	struct IFileCreator;
}

struct IEditorModule;

// The editor application: startup, the modules, shutdown.
//
// This was a CWinApp. MFC ran it -- InitInstance, the message loop, then
// ExitInstance -- and the frame found it through AfxGetApp(). MFC is gone;
// the executable's wxApp runs it now (B2_MapEditor/main.cpp), calling
// Initialize from OnInit and Shutdown from OnExit, in the order MFC did, and
// the frame finds it through Get().
class MAPEDITOR_EXPORT CEditorApp
{
	static CEditorApp *pInstance;
	//
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
	virtual ~CEditorApp();

	// The one application object, or null before it is made.
	static CEditorApp* Get() { return pInstance; }

	// What InitInstance did. rszCommandLine is the command line after the
	// program's name, as CWinApp::m_lpCmdLine held it. False when the editor
	// should not start; Shutdown is still to be called then, as MFC called
	// ExitInstance after a failed InitInstance.
	virtual bool Initialize( const std::string &rszCommandLine );
	// What ExitInstance did, less CWinApp's own.
	virtual void Shutdown();

	virtual void LoadMapEditorModule( const std::string &szModuleName ) = 0;
	virtual void UnloadMapEditorModule() = 0;
	virtual const std::vector<IEditorModule*>& GetEditorModules() = 0;
	virtual bool GameXInitialize() = 0;
	virtual void GameXPostStorageInitialize() = 0;
	virtual void CreateMenus( struct IMainFrame *pMainFrame ) const = 0;
};
