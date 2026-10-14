#include "stdafx.h"

#include "EditorAI.h"
#include "EditorScene.h"
#include "MapEditorLib/MapEditorModule.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"

#include "ED_B2_export.h"

#include <zconf.h>

class CEditorModuleB2 : public IEditorModule
{
	void ModuleStartup() override;
	void ModuleShutdown() override;
	void ModuleCreate() override;
	void ModuleDestroy() override;
	void ModuleCreateControls() override;
	void ModulePostCreateControls() override;
	void ModulePreDestroyControls() override;
	void ModuleDestroyControls() override;
	void ModulePostCreateMainFrame() override;
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

ED_B2_EXPORT IEditorModule *GetEditorModule2()
{
	return &theEDB2Module;
}

ED_B2_EXPORT IEditorModule *GetEditorModule3()
{
	return 0;
}

ED_B2_EXPORT IEditorModule *GetEditorModule4()
{
	return 0;
}
