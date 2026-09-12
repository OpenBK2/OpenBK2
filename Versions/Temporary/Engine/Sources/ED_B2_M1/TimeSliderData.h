#pragma once

#include <string>
#include <vector>

// What the movie timeline is, as opposed to how it is drawn.
//
// Split out of TimeSliderControl.h when the movies editor got a wx front end,
// because all of this was already free of any toolkit and both controls need
// exactly the same answers: where a key is, what is selected, which key is
// next. Two copies of "is this point inside the selection" would be two
// timelines that disagree under the same mouse.
//
// TimeSliderControl.h includes this and adds the MFC control; the wx panel in
// MoviesEditorViewWx.cpp includes it and draws the same data.

#define DEF_SEL_RAD	0.1f

// How the timeline is painted, shared for the same reason: two sets of colours
// would be two timelines that look different for no reason anybody chose.
// 0x00BBGGRR, as Win32 COLORREFs are, and unpacked by each front end.
#define TSL_BG_COLOR				RGB( 0xBB, 0xBB, 0xBB )
#define TSL_BG_E_COLOR			RGB( 0x33, 0x33, 0x33 )
#define TSL_KEY_COLOR				RGB( 0xBB, 0x00, 0x00 )
#define TSL_AKEY_COLOR			RGB( 0x33, 0x33, 0x00 )
#define TSL_GRID_COLOR			RGB( 0x33, 0x33, 0x33 )
#define TSL_SLIDER_COLOR		RGB( 0x00, 0x00, 0xBB )
#define TSL_SEL_SPACE_COLOR	RGB( 0x00, 0xBB, 0xBB )
#define TSL_DEF_SPACING				50
#define TSL_DEF_CURSOR_WIDTH	20
#define TSL_DEF_SPACING_SCALE_COEFF	2.0f


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


// The grid spacing the timeline draws at, for a given length, scale and pixel
// width. Shared so that both timelines put their grid lines in the same places.
//
// The guard is the interesting half. The control is sized before it has any
// data, and stays at length 0 until a movie is picked: log( 0 ) is -inf there,
// (int) of that is INT_MIN, and pow( 2, INT_MIN ) is 0 -- a spacing that means
// "a grid line every zero seconds", which is what hung the editor for good the
// moment a length arrived.
inline float CalcTimeSliderSpacing( float fLength, float fScale, int nWidth )
{
	const float fSpan = fLength * fScale;
	if ( (fSpan <= 0.0f) || (nWidth <= 0) )
	{
		return 1.0f;
	}
	const float fSpacing = pow( TSL_DEF_SPACING_SCALE_COEFF,
															(int)(log(fSpan * TSL_DEF_SPACING / nWidth)/log(TSL_DEF_SPACING_SCALE_COEFF) - 1) );
	// a span small enough to underflow the power would do the same as above
	return ( fSpacing > 0.0f ) ? fSpacing : 1.0f;
}
