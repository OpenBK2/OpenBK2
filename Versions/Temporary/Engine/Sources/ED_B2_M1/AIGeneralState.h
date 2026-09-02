#pragma once

#include "AIGeneralWindow.h"

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
	SAIGeneralPointsWindowData dialogData;	// editor window data
	CSceneDrawTool sceneDrawTool;

	bool bMoveParcel;
	bool bMovePoint;
	bool bRotateParcel;
	bool bRotatePoint;

	CMapInfoEditor* GetMapInfoEditor() { return pMapInfoEditor; }

public:
	CAIGeneralPointsState( CMapInfoEditor *pMapInfoEditor = 0 );
	virtual ~CAIGeneralPointsState() {}

	// IInputStateInterface
	virtual void Enter();
	virtual void Leave();
	virtual void OnSetFocus( class CWnd* pNewWnd );
	virtual void Draw( CPaintDC *pPaintDC );
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );

	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

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


