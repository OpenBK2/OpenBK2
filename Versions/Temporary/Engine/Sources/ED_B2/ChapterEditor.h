#pragma once

#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/DefaultView.h"
#include "ChapterState.h"

class CChapterEditorSettings
{
public:
	CDBID dbidTemplateScreenID;

	// serializing...
	int operator&( IXmlSaver &xs );
};

class CChapterEditor : public CEditorBase, public CDefaultView
{
	OBJECT_NOCOPY_METHODS( CChapterEditor );
	//Данные специфичные для данного редактрора
	CChapterEditorSettings editorSettings;
	// Данные общего назначения 
	CChapterState *pChapterState;
	//
	CChapterEditor();
	~CChapterEditor();
	//
public:
	//IEditor
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pChapterState; }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls() {}
	void PostCreateControls() {}
	void PreDestroyControls() {}
	void DestroyControls() {}
	void Create();
	void Destroy();

	//CDefaultView
	void Undo( IController* pController ) {}
	void Redo( IController* pController );
	
	const CChapterEditorSettings GetEditorSettings() const { return editorSettings; }

	IManipulator* CreateChapterManipulator();
};


