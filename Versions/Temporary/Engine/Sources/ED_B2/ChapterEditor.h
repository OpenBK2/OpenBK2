#pragma once

#include "ChapterState.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"

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
	~CChapterEditor() override;
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
		return pChapterState;
	}

	void GetChildFrameType( std::string *pszChildFrameTypeName ) override
	{
		( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__";
	}

	void CreateControls() override {}

	void PostCreateControls() override {}

	void PreDestroyControls() override {}

	void DestroyControls() override {}

	void Create() override;
	void Destroy() override;

	//CDefaultView
	void Undo( IController *pController ) override {}

	void Redo( IController *pController ) override;

	const CChapterEditorSettings GetEditorSettings() const
	{
		return editorSettings;
	}

	IManipulator *CreateChapterManipulator();
};
