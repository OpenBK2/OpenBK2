#if !defined( __AIGENERAL_STATE__ )
#define __AIGENERAL_STATE__
#pragma once

#include "AIGeneralWindow.h"

#include "MapInfoEditor.h"


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
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

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

#endif // #if !defined( __AIGENERAL_STATE__ )
