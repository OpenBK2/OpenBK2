#pragma once

#include "MapInfoEditor.h"
#include "PolygonState.h"
// SScriptAreaWindowData, which is all this needed from the palette. It used to
// include ScriptAreaWindow.h and take an MFC dialog class with it.
#include "MapEditorLib/Tools_FreeIDCollector.h"
#include "MapEditorLib/Tools_IndexCollector.h"
#include "ScriptAreaView.h"

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
		std::string szName;
		CPolygonState::CControlPointList cpList; // (2шт. макс.)
		bool bSelected;

		//
		enum
		{
			CLR_NORMAL_AREA = 0x0000FF00,
			CLR_SELECTED_AREA = 0xFFFFFF00
		};

		//
		SScriptArea();
		//
		float GetRadius() const;
		CVec2 GetAABBHalfSize() const;
		CVec2 GetCenter() const;
		void Draw( IPaintContext *pPaintDC, CSceneDrawTool *pSceneDrawTool ) const;
		bool Pick( const CVec3 &rPoint ) const;
	};

	typedef std::unordered_map<unsigned, SScriptArea> CScriptAreaMap;
	//
private:
	CMapInfoEditor *pMapInfoEditor;
	//
	CScriptAreaMap scriptAreaMap;
	CIndexCollector<unsigned> scriptAreaIDToIndexCollector;
	CFreeIDCollector scriptAreaIDCollector;
	//
	SScriptAreaWindowData dialogData; // данные окна редактора
	bool bShift;                      // для мультиселекта областей
	//
	bool SetScriptAreaWindowData( SScriptAreaWindowData::EChangeMask eChangeMask );
	bool ProcessScriptAreaWindowData();
	//
	bool IsPointInArea( SScriptArea *pA, const CVec3 &vPos );
	void ClearSelection();

	void Clear();
	void GetScriptAreaMap();
	void UpdateScriptArea( unsigned nScriptAreaID );
	unsigned InsertScriptArea( NDb::EScriptAreaTypes eType,
	                           const std::string &rszName,
	                           const CVec3 &rStart,
	                           const CVec3 &rFinish );
	void RemoveScriptArea( unsigned nScriptAreaID );

public:
	CScriptAreaState( CMapInfoEditor *pMapInfoEditor = 0 );

	~CScriptAreaState() override {}

	// IInputStateInterface
	void Enter() override;
	void Leave() override;
	void Draw( IPaintContext *pPaintDC ) override;
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;

	// CPolygonState
	bool SkipEnterAfterInsert() override
	{
		return true;
	}

	bool CanEdit() override
	{
		return true;
	}

	bool CanInsertPolygon() override
	{
		return true;
	}

	bool IsClosedPolygon() override
	{
		return false;
	}

	bool IsDrawSceneDrawTool() override
	{
		return false;
	}

	EMoveType GetMoveType() override;
	void GetBounds( int *pnMinCount, int *pnMaxCount ) override;
	CControlPointList *GetControlPoints( int nPolygonID ) override;
	bool PrepareControlPoints( CControlPointList *pControlPointList ) override;
	void PickPolygon( const CVec3 &rvPos, CPolygonIDList *pPickPolygonIDList ) override;
	void UpdatePolygon( int nPolygonID, EUpdateType eEpdateType ) override;
	unsigned InsertPolygon( const CControlPointList &rControlPointList ) override;
	void RemovePolygon( int nPolygonID ) override;

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;
	//
	void PostDraw( IPaintContext *pPaintDC ) override;
};
