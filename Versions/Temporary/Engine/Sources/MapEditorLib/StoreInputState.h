#if !defined(__STORE_INPUT_STATE__)
#define __STORE_INPUT_STATE__
#pragma once

#include "DefaultInputState.h"

class CStoreInputState : public CDefaultInputState
{
public:
	enum EInputStateType
	{
		IST_UNKNOWN				= 0,

		IST_MOUSE					= 1,
		IST_KEYBOARD			= 2,
		IST_CONTEXTMENU		= 3,

		IST_COUNT					= 4,
	};
	//
	enum EInputStateEvent
	{
		ISE_UNKNOWN				= 0,

		ISE_MOUSEMOVE			= 1,
		ISE_MOUSEWEEL			= 2,

		ISE_LBUTTONDOWN		= 3,
		ISE_LBUTTONUP			= 4,
		ISE_LBUTTONDBLCLK	= 5,

		ISE_RBUTTONDOWN		= 6,
		ISE_RBUTTONUP			= 7,
		ISE_RBUTTONDBLCLK	= 8,

		ISE_MBUTTONDOWN		= 9,
		ISE_MBUTTONUP			= 10,
		ISE_MBUTTONDBLCLK	= 11,

		ISE_KEYDOWN				= 12,
		ISE_KEYUP					= 13,
		ISE_CHAR					= 14,
		ISE_SYSKEYDOWN		= 15,
		ISE_SYSKEYUP			= 16,
		ISE_SYSCHAR				= 17,

		ISE_CONTEXTMENU		= 18,
		
		ISE_COUNT					= 19,
	};
	//
	struct SInputStateEventInfo
	{
		// common data
		EInputStateType nType;
		EInputStateEvent nEventType;
		UINT nFlags;
		short zDelta;
		CTPoint<int> point;

		UINT nChar;
		UINT nRepCnt;

		SInputStateEventInfo()
			: nType( IST_UNKNOWN ),
				nEventType( ISE_UNKNOWN ),
				nFlags( 0 ),
				zDelta( 0 ),
				point( 0, 0 ),
				nChar( 0 ),
				nRepCnt( 0 ) {}
		SInputStateEventInfo( const SInputStateEventInfo &rInputStateEventInfo )
			: nType( rInputStateEventInfo.nType ),
				nEventType( rInputStateEventInfo.nEventType ),
				nFlags( rInputStateEventInfo.nFlags ),
				zDelta( rInputStateEventInfo.zDelta ),
				point( rInputStateEventInfo.point ),
				nChar( rInputStateEventInfo.nChar ),
				nRepCnt( rInputStateEventInfo.nRepCnt ) {}
		SInputStateEventInfo& operator=( const SInputStateEventInfo &rInputStateEventInfo )
		{
			if( &rInputStateEventInfo != this )
			{
				nType = rInputStateEventInfo.nType;
				nEventType = rInputStateEventInfo.nEventType;
				nFlags = rInputStateEventInfo.nFlags;
				zDelta = rInputStateEventInfo.zDelta;
				point = rInputStateEventInfo.point;
				nChar = rInputStateEventInfo.nChar;
				nRepCnt = rInputStateEventInfo.nRepCnt;
			}
			return *this;
		}	
	};

	//IInputState interface
	virtual void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual bool OnMouseWheel			( UINT nFlags, short zDelta, CTPoint<int> &rMousePoint );
	//
	virtual void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	virtual void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnRButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnRButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	virtual void OnMButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnMButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnMButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	virtual void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnKeyUp					( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnChar						( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnSysKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnSysKeyUp				( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnSysChar				( UINT nChar, UINT nRepCnt, UINT nFlags );
	//
	virtual void OnContextMenu( const CTPoint<int> &rMousePoint );

protected:
	//CStoreInputState
	virtual void OnInputStateEvent( const SInputStateEventInfo &rInputStateEventInfo ) = 0;
};

#endif // !defined(__STORE_INPUT_STATE__)

