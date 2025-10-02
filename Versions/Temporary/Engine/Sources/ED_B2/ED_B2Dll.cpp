#include "StdAfx.h"

#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "../MapeditorLib/MapEditorModule.h"
#include "EditorAI.h"
#include "EditorScene.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


class CEditorModuleB2 : public IEditorModule
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

void CEditorModuleB2::ModuleStartup()
{
	// не существует уже <все> кроме Singleton<IUserDataContainer>()
}

void CEditorModuleB2::ModuleShutdown()
{
	// существует только Singleton<IUserDataContainer>()
}

void CEditorModuleB2::ModuleCreate()
{
	// существует уже <все>
	// вызывается до создания MainFrame
	NSingleton::RegisterSingleton( new CEditorAI(), IEditorAI::tidTypeID );
	NSingleton::RegisterSingleton( new CEditorScene(), IEditorScene::tidTypeID );
}

void CEditorModuleB2::ModuleDestroy()
{
	// существует еще <все>
	// вызывается после разрушения MainFrame
	NSingleton::UnRegisterSingleton( IEditorAI::tidTypeID );
	NSingleton::UnRegisterSingleton( IEditorScene::tidTypeID );
}

void CEditorModuleB2::ModuleCreateControls()
{
	// существует уже <все>
	// вызывается после создания MainFrame, перед LoadBarState
}

void CEditorModuleB2::ModulePostCreateControls()
{
	// существует уже <все>
	// вызывается после создания MainFrame, после LoadBarState
}

void CEditorModuleB2::ModulePreDestroyControls()
{
	// существует еще <все>
	// вызывается перед разрушением MainFrame, перед SaveBarState
}

void CEditorModuleB2::ModuleDestroyControls()
{
	// существует еще <все>
	// вызывается после создания MainFrame, после SaveBarState
}

void CEditorModuleB2::ModulePostCreateMainFrame()
{
	// вызывается после создания MainFrame и PostStorageInitialize()
}

static CEditorModuleB2 theEDB2Module;

IEditorModule* GetEditorModule2()
{
	return &theEDB2Module;
}

IEditorModule* GetEditorModule3() { return 0; }
IEditorModule* GetEditorModule4() { return 0; }

