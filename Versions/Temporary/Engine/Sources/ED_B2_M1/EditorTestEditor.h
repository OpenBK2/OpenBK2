#if !defined(__EDITOR_TEST_EDITOR__)
#define __EDITOR_TEST_EDITOR__
#pragma once

#include "..\MapEditorLib\EditorBase.h"
#include "..\MapEditorLib\DefaultView.h"
#include "EditorTestState.h"


class CEditorTestEditor : public CEditorBase, public CDefaultView
{
	friend class CEditortestState;

	OBJECT_NOCOPY_METHODS( CEditorTestEditor );
	//Данные специфичные для данного редактрора
	CTPoint<int> terrainSize;
	// Данные общего назначения 
	CEditorTestState *pEditorTestState;
	//
	CEditorTestEditor();
	//
public:
	//IEditor
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pEditorTestState; }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls() {}
	void PostCreateControls() {}
	void PreDestroyControls() {}
	void DestroyControls() { Destroy(); }
	void Create();
	void Destroy();

	//CDefaultView
	void Undo( IController* pController ) {}
	void Redo( IController* pController ) {}
};

#endif // !defined(__EDITOR_TEST_EDITOR__)

