#pragma once

#include "MapEditorLib/DefaultInputState.h"
#include "Misc/PlaneGeometry.h"
#include "Tools_SceneDraw.h"

// A temporary path for the clipboard; it never changes the map or its undo history.
class CScriptPathPointsState : public CDefaultInputState
{
	std::vector<CVec3> points;
	CSceneDrawTool sceneDrawTool;

	void UpdatePath();
	bool CopyPathToClipboard() const;

public:
	void Enter() override;
	void Leave() override;
	void Draw( IPaintContext *pPaintDC ) override;

	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
};
