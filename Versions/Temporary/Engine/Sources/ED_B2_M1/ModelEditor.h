#if !defined(__MODEL_EDITOR__)
#define __MODEL_EDITOR__
#pragma once

#include "..\MapEditorLib\EditorBase.h"
#include "..\MapEditorLib\DefaultView.h"
#include "ModelWindow.h"
#include "ModelEditorSettings.h"

#define TOOLBAR_MODEL_ELEMENTS_COUNT 13

EXTERNVAR const UINT TOOLBAR_MODEL_ELEMENTS_ID[TOOLBAR_MODEL_ELEMENTS_COUNT];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CModelEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	friend class CModelState;
	OBJECT_NOCOPY_METHODS( CModelEditor );

	UINT nModelToolbarID;
	SECControlBar *pwndTool;
	CModelWindow modelWindow;
	bool bPreviousCameraHandleType;
	float fFOV;

	// Данные общего назначения 
	CModelState *pModelState;
	CModelEditorSettings editorSettings;
	//
	CModelEditor();
	//
public:
	//IEditor
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pModelState; }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls();
	void PostCreateControls();
	void PreDestroyControls();
	void DestroyControls();
	void Create();
	void Destroy();

	//CDefaultView
	void Undo( IController* pController ) {}
	void Redo( IController* pController ) {}
	
	//ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	// Icons creations
	void CreateIcon();
	void RestoreCameraParametersFromStats();

	// Targeting for creating icons shots
	void CreateTarget();
	void DeleteTarget();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__MODEL_EDITOR__)
