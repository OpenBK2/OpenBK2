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
	std::vector<SReinfPoint> reinfPoints; // описание reinforcement-ов текущего выбранного игрока

	bool CreateReinfPoint();
	bool DeleteSelectedReinfPoint();
	bool GetReinfPointsFromWindow();
	bool SaveCurrentReinfPoint( const std::vector<SReinfPoint> &rReinfPoints, int nPlayerIndex, int nSelectedReinfPoint );

public:
	CReinfPointsState( CMapInfoEditor *_pMapInfoEditor = 0 );

	~CReinfPointsState() override {}

	//
	void Enter() override;
	void Leave() override;
	void OnSetFocus( IWidget *pNewWnd ) override;
	void Draw( IPaintContext *pPaintDC ) override;
	void PostDraw( IPaintContext *pPaintDC ) override;

	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;

	void RefreshReinfPointsWindow();
	bool EditPointDeployTemplate();
	bool EditPointTypedTemplate();
};
