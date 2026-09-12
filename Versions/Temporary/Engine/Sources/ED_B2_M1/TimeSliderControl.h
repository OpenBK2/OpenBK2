#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "TimeSliderData.h"

#include <cstdint>

// The MFC half of the movie timeline. What it draws -- the keys, the selection,
// the cursor and the spacing between grid lines -- is in TimeSliderData.h, and
// the wx panel in MoviesEditorViewWx.cpp draws the same thing from the same
// data.

//
//
//		Custom time slider control
//
//

class CTimeSliderControl : public CStatic, public ICommandHandler
{
private:
	float fSpacing;

protected:
	afx_msg void OnPaint();

	afx_msg void OnLButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonUp( unsigned nFlags, CPoint point );
	afx_msg void OnMouseMove( unsigned nFlags, CPoint point );
	afx_msg void OnSize( unsigned nType, int cx, int cy );

	DECLARE_MESSAGE_MAP()

	void UpdateActiveKeys() { data.UpdateSelection(); }
	int GetClientX( float fTime ) const
	{
		CRect rect;
		GetClientRect( &rect );

		// With no movie chosen fLength is 0, and this divided by it: the cursor
		// and the timeline's far end were then drawn at whatever the float to int
		// conversion made of inf or NaN, which on MSVC is INT_MIN.
		const float fSpan = data.fLength * data.fScale;
		if ( fSpan <= 0.0f )
			return 0;

		return rect.Width() * (fTime - data.fStartTime) / fSpan;
	}

public:
	CTimeSliderControl();
	virtual ~CTimeSliderControl() {}

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// View Setup
	bool HasActiveKeys() const { return data.HasActiveKeys(); }
	bool HasSingleActiveKey() const { return data.HasSingleActiveKey(); }
	bool IsPointInsideSelection( float fPoint ) const { return data.IsPointInsideSelection( fPoint ); }
	//
	void SetStartTime( float fStartTime );
	void SetLength( float fLength );
	void SetCursorPos( float fCursorTime );
	void ResetSelection();
	void UpdateSelection();
	void SetSelectionStart( float fStart );
	void SetSelectionLength( float fLen );
	void SetMoveTo( float fMoveTo );
	void SetMoveFrom( float fMoveFrom );
	void SetScale( float fScale );
	//
	float GetLength() const { return data.fLength; }
	float GetScale() const { return data.fScale; }
	float GetCursorPos() const { return data.GetCursorPos(); }
	float GetSelectionStart() const { return data.fSelectionStart; }
	float GetMoveOffset() const { return data.fMoveToValue - data.fMoveFromValue; }

	void RefreshSpacing();
	void ClearControl();

	float GetFirstKeyTime() const { return data.GetFirstKeyTime(); }
	float GetLastKeyTime() const { return data.GetLastKeyTime(); }
	float GetPrevKeyTime() const { return data.GetPrevKeyTime(); }
	float GetNextKeyTime() const { return data.GetNextKeyTime(); }

	// let it be in public for a while
	STimeSliderData data;
};
