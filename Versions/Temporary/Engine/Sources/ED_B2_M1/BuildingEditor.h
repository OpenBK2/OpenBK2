#pragma once

#define EDITOR_BUILDING_ID (-4)

#include "B2_M1_World/MOBuilding.h"
#include "MapEditorLib/DefaultShortcutBar.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/EditorBase.h"

#include <cstdint>

class CBuildingRPGStatsEditorSettings
{
public:
	bool bShowShortcutBar;
	
	CBuildingRPGStatsEditorSettings()
		: bShowShortcutBar( true )
	{}

	// serializing...
	int operator&( IXmlSaver &xs );
};

class CBuildingEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	friend class CBuildingState;
	OBJECT_NOCOPY_METHODS( CBuildingEditor );
	//	
	CBuildingState *pBuildingState;
	CBuildingRPGStatsEditorSettings editorSettings;
	CVec3 vLastCameraAnchor;
	string szLastTerrainName;
	string szCurrSeason;
	//
	SECControlBar *pwndShortcutBar;
	CDefaultShortcutBar wndShortcutBar;
	//
	CTPoint<int> terrainSize;
	CObj<CMOBuilding> pBuilding;
	//
	CVec3 vBuildingPos;
	CVec2 vBuildingOrigin;
	//
	bool bDrawPassability;
	CArray2D<uint8_t> modelPassability;
	//
	string szScreenTitle;
	//
	bool IsModelValid() const { return pBuilding != 0; }
	//
	CBuildingEditor();
	~CBuildingEditor();
	//	
public:
	//IEditor
	CObj<CMOBuilding> &GetBuildingPtr() { return pBuilding; };
	const string &GetCurrSeason() const;

	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return reinterpret_cast<IInputState*>(pBuildingState); }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls();
	void PostCreateControls() {}
	void PreDestroyControls() {}
	void DestroyControls();
	void Create();
	void Destroy();

	//CDefaultView
	void Undo( IController* pController );
	void Redo( IController* pController );

	//ICommandHandler
	bool HandleCommand( UINT nCommandID, uint32_t dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	//CBuildingEditor
	void ReloadModel( NDb::ESeason eSeason );
	void ReloadTerrain( const string &rszMapInfoName, const CVec3 &vLastCameraAnchor );
	//
	void DrawPassability( class CPaintDC *pPaintDC );
	void SetDrawPassability( bool bSet ) { bDrawPassability = bSet; }
	//
	void SetScreenTitle( const string &rszScreenTitle );
	//
	void SetBuildingOrigin( const CVec2 &rvOrigin );
	CVec2 GetBuildingOrigin();
	CVec3 GetBuildingPos();
	IManipulator* CreateBuildingManipulator();
	CVec3 GetLastCameraAnchor() { return vLastCameraAnchor; }
	//
	void ChangeSeason( const NDb::ESeason eSeason );
};


