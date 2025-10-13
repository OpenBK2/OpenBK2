#pragma once

#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/DefaultShortcutBar.h"
#include "MarkerSet.h"
#include "SquadState.h"

#include <cstdint>

//
//
//			SQUAD EDITOR SETTINGS
//
//

class CSquadEditorSettings
{
public:
	bool bShowShortcutBar;
	CVec3 vLastBECameraAnchor;
	
	CSquadEditorSettings() :
		bShowShortcutBar( true ),
		vLastBECameraAnchor( VNULL3 )
	{
	}
	
	// serializing...
	int operator&( IXmlSaver &xs )
	{
		xs.Add( "ShowShortcutBar", &bShowShortcutBar );
		xs.Add( "LastBECameraAnchor", &vLastBECameraAnchor );
		return 0;
	}
};

//
//
//									SQUAD MEMBER INFO
//
//

struct SSquadMemberInfo
{
	int nSceneObjectID;
	int nMemberIndex;
	CVec3 pos;
	//
	SSquadMemberInfo() :
		nSceneObjectID(-1),
		nMemberIndex(-1),
		pos( VNULL3 )
	{
	}
	bool operator==( const SSquadMemberInfo &rOther )
	{
		if ( nSceneObjectID == rOther.nSceneObjectID )
			return true;
		return false;
	}
};

//
//
//					SQUAD EDITOR
//
//

class CSquadEditor : public CEditorBase, public CDefaultView, ICommandHandler
{
	friend class CSquadState;
	OBJECT_NOCOPY_METHODS( CSquadEditor );
	
	// common data memebers
	CSquadState *pSquadState;
	//
	SECControlBar *pwndShortcutBar;
	CDefaultShortcutBar wndShortcutBar;
	//

	// specific data memebers
	CSquadEditorSettings editorSettings;
	//
	list<SSquadMemberInfo> membersInfo;
	CVec3 vSquadCenterPos;
	//
	CPtr<SMarkerSet> pMarkers;
	
	//
	CSquadEditor();
	~CSquadEditor();
	//
public:
	//IEditor
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this; }
	IInputState* GetInputState() { return pSquadState; }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls();
	void PostCreateControls() {}
	void PreDestroyControls() {}
	void DestroyControls();
	void Create();
	void Destroy();
	//

	//CDefaultView
	void Undo( IController* pController ) {}
	void Redo( IController* pController ) {}
	//

	//ICommandHandler
	bool HandleCommand( UINT nCommandID, uint32_t dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	//

	//CSquadEditor
	void ReloadTerrain();
	IManipulator* CreateSquadManipulator();
	//
	int AddModel( const NDb::SModel *pModel, const CVec3 &rvPosition, int nMemberIndex );
	bool RemoveModel( int nID );
	void RemoveAllModels();
	CVec3 GetModelPosition( int nID );
	bool SetModelPosition( int nID, const CVec3 &rvPos );
	int GetModelMemberIndex( int nID );
	//
	CVec3 GetSquadCenterPos();
	//
	void HideAxis();
	void ShowAxis();
	//
	int GetMemberIndexBySceneID( int nSceneID );
	//
};


