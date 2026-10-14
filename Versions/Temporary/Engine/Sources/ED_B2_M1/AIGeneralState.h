#pragma once

#include "AIGeneralData.h"
#include "MapInfoEditor.h"

#include <cstdint>

//
//
//		AI GENERAL POINTS STATE
//
//

class CAIGeneralPointsState : public CDefaultInputState, public ICommandHandler
{
private:
	//
	CMapInfoEditor *pMapInfoEditor;
	SAIGeneralPointsWindowData dialogData; // editor window data
	CSceneDrawTool sceneDrawTool;

	bool bMoveParcel;
	bool bMovePoint;
	bool bRotateParcel;
	bool bRotatePoint;

	CMapInfoEditor *GetMapInfoEditor()
	{
		return pMapInfoEditor;
	}

public:
	CAIGeneralPointsState( CMapInfoEditor *pMapInfoEditor = 0 );

	~CAIGeneralPointsState() override {}

	// IInputStateInterface
	void Enter() override;
	void Leave() override;
	void OnSetFocus( IWidget *pNewWnd ) override;
	void Draw( IPaintContext *pPaintDC ) override;
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;

	void GetDataFromDB();
	void SaveDataToDB();
	void EditParcel();
	void AddParcel();
	void AddID();
	void DeleteParcel();
	void DeleteID();
	void AddPoint( const CTPoint<int> &rMousePoint );
	void DeletePoint();
	void RefreshWindowData();
};
