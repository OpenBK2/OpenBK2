#if !defined(__UNIT_START_CMD_STATE__)
#define __UNIT_START_CMD_STATE__
#pragma once

#include "MapInfoEditor.h"
#include "SimpleObjectState.h"
#include "EdUnitStartCmd.h"
#include "UnitStartCmdWindow.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//	UNIT START COMMANDS LIST
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SStartCommand
{
	int nCmdType;											// тип команды
	vector<int> unitLinkIDs;				// LinkID юнитов, для которых эта команда
	int nTgtLinkID;										// LinkID объекта назначения команды (допустим грузовика для посадки взводов)
	CVec2 vTgtPos;										// точка назначения для команды
	int nData;														// добавочный параметр (интерпретация зависит от типа команды)
	///
	SStartCommand();
	void Init();
	bool LoadFromDB( IManipulator *pManipulator, int nIndex );
	bool UpdateDB( IManipulator *pManipulator, int nIndex );
	///
};
struct SStartCommandList
{
	vector<SStartCommand> commands;
	///
	SStartCommandList();
	void Init();
	bool UpdateDB( IManipulator *pManipulator );
	bool LoadFromDB( IManipulator *pManipulator );
	void RemoveCommands( const vector<int> &rIndices );
};
enum EMoveDir
{
	MV_UP,
	MV_DOWN
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//	UNIT START COMMANDS STATE
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEdUnitStartCmd;
class CUnitStartCmdState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;

	hash_map<int,SUnitCommandTypeInfo> commandTypesMnemonic;
	//
	bool bEdCmdVisible;
	CPtr<CEdUnitStartCmd> pEdUnitStartCmd;
	//
	CMapInfoEditor *pMapInfoEditor;
	//
	CUnitStartCmdState( CMapInfoEditor* _pMapInfoEditor = 0 );
	//
	typedef int CMapObjID;
	vector<CMapObjID> currCmdUnits;
	CMapObjID nCurrTargetUnit;
	CVec3 vCurrTargetPos;
	void GetSelectedUnitIDs( vector<CMapObjID> *pIDs );
	//
	SStartCommandList commandsList;
	//
	void ResetStateData();
	//
	void RefreshDockingWindow( const vector<int> *pSelection );
	void FilterCommandsyBySelection();
	//
	bool GetLinkIDs( vector<int> *pLinkIDs  );
	//
	struct SCmdMarker
	{
		vector<CVec3> unitPositions;
    CVec3 vTargetPos;
		bool bTargetIsUnit; // false -- target is position
		bool bTargetSelected;
		//
		SCmdMarker() :
			vTargetPos( VNULL3 ),
			bTargetIsUnit( false ),
			bTargetSelected( false )
		{
		}
	};
	vector<SCmdMarker> markers;
	SCmdMarker currentCmdMarker;
	void DrawCommandMarkers();
	void DrawCommandMarker( const SCmdMarker &rMarker );
	void ClearCmdMarkers();
	void UpdateCmdMarkers();
	//
	string GetMapObjectName( SObjectInfo *pMOI );
	bool GetMapObjectElementIDs( int nObjectID, vector<int> *pRes );
	//
	const SUnitCommandTypeInfo* GetCurrentCommandType();

protected:
	// IInputState
	virtual void Enter();
	virtual void Leave();
	virtual void Draw( CPaintDC *pDC );
	
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnRButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );

	// CMapObjectState
	bool CanEdit() { return true; }
	bool CanInsertMapObject() { return false; }
	bool IsDrawSceneDrawTool() { return false; }
	//
	NMapInfoEditor::SObjectInfoCollector* GetObjectInfoCollector();
	CMapInfoEditor* GetMapInfoEditor();

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

public:
	void OnEdUnitStartCmdDialogEvent( CEdUnitStartCmd::EDlgEvents eEvt );
	//
	void UsrEvtAddCmd( const SUnitStartCmdWindowData &data );
	void UsrEvtDelCmd( const SUnitStartCmdWindowData &data );
	void UsrEvtEditCmd( const SUnitStartCmdWindowData &data );
	void UsrEvtMoveCmd( EMoveDir eDir, const SUnitStartCmdWindowData &data );
	void UsrEvtSelChange( const SUnitStartCmdWindowData &data );
	//
	void EdCmdOK();
	void EdCmdCancel();
	void EdCmdClear();
	void EdCmdTypeChange() {}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__UNIT_START_CMD_STATE__)
