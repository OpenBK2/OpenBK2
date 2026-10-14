#pragma once

#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "TerrainState.h"

#include <cstdint>

class CTerrainEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	friend class CTerrainState;
	OBJECT_NOCOPY_METHODS( CTerrainEditor );
	//Данные специфичные для данного редактрора
	// Данные общего назначения
	CTerrainState *pTerrainState;
	//
	CTerrainEditor();
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
		return pTerrainState;
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

	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override
	{
		return false;
	}

	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override
	{
		return false;
	}
};
