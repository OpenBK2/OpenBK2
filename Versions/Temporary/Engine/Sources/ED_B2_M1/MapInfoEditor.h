#pragma once

#include "HeightContainer.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/PaletteList.h"
#include "MapInfoController.h"
#include "MapInfoEditorSettings.h"
#include "MapInfoState.h"
#include "MiniMapView.h"
#include "MoviesEditorView.h"
#include "ShortcutBarView.h"
#include "SpotInfoData.h"

#include "ED_B2_M1_export.h"

#include <cstdint>
#include <memory>

#define TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT 11
#define TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT 2

EXTERNVAR ED_B2_M1_EXPORT const unsigned TOOLBAR_MAPINFO_TOOLS_ELEMENTS_ID[TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT];
EXTERNVAR ED_B2_M1_EXPORT const unsigned TOOLBAR_MAPINFO_VIEW_ELEMENTS_ID[TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT];

using namespace NMapInfoEditor;

class CMapInfoEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	friend class CVSOState;
	friend class CMapObjectState;
	friend class CPolygonState;
	//
	friend class CMapInfoState;
	friend class CObjectState;
	friend class CScriptAreaState;
	friend class CCameraPositionState;
	friend class CUnitStartCmdState;

	friend class CMapObjectMultiState;
	friend class CMapObjectPasteState;
	friend class CSimpleObjectState;
	friend class CSpotState;
	friend class CBridgeState;
	friend class CEntrenchmentState;
	friend class CFenceState;

	friend class CTileState;
	friend class CHeightState;
	friend class CHeightStateV2;
	friend class CHeightStateV3;
	friend class CFieldState;

	friend class CVSOMultiState;
	friend class CVSOStateEx;
	friend class CRoadState;
	friend class CRiverState;
	friend class CCragState;
	friend class CLakeState;

	friend class CCoastState;

	friend class CReinfPointsState;
	friend class CScriptCameraState;
	friend class CAIGeneralPointsState;

	friend struct NMapInfoEditor::SObjectInfoCollector;
	friend struct NMapInfoEditor::SSpotInfo;

	OBJECT_NOCOPY_METHODS( CMapInfoEditor );

	CMapInfoEditorSettings editorSettings;

	SObjectInfoCollector objectInfoCollector;
	SVSOCollector VSOCollector;
	CHeightContainer heightContainer;
	const NDb::SMapInfo *pMapInfo;
	bool bSaveFailed = false;
	std::string szSaveError;

	//
	inline void ClearMapInfoData()
	{
		objectInfoCollector.Clear();
		pMapInfo = 0;
	}

	// другие переменные
	unsigned nMapInfoToolsToolbarID;
	unsigned nMapInfoViewToolbarID;
	IDockPanel *pwndMiniMap; // MiniMap docking window
	IDockPanel *pwndShortcutBar;
	IDockPanel *pwndMoviesEditor; // Script MovieEditor docking window
	// The panes are the frame's; their contents are whichever toolkit's this
	// session runs, and are owned here. See MiniMapView.h and
	// MoviesEditorView.h. Null until CreateControls has made them.
	NMiniMapView::IView *pMiniMapView;
	NMoviesEditorView::IView *pMoviesEditorView;
	// The shortcut bar in the pane, in whichever toolkit this session draws it;
	// see ShortcutBarView.h.
	std::unique_ptr<NShortcutBar::IView> pShortcutBarView;
	CObj<IEditorScene> pEditorScene;
	// Данные общего назначения
	CMapInfoState *pMapInfoState;
	//
	CMapInfoEditor();
	//
	//
	void ConfigureViewFilter();
	void ApplyViewFilter();
	//
	//
	void RunGame();
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
		return pMapInfoState;
	}

	void GetChildFrameType( std::string *pszChildFrameTypeName ) override
	{
		( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__";
	}

	//IEditor
	void CreateControls() override;
	void ResetGUI( bool bActive ) override;
	void PostCreateControls() override;
	void PreDestroyControls() override;
	void DestroyControls() override;
	void Create() override;
	void Destroy() override;
	void Save( bool bSaveChanges ) override;

	std::string GetSaveError() const override
	{
		return szSaveError;
	}

	bool ShowProgress() override
	{
		return false;
	}

	void GetChangesFromController( CObjectBaseController *pObjectController, bool bRedo );

	// Создание Undo Operation
	virtual CMapInfoController *CreateController()
	{
		CMapInfoController *pMapInfoController =
				CDefaultView::CreateController<CMapInfoController>( static_cast<CMapInfoController *>( 0 ) );
		if ( pMapInfoController )
		{
			pMapInfoController->SetTemporaryLabel( CMapInfoController::TEMPORARY_LABEL );
		}
		return pMapInfoController;
	}

	//CDefaultView
	void Undo( IController *pController ) override;
	void Redo( IController *pController ) override;

	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;

	////////////////////////////
	bool CreateMinimapImage();
	bool GenerateMinimapImage( const std::string &szFileName );
	//
};
