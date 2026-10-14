#pragma once

#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"
#include "TextureState.h"

class CTextureEditor : public CEditorBase, public CDefaultView
{
	OBJECT_NOCOPY_METHODS( CTextureEditor );

	//Life-cycle
	CTextureEditor();
	~CTextureEditor() override;

public:
	//IEditor
	void GetTemporaryLabel( std::string *pszTemporaryLabel ) override
	{
		pszTemporaryLabel->clear();
	}

	void GetChildFrameType( std::string *pszChildFrameTypeName ) override
	{
		( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__";
	}

	IView *GetView() override
	{
		return this;
	}

	IInputState *GetInputState() override
	{
		return pState;
	}

	void Create() override;
	void Destroy() override;

	void CreateControls() override {}

	void PostCreateControls() override {}

	void PreDestroyControls() override {}

	void DestroyControls() override {}

	//CDefaultView
	void Undo( IController *pController ) override {}

	void Redo( IController *pController ) override {}

	// members
protected:
	CTextureState *pState;
};
