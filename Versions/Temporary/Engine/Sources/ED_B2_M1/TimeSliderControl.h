#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

#define DEF_SEL_RAD	0.1f

//
//	Movie Slider Key Data
//

struct SMovieKeyData
{
	float fTime;
	bool bActive;
	std::string szCameraName;

	SMovieKeyData() { Clear(); }
	//
	void Clear()
	{
		fTime = 0.0f;
		bActive = false;
		szCameraName = "";
	}
};


//
//	Movie Slider Data
//

struct STimeSliderData
{
private:
	float fCursorTime;

public:
	float fStartTime, fLength, fScale;

//	bool bSelectionMode;
	float fSelectionStart;
	float fSelectionLength;

	float fMoveFromValue, fMoveToValue;

	// keys are not sorted by time!!!
	std::vector<SMovieKeyData> keys;

	STimeSliderData() { Clear(); }
	//
	void Clear()
	{
		fStartTime = 0.0f;
		fLength = 0.0f;
		fScale = 1.0f;
		fCursorTime = 0.0f;
		fSelectionStart = 0.0f;

		//bSelectionMode = false;
		fSelectionLength = 0.0f;

		fMoveFromValue = 0.0f;
		fMoveToValue = 0.0f;

		keys.resize( 0 );
	}
	//
	bool HasActiveKeys() const
	{
		for ( std::vector<SMovieKeyData>::const_iterator itKey = keys.begin(); itKey != keys.end(); ++itKey )
		{
			if ( itKey->bActive )
			{
				return true;
			}
		}
		return false;
	}
	//
	bool HasSingleActiveKey() const
	{
		int nCount = 0;
		for ( std::vector<SMovieKeyData>::const_iterator itKey = keys.begin(); itKey != keys.end(); ++itKey )
		{
			if ( itKey->bActive )
				++nCount;
		}
		return (nCount == 1);
	}
	//
	bool IsSelectionValid() const
	{
		return ( fabs(fSelectionLength) > FP_EPSILON );
	}
	//
	bool IsPointUnderCursor( const float fPoint ) const
	{
		//
		return ( fabs(fPoint - fCursorTime) < DEF_SEL_RAD );
	}
	bool IsPointInsideSelection( const float fPoint ) const
	{
		const float fStart = (std::min)( fSelectionStart, fSelectionStart + fSelectionLength );
		const float fEnd = (std::max)( fSelectionStart, fSelectionStart + fSelectionLength );

		return ( (IsSelectionValid() && (fPoint >= fStart) && (fPoint <= fEnd)) || (IsPointUnderCursor(fPoint)) );
	}
	//
	void UpdateSelection()
	{
		for ( std::vector<SMovieKeyData>::iterator itKey = keys.begin(); itKey != keys.end(); ++itKey )
		{
			itKey->bActive = IsPointInsideSelection( itKey->fTime );
		}
	}
	//
	void ResetSelection()
	{
		fSelectionLength = 0.0f;
		fMoveFromValue = 0.0f;
		fMoveToValue = 0.0f;
	}
	//
	void SetCursorPos( float fTime )
	{
		fCursorTime = fTime;
	}
	//
	float GetCursorPos() const
	{
		return fCursorTime;
	}
	//
	float GetFirstKeyTime() const
	{
		NI_VERIFY( keys.size() > 0, "", return 0.0f )

		float fTime = FP_MAX_VALUE;
		for ( std::vector<SMovieKeyData>::const_iterator itKey = keys.begin(); itKey != keys.end(); ++itKey )
		{
			fTime = (std::min)( fTime, itKey->fTime );
		}
		return fTime;
	}
	//
	float GetLastKeyTime() const
	{
		NI_VERIFY( keys.size() > 0, "", return 0.0f )

		float fTime = 0.0f;
		for ( std::vector<SMovieKeyData>::const_iterator itKey = keys.begin(); itKey != keys.end(); ++itKey )
		{
			fTime = (std::max)( fTime, itKey->fTime );
		}
		return fTime;
	}
	//
	float GetPrevKeyTime() const
	{
		NI_VERIFY( keys.size() > 0, "", return 0.0f )

		int i = -1;
		float fDiff = FP_MAX_VALUE;
		//
		for ( int j = 0; j < keys.size(); ++j )
		{
			const float fCurrDiff = fCursorTime - keys[j].fTime;
			if ( fCurrDiff <= 0 )
				continue;
			//
			if ( fCurrDiff < fDiff )
			{
				fDiff = fCurrDiff;
				i = j;
			}
		}
		if ( i < 0 )
			return fCursorTime;
		//
		return keys[i].fTime;
	}
	//
	float GetNextKeyTime() const
	{
		NI_VERIFY( keys.size() > 0, "", return 0.0f )

			int i = -1;
		float fDiff = FP_MAX_VALUE;
		//
		for ( int j = 0; j < keys.size(); ++j )
		{
			const float fCurrDiff = keys[j].fTime - fCursorTime;
			if ( fCurrDiff <= 0 )
				continue;
			//
			if ( fCurrDiff < fDiff )
			{
				fDiff = fCurrDiff;
				i = j;
			}
		}
		if ( i < 0 )
			return fCursorTime;
		//
		return keys[i].fTime;
	}

};


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

		return rect.Width() * (fTime - data.fStartTime) / (data.fLength * data.fScale);
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



