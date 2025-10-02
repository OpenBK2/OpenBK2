#pragma once

#include "../MapEditorLib/EditorBase.h"
#include "../MapEditorLib/DefaultView.h"
#include "TextureState.h"


class CTextureEditor : public CEditorBase, public CDefaultView
{
	OBJECT_NOCOPY_METHODS( CTextureEditor );

	//Life-cycle
	CTextureEditor();
	~CTextureEditor();

public:
	//IEditor
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	IView* GetView() { return this;  }
	IInputState* GetInputState() { return pState; }
	void Create();
	void Destroy();
	void CreateControls() {}
	void PostCreateControls() {}
	void PreDestroyControls() {}
	void DestroyControls() {}

	//CDefaultView
	void Undo( IController* pController ) {}
	void Redo( IController* pController ) {}

	// members
protected:
	CTextureState * pState;
};


