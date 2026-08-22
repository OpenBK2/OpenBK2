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

	void BeginDrag( unsigned nFlags );
	void ContinueDrag( const CTPoint<int> &rMousePoint, unsigned nFlags );
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
	void OnMouseMove			( unsigned nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnLButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp			( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk	( unsigned nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnRButtonDown		( unsigned nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnKeyDown				( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	void OnKeyUp					( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	//
	void OnContextMenu		( const CTPoint<int> &rMousePoint );
};


