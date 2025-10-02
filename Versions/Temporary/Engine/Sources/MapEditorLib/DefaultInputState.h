#pragma once

#include "Interface_InputState.h"

class CDefaultInputState : public IInputState
{
public:
	//IInputState interface
	virtual void Enter() {}
	virtual void Leave() {}
	//
	virtual void Draw( class CPaintDC *pPaintDC ) {}
	virtual void PostDraw( class CPaintDC *pPaintDC ) {}
	//
	virtual void OnSetFocus				( class CWnd* pNewWnd ) {}
	virtual void OnKillFocus			( class CWnd* pOldWnd ) {}
	//
	virtual void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual bool OnMouseWheel			( UINT nFlags, short zDelta, CTPoint<int> &rMousePoint ) { return true; }
	//
	virtual void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	//
	virtual void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnRButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnRButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	//
	virtual void OnMButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnMButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnMButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint ) {}
	//
	virtual void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags ) {}
	virtual void OnKeyUp					( UINT nChar, UINT nRepCnt, UINT nFlags ) {}
	virtual void OnChar						( UINT nChar, UINT nRepCnt, UINT nFlags ) {}
	virtual void OnSysKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags ) {}
	virtual void OnSysKeyUp				( UINT nChar, UINT nRepCnt, UINT nFlags ) {}
	virtual void OnSysChar				( UINT nChar, UINT nRepCnt, UINT nFlags ) {}
	//
	virtual void OnContextMenu( const CTPoint<int> &rMousePoint ) {}
};


