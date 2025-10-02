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
	virtual void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual bool OnMouseWheel			( UINT nFlags, short zDelta, CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnRButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnRButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnMButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnMButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	virtual void OnMButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags ) = 0;
	virtual void OnKeyUp					( UINT nChar, UINT nRepCnt, UINT nFlags ) = 0;
	virtual void OnChar						( UINT nChar, UINT nRepCnt, UINT nFlags ) = 0;
	virtual void OnSysKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags ) = 0;
	virtual void OnSysKeyUp				( UINT nChar, UINT nRepCnt, UINT nFlags ) = 0;
	virtual void OnSysChar				( UINT nChar, UINT nRepCnt, UINT nFlags ) = 0;
	//
	virtual void OnContextMenu		( const CTPoint<int> &rMousePoint ) = 0;
	//
	virtual ~IInputState() {}
};


