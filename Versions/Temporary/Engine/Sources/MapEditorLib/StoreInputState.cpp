#include "stdafx.h"
#include "StoreInputState.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


void CStoreInputState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
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


bool CStoreInputState::OnMouseWheel( UINT nFlags, short zDelta, CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnRButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnMButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnMButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
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


void CStoreInputState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
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


void CStoreInputState::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
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


void CStoreInputState::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
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


void CStoreInputState::OnSysKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
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


void CStoreInputState::OnSysKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
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


void CStoreInputState::OnSysChar( UINT nChar, UINT nRepCnt, UINT nFlags )
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


