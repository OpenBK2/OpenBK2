#pragma once

#include "MapInfoEditor.h"

//
//		REINF POINTS STATE
//

class CReinfPointsState : public CDefaultInputState, public ICommandHandler
{
	CMapInfoEditor *pMapInfoEditor;
	//
	int nSelectedPlayer;
  int nSelectedReinfPoint;
	CSceneDrawTool sceneDrawTool;
	bool bIsAvia;
	bool bMove;
	bool bRotate;

public:
	struct SRSDeployTemplate : public NDb::SDeployTemplate
	{
	};
	//
	struct STypedTemplate
	{
		string szTemplateType;
		string szTemplate;
	};
	//
	typedef vector<STypedTemplate> CTypedTemplateType;
	//
	struct SReinfPoint : public NDb::SReinforcementPosition
	{
		vector<STypedTemplate> typedTemplates;
		string szDeployTemplate;
	};

private:
	vector<SReinfPoint> reinfPoints;						// описание reinforcement-ов текущего выбранного игрока

	bool CreateReinfPoint();
	bool DeleteSelectedReinfPoint();
	bool GetReinfPointsFromWindow();
	bool SaveCurrentReinfPoint( const vector<SReinfPoint> &rReinfPoints, int nPlayerIndex, int nSelectedReinfPoint );

public:
	CReinfPointsState( CMapInfoEditor* _pMapInfoEditor = 0 );
	virtual ~CReinfPointsState() {}
	//
	void Enter();
	void Leave();
	virtual void OnSetFocus( class CWnd* pNewWnd );
	void Draw( CPaintDC *pPaintDC );
	void PostDraw( CPaintDC *pPaintDC );

	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint );

	void RefreshReinfPointsWindow();
	bool EditPointDeployTemplate();
	bool EditPointTypedTemplate();
};

