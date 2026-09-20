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
	// bResetProfile is the -reg switch, which used to be a substring search over
	// the whole command line: a map whose name happened to contain "-reg" wiped
	// the profile. It is an exact argument now.
	bool ParseCommandLine( const std::string &rszFileToOpen, bool bResetProfile );

	void RegisterEditors();
protected:
	void SetMapFileName( const std::string &szMapFileName );
public:
	CEditorApp();
	virtual ~CEditorApp();

	// The one application object, or null before it is made.
	static CEditorApp* Get() { return pInstance; }

	// What InitInstance did. rArgs is the command line already split and
	// unquoted, without the program's name: what wx hands its wxApp. It was the
	// whole tail of the command line as one string, the way CWinApp::m_lpCmdLine
	// held it, with the quotes trimmed off by hand afterwards. False when the
	// editor should not start; Shutdown is still to be called then, as MFC
	// called ExitInstance after a failed InitInstance.
	virtual bool Initialize( const std::vector<std::string> &rArgs );
	// What ExitInstance did, less CWinApp's own.
	virtual void Shutdown();

	virtual void LoadMapEditorModule( const std::string &szModuleName ) = 0;
	virtual void UnloadMapEditorModule() = 0;
	virtual const std::vector<IEditorModule*>& GetEditorModules() = 0;
	virtual bool GameXInitialize() = 0;
	virtual void GameXPostStorageInitialize() = 0;
	virtual void CreateMenus( struct IMainFrame *pMainFrame ) const = 0;
};
