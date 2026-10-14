#pragma once

#include "EffectState.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"

class CEffectEditor : public CEditorBase, public CDefaultView
{
	friend class CEffectState;
	OBJECT_NOCOPY_METHODS( CEffectEditor );
	//Данные специфичные для данного редактрора
	CTPoint<int> terrainSize;
	// Данные общего назначения
	CEffectState *pEffectState;
	//
	const char *GetDesiredMapSeason() const;
	//
	CEffectEditor();
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
		return pEffectState;
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
