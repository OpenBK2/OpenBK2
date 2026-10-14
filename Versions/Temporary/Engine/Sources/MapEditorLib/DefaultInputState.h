#pragma once

#include "Interface_InputState.h"

class CDefaultInputState : public IInputState
{
public:
	//IInputState interface
	void Enter() override {}

	void Leave() override {}

	//
	void Draw( IPaintContext *pPaintDC ) override {}

	void PostDraw( IPaintContext *pPaintDC ) override {}

	//
	void OnSetFocus( IWidget *pNewWnd ) override {}

	void OnKillFocus( IWidget *pOldWnd ) override {}

	//
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	bool OnMouseWheel( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint ) override
	{
		return true;
	}

	//
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	void OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	//
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	void OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	//
	void OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	void OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	void OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override {}

	//
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override {}

	void OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override {}

	//
	void OnContextMenu( const CTPoint<int> &rMousePoint ) override {}
};
