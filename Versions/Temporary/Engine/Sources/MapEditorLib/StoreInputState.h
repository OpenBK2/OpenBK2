#pragma once

#include "DefaultInputState.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CStoreInputState : public CDefaultInputState
{
public:
	enum EInputStateType
	{
		IST_UNKNOWN = 0,

		IST_MOUSE = 1,
		IST_KEYBOARD = 2,
		IST_CONTEXTMENU = 3,

		IST_COUNT = 4,
	};

	//
	enum EInputStateEvent
	{
		ISE_UNKNOWN = 0,

		ISE_MOUSEMOVE = 1,
		ISE_MOUSEWEEL = 2,

		ISE_LBUTTONDOWN = 3,
		ISE_LBUTTONUP = 4,
		ISE_LBUTTONDBLCLK = 5,

		ISE_RBUTTONDOWN = 6,
		ISE_RBUTTONUP = 7,
		ISE_RBUTTONDBLCLK = 8,

		ISE_MBUTTONDOWN = 9,
		ISE_MBUTTONUP = 10,
		ISE_MBUTTONDBLCLK = 11,

		ISE_KEYDOWN = 12,
		ISE_KEYUP = 13,

		ISE_CONTEXTMENU = 14,

		ISE_COUNT = 15,
	};

	//
	struct SInputStateEventInfo
	{
		// common data
		EInputStateType nType;
		EInputStateEvent nEventType;
		unsigned nFlags;
		short zDelta;
		CTPoint<int> point;

		unsigned nChar;
		unsigned nRepCnt;

		SInputStateEventInfo()
			: nType( IST_UNKNOWN )
			, nEventType( ISE_UNKNOWN )
			, nFlags( 0 )
			, zDelta( 0 )
			, point( 0, 0 )
			, nChar( 0 )
			, nRepCnt( 0 )
		{
		}

		SInputStateEventInfo( const SInputStateEventInfo &rInputStateEventInfo )
			: nType( rInputStateEventInfo.nType )
			, nEventType( rInputStateEventInfo.nEventType )
			, nFlags( rInputStateEventInfo.nFlags )
			, zDelta( rInputStateEventInfo.zDelta )
			, point( rInputStateEventInfo.point )
			, nChar( rInputStateEventInfo.nChar )
			, nRepCnt( rInputStateEventInfo.nRepCnt )
		{
		}

		SInputStateEventInfo &operator=( const SInputStateEventInfo &rInputStateEventInfo )
		{
			if ( &rInputStateEventInfo != this )
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
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	bool OnMouseWheel( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint ) override;
	//
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	//
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	//
	void OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	//
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
	void OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
	//
	void OnContextMenu( const CTPoint<int> &rMousePoint ) override;

protected:
	//CStoreInputState
	virtual void OnInputStateEvent( const SInputStateEventInfo &rInputStateEventInfo ) = 0;
};
