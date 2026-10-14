#pragma once

#include "EditorTestState.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"

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
	void GetTemporaryLabel( std::string *pszTemporaryLabel ) override
	{
		pszTemporaryLabel->clear();
	}

	IView *GetView() override
	{
		return this;
	}

	IInputState *GetInputState() override
	{
		return pEditorTestState;
	}

	void GetChildFrameType( std::string *pszChildFrameTypeName ) override
	{
		( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__";
	}

	void CreateControls() override {}

	void PostCreateControls() override {}

	void PreDestroyControls() override {}

	void DestroyControls() override
	{
		Destroy();
	}

	void Create() override;
	void Destroy() override;

	//CDefaultView
	void Undo( IController *pController ) override {}

	void Redo( IController *pController ) override {}
};
