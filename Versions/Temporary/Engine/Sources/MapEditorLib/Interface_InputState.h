#pragma once


#define INVALID_INPUT_STATE_INDEX (-1)

struct IInputState
{
	virtual void Enter() = 0;
	virtual void Leave() = 0;
	//
	virtual void Draw( class CPaintDC *pDC ) = 0;
	virtual void PostDraw( class CPaintDC *pDC ) = 0;
	//
	virtual void OnSetFocus				( class CWnd* pNewWnd ) = 0;
	virtual void OnKillFocus			( class CWnd* pOldWnd ) = 0;
	//
	virtual void OnMouseMove			( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual bool OnMouseWheel			( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnLButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnLButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnLButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnRButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnRButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnRButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnMButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnMButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnMButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnKeyDown				( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) = 0;
	virtual void OnKeyUp					( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) = 0;
	virtual void OnChar						( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) = 0;
	virtual void OnSysKeyDown			( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) = 0;
	virtual void OnSysKeyUp				( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) = 0;
	virtual void OnSysChar				( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) = 0;
	//
	virtual void OnContextMenu		( const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual ~IInputState() {}
};


