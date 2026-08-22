#pragma once

#include "PolygonState.h"
#include "MapInfoEditor.h"
#include "ScriptAreaWindow.h"
#include "MapEditorLib/Tools_IndexCollector.h"
#include "MapEditorLib/Tools_FreeIDCollector.h"

#include <cstdint>

//
//
//	SCRIPT AREA STATE
//
//

class CScriptAreaState : public CPolygonState
{
public:
	struct SScriptArea
	{
		NDb::EScriptAreaTypes eType;
		int nScriptAreaID;
		string szName;
		CPolygonState::CControlPointList cpList; // (2шт. макс.)
		bool bSelected;
		//
		enum { CLR_NORMAL_AREA = 0x0000FF00, CLR_SELECTED_AREA = 0xFFFFFF00 };
		//
		SScriptArea();
		//
		float GetRadius() const;
		CVec2 GetAABBHalfSize() const;
		CVec2 GetCenter() const;
		void Draw( CPaintDC *pPaintDC, CSceneDrawTool *pSceneDrawTool ) const;
		bool Pick( const CVec3 &rPoint ) const;
	};
	typedef hash_map<unsigned, SScriptArea> CScriptAreaMap;
	//
private:
	CMapInfoEditor *pMapInfoEditor;
	//
	CScriptAreaMap scriptAreaMap;
	CIndexCollector<unsigned> scriptAreaIDToIndexCollector;
	CFreeIDCollector scriptAreaIDCollector;
	//
	SScriptAreaWindowData dialogData;			// данные окна редактора
	bool bShift;													// для мультиселекта областей
	//
	bool SetScriptAreaWindowData( SScriptAreaWindowData::EChangeMask eChangeMask );
	bool ProcessScriptAreaWindowData();
	//
	bool IsPointInArea( SScriptArea *pA, const CVec3 &vPos );
	void ClearSelection();

	void Clear();
	void GetScriptAreaMap();
	void UpdateScriptArea( unsigned nScriptAreaID );
	unsigned InsertScriptArea( NDb::EScriptAreaTypes eType, const string &rszName, const CVec3 &rStart, const CVec3 &rFinish );
	void RemoveScriptArea( unsigned nScriptAreaID );

public:
	CScriptAreaState( CMapInfoEditor *pMapInfoEditor = 0 );
	virtual ~CScriptAreaState() {}

	// IInputStateInterface
	void Enter();
	void Leave();
	void Draw( CPaintDC *pPaintDC );
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );

	// CPolygonState
	virtual bool SkipEnterAfterInsert() { return true; }
	virtual bool CanEdit() { return true; }
	virtual bool CanInsertPolygon() { return true; }
	virtual bool IsClosedPolygon() { return false; }
	virtual bool IsDrawSceneDrawTool() { return false; }
	virtual EMoveType GetMoveType();
	virtual void GetBounds( int *pnMinCount, int *pnMaxCount );
	virtual CControlPointList* GetControlPoints( int nPolygonID );
	virtual bool PrepareControlPoints( CControlPointList *pControlPointList );
	virtual void PickPolygon( const CVec3 &rvPos, CPolygonIDList *pPickPolygonIDList );
	virtual void UpdatePolygon( int nPolygonID, EUpdateType eEpdateType );
	virtual unsigned InsertPolygon( const CControlPointList &rControlPointList );
	virtual void RemovePolygon( int nPolygonID );

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	//
	void PostDraw( CPaintDC *pPaintDC );
};


