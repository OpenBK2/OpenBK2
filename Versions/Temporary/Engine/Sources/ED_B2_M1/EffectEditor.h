#if !defined(__EFFECT_EDITOR__)
#define __EFFECT_EDITOR__
#pragma once

#include "..\MapEditorLib\EditorBase.h"
#include "..\MapEditorLib\DefaultView.h"
#include "EffectState.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pEffectState; }
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__EFFECT_EDITOR__)
