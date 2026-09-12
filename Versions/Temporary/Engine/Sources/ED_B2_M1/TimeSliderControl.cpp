#include "stdafx.h"

#include "MapEditorLib/ResourceDefines.h"
#include "CommandHandlerDefines.h"
#include "MapEditorLib/ResizeDialog.h"
#include "DrawToolsDC.h"

#include "TimeSliderControl.h"

#include <climits>
#include <cstdint>

// The colours and the spacing constants are in TimeSliderData.h, with the data
// they draw, so that the wx timeline is the same timeline.

BEGIN_MESSAGE_MAP( CTimeSliderControl, CWnd )
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


CTimeSliderControl::CTimeSliderControl()
	:fSpacing( 1.0f )
{
}


void CTimeSliderControl::OnPaint()
{
	CPaintDC paintDC( this );
	// Was NDrawToolsDC::BackupDCSettings, which is IPaintContext-based now and
	// is for overlay drawing. This is a control painting itself, so it uses
	// MFC's own save/restore, which also nests where the old statics did not.
	const int nSavedDC = paintDC.SaveDC();

	CRect rect;
	GetClientRect( &rect );

	CDC dc;
	int nRes = dc.CreateCompatibleDC( &paintDC );
	CBitmap bmp;
	nRes = bmp.CreateCompatibleBitmap( &paintDC, rect.Width(), rect.Height() );
	CBitmap *pOldBitmap = dc.SelectObject( &bmp );
	dc.FillSolidRect( 0, 0, rect.Width(), rect.Height(), TSL_BG_COLOR );

	const float fMoveOffset = GetMoveOffset();

	// selection
	if ( data.IsSelectionValid() )
	{
		const int nSelStart = GetClientX( fMoveOffset + data.fSelectionStart );
		const int nSelFinish = GetClientX( fMoveOffset + data.fSelectionStart + data.fSelectionLength );
		dc.FillSolidRect( nSelStart, 0, nSelFinish - nSelStart, rect.Height(), TSL_SEL_SPACE_COLOR );
	}

	// cursor
	{
		const int nCursorPos = GetClientX( data.GetCursorPos() );
		CPen gridPen( PS_SOLID, TSL_DEF_CURSOR_WIDTH / 2, TSL_SLIDER_COLOR );
		CPen* pOldPen = dc.SelectObject( &gridPen );

		dc.MoveTo( nCursorPos, 0 );
		dc.LineTo( nCursorPos, rect.Height() );
	}

	// grid
	if ( fSpacing > 0.0f )
	{
		CPen gridPen( PS_SOLID, 1, TSL_GRID_COLOR );
		CPen* pOldPen = dc.SelectObject( &gridPen );

		// The loop bound used to be this division itself, evaluated every
		// iteration. With fSpacing at 0 -- which is what RefreshSpacing left
		// behind while the control had no length yet -- it was +inf, so the loop
		// never ended: the editor hung here drawing two billion grid lines at
		// x = 0 as soon as a movie length arrived. The count is worked out once
		// now, and only used when it is a real number of lines.
		const double fGridLines = (data.fStartTime + data.fLength * data.fScale) / fSpacing;
		const int nGridLines = ( (fGridLines > 0.0) && (fGridLines < (double)INT_MAX) ) ? (int)fGridLines : 0;

		for ( int i = 0; i < nGridLines; ++i )
		{
			const int nGridPos = GetClientX( fSpacing * i );
			// positions only grow with i, so past the right edge there is
			// nothing left to draw
			if ( nGridPos > rect.Width() )
				break;
			if ( nGridPos >= 0 )
			{
				dc.MoveTo( nGridPos, rect.Height() * 3.0f / 4.0f );
				dc.LineTo( nGridPos, rect.Height() );
			}
		}
	}

	// keys
	{
		int nOldBkMode = dc.SetBkMode( OPAQUE );

		for ( std::vector<SMovieKeyData>::const_iterator itKey = data.keys.begin(); itKey != data.keys.end(); ++itKey )
		{
			if ( itKey->bActive )
			{
				const int nKeyPos = GetClientX( fMoveOffset + itKey->fTime );

				if ( (nKeyPos >= 0) && (nKeyPos <= rect.Width()) )
				{
					CPen gridPen( PS_SOLID, 3, TSL_AKEY_COLOR );
					CPen* pOldPen = dc.SelectObject( &gridPen );

					dc.MoveTo( nKeyPos, 0 );
					dc.LineTo( nKeyPos, rect.Height() );

					int nOldBkMode = dc.SetBkMode( TRANSPARENT );
					COLORREF oldColor = dc.GetTextColor();
					dc.SetTextColor( TSL_AKEY_COLOR );

					dc.TextOut( nKeyPos + 2, 0, itKey->szCameraName.c_str(), itKey->szCameraName.length() );
				}
			}
			else
			{
				const int nKeyPos = GetClientX( itKey->fTime );

				if ( (nKeyPos >= 0) && (nKeyPos <= rect.Width()) )
				{
					CPen gridPen( PS_SOLID, 3, TSL_KEY_COLOR );
					CPen* pOldPen = dc.SelectObject( &gridPen );

					dc.MoveTo( nKeyPos, 0 );
					dc.LineTo( nKeyPos, rect.Height() );
				}
			}
		}
	}

	// outer timeline
	{
		const int nCursorPos = GetClientX( data.fLength );
		dc.FillSolidRect( nCursorPos, 0, rect.Width(), rect.Height(), TSL_BG_E_COLOR );
	}

	paintDC.BitBlt( rect.left, rect.top, rect.Width(), rect.Height(), &dc, 0, 0, SRCCOPY );
	dc.SelectObject( pOldBitmap );

	paintDC.RestoreDC( nSavedDC );
}


bool CTimeSliderControl::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	//
	return false;
}


bool CTimeSliderControl::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CTimeSliderControl::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CTimeSliderControl::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}


void CTimeSliderControl::OnLButtonDown( unsigned nFlags, CPoint point )
{
	//
}


void CTimeSliderControl::OnLButtonUp( unsigned nFlags, CPoint point )
{
	//
}


void CTimeSliderControl::OnMouseMove( unsigned nFlags, CPoint point )
{
	//
}


void CTimeSliderControl::RefreshSpacing()
{
	CRect rect;
	GetClientRect( &rect );

	// Shared with the wx timeline, guard and all; see TimeSliderData.h.
	fSpacing = CalcTimeSliderSpacing( data.fLength, data.fScale, rect.Width() );
	//RedrawWindow();
}


void CTimeSliderControl::SetStartTime( float fStartTime )
{
	data.fStartTime = fStartTime;
	RedrawWindow();
}


void CTimeSliderControl::SetLength( float fLength )
{
	data.fLength = fLength;
	RedrawWindow();
}


void CTimeSliderControl::SetCursorPos( float fCursorTime )
{
	data.SetCursorPos( fCursorTime );
	RedrawWindow();
}


void CTimeSliderControl::ResetSelection()
{
	data.ResetSelection();
	RedrawWindow();
}


void CTimeSliderControl::UpdateSelection()
{
	data.UpdateSelection();
	RedrawWindow();
}


void CTimeSliderControl::SetSelectionStart( float fStart )
{
	data.fSelectionStart = fStart;
	RedrawWindow();
}


void CTimeSliderControl::SetSelectionLength( float fLen )
{
	data.fSelectionLength = fLen;
	RedrawWindow();
}


void CTimeSliderControl::SetMoveFrom( float fMoveFrom )
{
	data.fMoveFromValue = fMoveFrom;
	RedrawWindow();
}


void CTimeSliderControl::SetMoveTo( float fMoveTo )
{
	data.fMoveToValue = fMoveTo;
	RedrawWindow();
}


void CTimeSliderControl::SetScale( float fScale )
{
	data.fScale = fScale;
	RedrawWindow();
}


void CTimeSliderControl::ClearControl()
{
	data.Clear();
	RedrawWindow();
}


void CTimeSliderControl::OnSize( unsigned nType, int cx, int cy )
{
	CStatic::OnSize( nType, cx, cy );
	//
	RefreshSpacing();
	//if ( wndTimeSliderControl.IsWindowEnabled() )
	//	wndTimeSliderControl.RefreshSpacing();
}



