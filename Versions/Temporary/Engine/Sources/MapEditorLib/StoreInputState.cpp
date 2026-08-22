#include "stdafx.h"
#include "StoreInputState.h"

void CStoreInputState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_MOUSEMOVE;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


bool CStoreInputState::OnMouseWheel( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_MOUSEWEEL;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.zDelta = zDelta;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
	return true;
}


void CStoreInputState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_LBUTTONDOWN;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_LBUTTONUP;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_LBUTTONDBLCLK;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_RBUTTONDOWN;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_RBUTTONUP;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_RBUTTONDBLCLK;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_MBUTTONDOWN;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_MBUTTONUP;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_MOUSE;
	inputStateEventInfo.nEventType = ISE_MBUTTONDBLCLK;
	//
	inputStateEventInfo.nFlags = nFlags;
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_KEYBOARD;
	inputStateEventInfo.nEventType = ISE_KEYDOWN;
	//
	inputStateEventInfo.nChar = nChar;
	inputStateEventInfo.nRepCnt = nRepCnt;
	inputStateEventInfo.nFlags = nFlags;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_KEYBOARD;
	inputStateEventInfo.nEventType = ISE_KEYUP;
	//
	inputStateEventInfo.nChar = nChar;
	inputStateEventInfo.nRepCnt = nRepCnt;
	inputStateEventInfo.nFlags = nFlags;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_KEYBOARD;
	inputStateEventInfo.nEventType = ISE_CHAR;
	//
	inputStateEventInfo.nChar = nChar;
	inputStateEventInfo.nRepCnt = nRepCnt;
	inputStateEventInfo.nFlags = nFlags;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnSysKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_KEYBOARD;
	inputStateEventInfo.nEventType = ISE_SYSKEYDOWN;
	//
	inputStateEventInfo.nChar = nChar;
	inputStateEventInfo.nRepCnt = nRepCnt;
	inputStateEventInfo.nFlags = nFlags;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnSysKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_KEYBOARD;
	inputStateEventInfo.nEventType = ISE_SYSKEYUP;
	//
	inputStateEventInfo.nChar = nChar;
	inputStateEventInfo.nRepCnt = nRepCnt;
	inputStateEventInfo.nFlags = nFlags;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnSysChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_KEYBOARD;
	inputStateEventInfo.nEventType = ISE_SYSCHAR;
	//
	inputStateEventInfo.nChar = nChar;
	inputStateEventInfo.nRepCnt = nRepCnt;
	inputStateEventInfo.nFlags = nFlags;
	//
	OnInputStateEvent( inputStateEventInfo );
}


void CStoreInputState::OnContextMenu( const CTPoint<int> &rMousePoint )
{
	SInputStateEventInfo inputStateEventInfo;
	inputStateEventInfo.nType = IST_CONTEXTMENU;
	inputStateEventInfo.nEventType = ISE_CONTEXTMENU;
	//
	inputStateEventInfo.point = rMousePoint;
	//
	OnInputStateEvent( inputStateEventInfo );
}

// basement storage  


