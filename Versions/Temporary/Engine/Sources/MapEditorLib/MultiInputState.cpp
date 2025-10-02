#include "stdafx.h"
#include "MultiInputState.h"

void CMultiInputState::Enter()
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->Enter();
}


void CMultiInputState::Leave()
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->Leave();
}


void CMultiInputState::Draw( CPaintDC *pPaintDC )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->Draw( pPaintDC );
}


void CMultiInputState::PostDraw( CPaintDC *pPaintDC )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->PostDraw( pPaintDC );
}



void CMultiInputState::OnSetFocus( CWnd* pNewWnd )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnSetFocus( pNewWnd );
}


void CMultiInputState::OnKillFocus( CWnd* pOldWnd )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnKillFocus( pOldWnd );
}


void CMultiInputState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMouseMove( nFlags, rMousePoint );
}


bool CMultiInputState::OnMouseWheel( UINT nFlags, short zDelta, CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		return inputStateList[nActiveInputState]->OnMouseWheel( nFlags, zDelta, rMousePoint );
	else
		return false;
}


void CMultiInputState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnLButtonDown( nFlags, rMousePoint );
}


void CMultiInputState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnLButtonUp( nFlags, rMousePoint );
}


void CMultiInputState::OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnLButtonDblClk( nFlags, rMousePoint );
}


void CMultiInputState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnRButtonDown( nFlags, rMousePoint );
}


void CMultiInputState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnRButtonUp( nFlags, rMousePoint );
}


void CMultiInputState::OnRButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnRButtonDblClk( nFlags, rMousePoint );
}


void CMultiInputState::OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMButtonDown( nFlags, rMousePoint );
}


void CMultiInputState::OnMButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMButtonUp( nFlags, rMousePoint );
}


void CMultiInputState::OnMButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMButtonDblClk( nFlags, rMousePoint );
}


void CMultiInputState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnKeyDown( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnKeyUp( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnChar( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnSysKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnSysKeyDown( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnSysKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnSysKeyUp( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnSysChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnSysChar( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnContextMenu( const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnContextMenu( rMousePoint );
}

// basement storage  


