#pragma once

#include "MapInfoEditor.h"

#include <cstdint>

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
		std::string szTemplateType;
		std::string szTemplate;
	};
	//
	typedef std::vector<STypedTemplate> CTypedTemplateType;
	//
	struct SReinfPoint : public NDb::SReinforcementPosition
	{
		std::vector<STypedTemplate> typedTemplates;
		std::string szDeployTemplate;
	};

private:
	std::vector<SReinfPoint> reinfPoints;						// описание reinforcement-ов текущего выбранного игрока

	bool CreateReinfPoint();
	bool DeleteSelectedReinfPoint();
	bool GetReinfPointsFromWindow();
	bool SaveCurrentReinfPoint( const std::vector<SReinfPoint> &rReinfPoints, int nPlayerIndex, int nSelectedReinfPoint );

public:
	CReinfPointsState( CMapInfoEditor* _pMapInfoEditor = 0 );
	virtual ~CReinfPointsState() {}
	//
	void Enter();
	void Leave();
	virtual void OnSetFocus( class CWnd* pNewWnd );
	void Draw( CPaintDC *pPaintDC );
	void PostDraw( CPaintDC *pPaintDC );

	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );

	void RefreshReinfPointsWindow();
	bool EditPointDeployTemplate();
	bool EditPointTypedTemplate();
};


