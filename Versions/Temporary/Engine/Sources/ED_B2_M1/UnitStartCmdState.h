#pragma once

#include "EditorMethods.h"
#include "MapInfoEditor.h"
#include "SimpleObjectState.h"
#include "UnitStartCmdDialog.h"
#include "UnitStartCmdWindow.h"

#include <cstdint>

//
//
//	UNIT START COMMANDS LIST
//
//

struct SStartCommand
{
	int nCmdType;											// тип команды
	std::vector<int> unitLinkIDs;				// LinkID юнитов, для которых эта команда
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
	std::vector<SStartCommand> commands;
	///
	SStartCommandList();
	void Init();
	bool UpdateDB( IManipulator *pManipulator );
	bool LoadFromDB( IManipulator *pManipulator );
	void RemoveCommands( const std::vector<int> &rIndices );
};
enum EMoveDir
{
	MV_UP,
	MV_DOWN
};

//
//
//	UNIT START COMMANDS STATE
//
//

// The command editor is modeless and owned here for the life of the tab; it is
// whichever toolkit's the session is running. See UnitStartCmdDialog.h.
class CUnitStartCmdState : public CMapObjectState, public NUnitStartCmdDialog::IListener
{
	friend class CMultiInputState;
	friend class CMapInfoState;

	std::unordered_map<int,SUnitCommandTypeInfo> commandTypesMnemonic;
	//
	bool bEdCmdVisible;
	NUnitStartCmdDialog::IDialog *pEdUnitStartCmd;
	//
	CMapInfoEditor *pMapInfoEditor;
	//
	CUnitStartCmdState( CMapInfoEditor* _pMapInfoEditor = 0 );
	// The dialog is owned here, plainly: it is made on the first Enter and
	// destroyed with the state, which destroys its window.
	virtual ~CUnitStartCmdState();
	//
	typedef int CMapObjID;
	std::vector<CMapObjID> currCmdUnits;
	CMapObjID nCurrTargetUnit;
	CVec3 vCurrTargetPos;
	void GetSelectedUnitIDs( std::vector<CMapObjID> *pIDs );
	//
	SStartCommandList commandsList;
	//
	void ResetStateData();
	//
	void RefreshDockingWindow( const std::vector<int> *pSelection );
	void FilterCommandsyBySelection();
	//
	bool GetLinkIDs( std::vector<int> *pLinkIDs  );
	//
	struct SCmdMarker
	{
		std::vector<CVec3> unitPositions;
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
	std::vector<SCmdMarker> markers;
	SCmdMarker currentCmdMarker;
	void DrawCommandMarkers();
	void DrawCommandMarker( const SCmdMarker &rMarker );
	void ClearCmdMarkers();
	void UpdateCmdMarkers();
	//
	std::string GetMapObjectName( SObjectInfo *pMOI );
	bool GetMapObjectElementIDs( int nObjectID, std::vector<int> *pRes );
	//
	const SUnitCommandTypeInfo* GetCurrentCommandType();

protected:
	// IInputState
	virtual void Enter();
	virtual void Leave();
	virtual void Draw( IPaintContext *pDC );
	
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint );

	// CMapObjectState
	bool CanEdit() { return true; }
	bool CanInsertMapObject() { return false; }
	bool IsDrawSceneDrawTool() { return false; }
	//
	NMapInfoEditor::SObjectInfoCollector* GetObjectInfoCollector();
	CMapInfoEditor* GetMapInfoEditor();

	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

public:
	// NUnitStartCmdDialog::IListener
	virtual void OnUnitStartCmdDialogEvent( NUnitStartCmdDialog::EEvent eEvent );
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


