#include "stdafx.h"

#include "../MapEditorLib/ResourceDefines.h"
#include "CommandHandlerDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "DrawToolsDC.h"

#include "TimeSliderControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
	NDrawToolsDC::BackupDCSettings( &paintDC );

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
	{
		CPen gridPen( PS_SOLID, 1, TSL_GRID_COLOR );
		CPen* pOldPen = dc.SelectObject( &gridPen );

		for ( int i = 0; i < (data.fStartTime + data.fLength * data.fScale) / fSpacing; ++i )
		{
			const int nGridPos = GetClientX( fSpacing * i );
			if ( (nGridPos >= 0) && (nGridPos <= rect.Width()) )
			{
				dc.MoveTo( nGridPos, rect.Height() * 3.0f / 4.0f );
				dc.LineTo( nGridPos, rect.Height() );
			}
		}
	}

	// keys
	{
		int nOldBkMode = dc.SetBkMode( OPAQUE );

		for ( vector<SMovieKeyData>::const_iterator itKey = data.keys.begin(); itKey != data.keys.end(); ++itKey )
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

	NDrawToolsDC::RestoreDCSettings( &paintDC );
}


bool CTimeSliderControl::HandleCommand( UINT nCommandID, DWORD dwData )
{
	//
	return false;
}


bool CTimeSliderControl::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CTimeSliderControl::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CTimeSliderControl::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}


void CTimeSliderControl::OnLButtonDown( UINT nFlags, CPoint point )
{
	//
}


void CTimeSliderControl::OnLButtonUp( UINT nFlags, CPoint point )
{
	//
}


void CTimeSliderControl::OnMouseMove( UINT nFlags, CPoint point )
{
	//
}


void CTimeSliderControl::RefreshSpacing()
{
	CRect rect;
	GetClientRect( &rect );

	fSpacing = pow( TSL_DEF_SPACING_SCALE_COEFF, (int)(log(data.fLength * data.fScale * TSL_DEF_SPACING / rect.Width())/log(TSL_DEF_SPACING_SCALE_COEFF) - 1) );
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


void CTimeSliderControl::OnSize( UINT nType, int cx, int cy )
{
	CStatic::OnSize( nType, cx, cy );
	//
	RefreshSpacing();
	//if ( wndTimeSliderControl.IsWindowEnabled() )
	//	wndTimeSliderControl.RefreshSpacing();
}



