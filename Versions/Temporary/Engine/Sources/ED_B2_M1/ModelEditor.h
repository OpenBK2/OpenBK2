#pragma once

#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"

#include "ED_B2_M1_export.h"
// ModelView.h rather than ModelWindow.h: which toolkit draws the palette is
// NModelView's business. ModelState.h was reached through ModelWindow.h and is
// needed here in its own right, for GetInputState.
#include "ModelEditorSettings.h"
#include "ModelState.h"
#include "ModelView.h"

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
	std::unique_ptr<NModelView::IView> pModelView;
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
		return pModelState;
	}

	void GetChildFrameType( std::string *pszChildFrameTypeName ) override
	{
		( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__";
	}

	void CreateControls() override;
	void ResetGUI( bool bActive ) override;
	void PostCreateControls() override;
	void PreDestroyControls() override;
	void DestroyControls() override;
	void Create() override;
	void Destroy() override;

	//CDefaultView
	void Undo( IController *pController ) override {}

	void Redo( IController *pController ) override {}

	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;

	// Icons creations
	void CreateIcon();
	void RestoreCameraParametersFromStats();

	// Targeting for creating icons shots
	void CreateTarget();
	void DeleteTarget();
};
