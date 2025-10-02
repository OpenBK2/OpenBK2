#pragma once

#include "MapInfoEditorSettings.h"
#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/DefaultView.h"

#include "MapInfoController.h"
#include "MapInfoState.h"
#include "HeightContainer.h"
#include "SpotInfoData.h"

#include "MapEditorLib/DefaultShortcutBar.h"
#include "MiniMapWindow.h"
#include "MoviesEditorWindow.h"


#define TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT 11
#define TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT 2

EXTERNVAR const UINT TOOLBAR_MAPINFO_TOOLS_ELEMENTS_ID[TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT];
EXTERNVAR const UINT TOOLBAR_MAPINFO_VIEW_ELEMENTS_ID[TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT];

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
	friend class CAdvClipboardState;

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
	//
	inline void ClearMapInfoData()
	{
		objectInfoCollector.Clear();
		pMapInfo = 0;
	}
	// другие переменные
	UINT nMapInfoToolsToolbarID;
	UINT nMapInfoViewToolbarID;
	SECControlBar *pwndMiniMap; // MiniMap docking window
	SECControlBar *pwndShortcutBar;
	SECControlBar *pwndMoviesEditor;	// Script MovieEditor docking window
	CMiniMapWindow wndMiniMap;
	CMoviesEditorWindow wndMoviesEditor;
	CDefaultShortcutBar wndShortcutBar;
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
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pMapInfoState; }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	//IEditor
	void CreateControls();
	void PostCreateControls();
	void PreDestroyControls();
	void DestroyControls();
	void Create();
	void Destroy();
	void Save( bool bSaveChanges );
	bool ShowProgress() { return false; }

	void GetChangesFromController( CObjectBaseController *pObjectController, bool bRedo );

	// Создание Undo Operation
	virtual CMapInfoController* CreateController()
	{ 
		CMapInfoController* pMapInfoController = CDefaultView::CreateController<CMapInfoController>( static_cast<CMapInfoController*>( 0 ) );
		if ( pMapInfoController )
		{
			pMapInfoController->SetTemporaryLabel( CMapInfoController::TEMPORARY_LABEL );
		}
		return pMapInfoController;
	}
	//CDefaultView
	void Undo( IController* pController );
	void Redo( IController* pController );

	//ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	
	////////////////////////////
	bool CreateMinimapImage();
	bool GenerateMinimapImage( const string &szFileName );
	//
};


