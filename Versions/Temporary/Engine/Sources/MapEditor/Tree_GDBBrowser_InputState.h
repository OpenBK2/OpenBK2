#pragma once

#include "MapEditorLib/DefaultInputState.h"

class CTreeGDBBrowserBase;
class CTreeGDBBrowserInputState : public CDefaultInputState
{
private:
	CTreeGDBBrowserBase *pTargetWindow;
	//
	bool bEnabled;
	bool bLeave;
	bool bCopy;
	HTREEITEM sourceItem;
	HTREEITEM targetItem;
	CTPoint<int> sourcePoint;

	HCURSOR hDefaultCursor;
	HCURSOR hMoveCursor;
	HCURSOR hCopyCursor;
	HCURSOR hErrorCursor;

	void BeginDrag( UINT nFlags );
	void ContinueDrag( const CTPoint<int> &rMousePoint, UINT nFlags );
	void EndDrag( bool bSuccess );

public:
	CTreeGDBBrowserInputState();
	virtual ~CTreeGDBBrowserInputState() {}
	//
	void SetTargetWindow(  CTreeGDBBrowserBase *_pTargetWindow );

	// IInputState
	void Enter();
	void Leave();
	//
	void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnKeyUp					( UINT nChar, UINT nRepCnt, UINT nFlags );
	//
	void OnContextMenu		( const CTPoint<int> &rMousePoint );
};


