#include "stdafx.h"
#include "MapEditorLib/MapEditorModule.h"

class CEditorModuleRTS : public IEditorModule
{
	void ModuleStartup();
	void ModuleShutdown();
	void ModuleCreate();
	void ModuleDestroy();
	void ModuleCreateControls();
	void ModulePostCreateControls();
	void ModulePreDestroyControls();
	void ModuleDestroyControls();
	void ModulePostCreateMainFrame();
};

void CEditorModuleRTS::ModuleStartup()
{
	// не существует уже <все> кроме Singleton<IUserDataContainer>()
}

void CEditorModuleRTS::ModuleShutdown()
{
	// существует только Singleton<IUserDataContainer>()
}

void CEditorModuleRTS::ModuleCreate()
{
	// существует уже <все>
	// вызывается до создания MainFrame
}

void CEditorModuleRTS::ModuleDestroy()
{
	// существует еще <все>
	// вызывается после разрушения MainFrame
}

void CEditorModuleRTS::ModuleCreateControls()
{
	// существует уже <все>
	// вызывается после создания MainFrame, перед LoadBarState
}

void CEditorModuleRTS::ModulePostCreateControls()
{
	// существует уже <все>
	// вызывается после создания MainFrame, после LoadBarState
}

void CEditorModuleRTS::ModulePreDestroyControls()
{
	// существует еще <все>
	// вызывается перед разрушением MainFrame, перед SaveBarState
}

void CEditorModuleRTS::ModuleDestroyControls()
{
	// существует еще <все>
	// вызывается после создания MainFrame, после но SaveBarState
}

void CEditorModuleRTS::ModulePostCreateMainFrame()
{
	// вызывается после создания MainFrame и PostStorageInitialize()
}

static CEditorModuleRTS theEDRTSModule;

IEditorModule* GetEditorModule0()
{
	return &theEDRTSModule;
}


