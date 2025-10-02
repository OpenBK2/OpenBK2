#include "StdAfx.h"
#include "ChapterEditor.h"
#include "../MapEditorLib/EditorFactory.h"
#include "..\mapeditorlib\interface_commandhandler.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commoneditormethods.h"
#include "..\MapEditorLib\Interface_UserData.h"
#include "../libdb/ResourceManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

REGISTER_EDITOR_IN_DLL( Chapter, CChapterEditor )

// CChapterEditor

CChapterEditor::CChapterEditor() :
	pChapterState( 0 )
{
}

CChapterEditor::~CChapterEditor()
{
	//Destroy();
}

void CChapterEditor::Create()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->SerializeSettings( editorSettings, "Chapter", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );

	if ( pChapterState == 0 )
	{
		pChapterState = new CChapterState( this );
	}
}

void CChapterEditor::Destroy()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->SerializeSettings( editorSettings, "Chapter", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );

	if ( pChapterState )
	{
		delete pChapterState;
		pChapterState = 0;
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
}

IManipulator* CChapterEditor::CreateChapterManipulator()
{
	IManipulator* pChapterManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( 
		GetObjectSet().szObjectTypeName, 
		GetObjectSet().objectNameSet.begin()->first );
	return pChapterManipulator;
}

void CChapterEditor::Redo( IController* pController )
{
	if ( pChapterState )
	{
		pChapterState->UpdateView();
	}
}

int CChapterEditorSettings::operator&( IXmlSaver &xs )
{
	xs.Add( "TemplateScreenID", &dbidTemplateScreenID );
	return 0;
}
