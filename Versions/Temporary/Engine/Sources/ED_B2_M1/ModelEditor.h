#pragma once

#include "ED_B2_M1_export.h"
#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/DefaultView.h"
// ModelView.h rather than ModelWindow.h: which toolkit draws the palette is
// NModelView's business. ModelState.h was reached through ModelWindow.h and is
// needed here in its own right, for GetInputState.
#include "ModelView.h"
#include "ModelState.h"
#include "ModelEditorSettings.h"

#include <cstdint>
#include <memory>

#define TOOLBAR_MODEL_ELEMENTS_COUNT 13

EXTERNVAR ED_B2_M1_EXPORT const unsigned TOOLBAR_MODEL_ELEMENTS_ID[TOOLBAR_MODEL_ELEMENTS_COUNT];


class CModelEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	friend class CModelState;
	OBJECT_NOCOPY_METHODS( CModelEditor );

	unsigned nModelToolbarID;
	IDockPanel *pwndTool;
	// The palette in pwndTool. Owned here, as the CModelWindow member it
	// replaces was; its window is destroyed in DestroyControls and the object
	// with it.
	std::unique_ptr<CWnd> pModelWindow;
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
	void GetTemporaryLabel( std::string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pModelState; }
	void GetChildFrameType( std::string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls();
	void ResetGUI( bool bActive ) override;
	void PostCreateControls();
	void PreDestroyControls();
	void DestroyControls();
	void Create();
	void Destroy();

	//CDefaultView
	void Undo( IController* pController ) {}
	void Redo( IController* pController ) {}
	
	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// Icons creations
	void CreateIcon();
	void RestoreCameraParametersFromStats();

	// Targeting for creating icons shots
	void CreateTarget();
	void DeleteTarget();
};


