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
	virtual void OnMouseMove			( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual bool OnMouseWheel			( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint ) { return true; }
	//
	virtual void OnLButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnLButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnLButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	//
	virtual void OnRButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnRButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnRButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	//
	virtual void OnMButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnMButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	virtual void OnMButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint ) {}
	//
	virtual void OnKeyDown				( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {}
	virtual void OnKeyUp					( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {}
	virtual void OnChar						( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {}
	virtual void OnSysKeyDown			( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {}
	virtual void OnSysKeyUp				( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {}
	virtual void OnSysChar				( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {}
	//
	virtual void OnContextMenu( const CTPoint<int> &rMousePoint ) {}
};


