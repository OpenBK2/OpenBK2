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


void CMultiInputState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMouseMove( nFlags, rMousePoint );
}


bool CMultiInputState::OnMouseWheel( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		return inputStateList[nActiveInputState]->OnMouseWheel( nFlags, zDelta, rMousePoint );
	else
		return false;
}


void CMultiInputState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnLButtonDown( nFlags, rMousePoint );
}


void CMultiInputState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnLButtonUp( nFlags, rMousePoint );
}


void CMultiInputState::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnLButtonDblClk( nFlags, rMousePoint );
}


void CMultiInputState::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnRButtonDown( nFlags, rMousePoint );
}


void CMultiInputState::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnRButtonUp( nFlags, rMousePoint );
}


void CMultiInputState::OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnRButtonDblClk( nFlags, rMousePoint );
}


void CMultiInputState::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMButtonDown( nFlags, rMousePoint );
}


void CMultiInputState::OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMButtonUp( nFlags, rMousePoint );
}


void CMultiInputState::OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnMButtonDblClk( nFlags, rMousePoint );
}


void CMultiInputState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnKeyDown( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnKeyUp( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnChar( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnSysKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnSysKeyDown( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnSysKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		inputStateList[nActiveInputState]->OnSysKeyUp( nChar, nRepCnt, nFlags );
}


void CMultiInputState::OnSysChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
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


