#include "stdafx.h"

#include "MfcPaintContext.h"


namespace
{
	// TWidgetColor is documented as 0x00BBGGRR, which is COLORREF's packing, so
	// these are a rename rather than a conversion. Kept as functions anyway so
	// that a front-end whose colours are packed the other way has one place to
	// change.
	inline COLORREF ToColorRef( TWidgetColor color )
	{
		return static_cast<COLORREF>( color );
	}

	inline TWidgetColor FromColorRef( COLORREF color )
	{
		return static_cast<TWidgetColor>( color );
	}

	inline int ToPenStyle( EPenStyle ePenStyle )
	{
		switch ( ePenStyle )
		{
			case PEN_DOT:
				return PS_DOT;
			case PEN_DASH:
				return PS_DASH;
			case PEN_SOLID:
			default:
				return PS_SOLID;
		}
	}

	inline int ToROP2( EDrawMode eDrawMode )
	{
		switch ( eDrawMode )
		{
			case DRAW_NOT:
				return R2_NOT;
			case DRAW_XOR:
				return R2_XORPEN;
			case DRAW_COPY:
			default:
				return R2_COPYPEN;
		}
	}

	// Both of the editor's font kinds resolved to ANSI_VAR_FONT: NDrawToolsDC
	// defined LABEL_MAIN_FONT and SIMPLE_FONT_TYPE as the same stock object.
	// The distinction is kept in the enum because the callers mean different
	// things by it and a front-end with two real faces should honour that.
	inline int ToStockFont( EFontKind eFontKind )
	{
		switch ( eFontKind )
		{
			case FONT_SMALL:
			case FONT_LABEL:
			default:
				return ANSI_VAR_FONT;
		}
	}

	inline CRect ToCRect( const CTRect<int> &rRect )
	{
		return CRect( rRect.left, rRect.top, rRect.right, rRect.bottom );
	}
}


CMfcPaintContext::CMfcPaintContext( CDC *_pDC )
	: pDC( _pDC )
{
}


CMfcPaintContext::~CMfcPaintContext()
{
	// Anything the caller left on the stack is put back, so that a state which
	// returned early cannot leave the front-end's DC holding one of our pens
	// after this object and its pens are gone.
	while ( !savedStateStack.empty() )
	{
		RestoreState();
	}
}


void CMfcPaintContext::SaveState()
{
	SSavedState state;
	state.pOldPen = pDC->GetCurrentPen();
	state.pOldBrush = pDC->GetCurrentBrush();
	state.pOldFont = pDC->GetCurrentFont();
	state.oldTextColor = pDC->GetTextColor();
	state.nOldBkMode = pDC->GetBkMode();
	state.nOldROP2 = pDC->GetROP2();
	savedStateStack.push_back( state );
}


void CMfcPaintContext::RestoreState()
{
	if ( savedStateStack.empty() )
	{
		return;
	}
	const SSavedState &rState = savedStateStack.back();
	pDC->SelectObject( rState.pOldPen );
	pDC->SelectObject( rState.pOldBrush );
	pDC->SelectObject( rState.pOldFont );
	pDC->SetTextColor( rState.oldTextColor );
	pDC->SetBkMode( rState.nOldBkMode );
	pDC->SetROP2( rState.nOldROP2 );
	savedStateStack.pop_back();
}


void CMfcPaintContext::SetPen( EPenStyle ePenStyle, int nWidth, TWidgetColor color )
{
	// Deselect before deleting: a GDI object still selected into a DC cannot be
	// destroyed, and DeleteObject silently fails if it is.
	pDC->SelectStockObject( BLACK_PEN );
	pen.DeleteObject();
	pen.CreatePen( ToPenStyle( ePenStyle ), nWidth, ToColorRef( color ) );
	pDC->SelectObject( &pen );
}


void CMfcPaintContext::SetBrush( TWidgetColor color )
{
	pDC->SelectStockObject( WHITE_BRUSH );
	brush.DeleteObject();
	brush.CreateSolidBrush( ToColorRef( color ) );
	pDC->SelectObject( &brush );
}


void CMfcPaintContext::SetFont( EFontKind eFontKind )
{
	pDC->SelectStockObject( SYSTEM_FONT );
	font.DeleteObject();
	font.CreateStockObject( ToStockFont( eFontKind ) );
	pDC->SelectObject( &font );
}


void CMfcPaintContext::SetDrawMode( EDrawMode eDrawMode )
{
	pDC->SetROP2( ToROP2( eDrawMode ) );
}


void CMfcPaintContext::SetTextColor( TWidgetColor color )
{
	pDC->SetTextColor( ToColorRef( color ) );
}


TWidgetColor CMfcPaintContext::GetTextColor() const
{
	return FromColorRef( pDC->GetTextColor() );
}


void CMfcPaintContext::SetTextBackgroundOpaque( bool bOpaque )
{
	pDC->SetBkMode( bOpaque ? OPAQUE : TRANSPARENT );
}


void CMfcPaintContext::MoveTo( int nX, int nY )
{
	pDC->MoveTo( nX, nY );
}


void CMfcPaintContext::LineTo( int nX, int nY )
{
	pDC->LineTo( nX, nY );
}


void CMfcPaintContext::Rectangle( const CTRect<int> &rRect )
{
	const CRect rect = ToCRect( rRect );
	pDC->Rectangle( &rect );
}


void CMfcPaintContext::FillRect( const CTRect<int> &rRect, TWidgetColor color )
{
	const CRect rect = ToCRect( rRect );
	CBrush fillBrush;
	fillBrush.CreateSolidBrush( ToColorRef( color ) );
	pDC->FillRect( &rect, &fillBrush );
}


void CMfcPaintContext::FrameRect( const CTRect<int> &rRect, TWidgetColor color )
{
	const CRect rect = ToCRect( rRect );
	CBrush frameBrush;
	frameBrush.CreateSolidBrush( ToColorRef( color ) );
	pDC->FrameRect( &rect, &frameBrush );
}


void CMfcPaintContext::DrawString( int nX, int nY, const std::string &rszText )
{
	pDC->TextOut( nX, nY, rszText.c_str(), rszText.length() );
}


CTRect<int> CMfcPaintContext::MeasureText( const std::string &rszText ) const
{
	// DT_CALCRECT measures instead of drawing and writes the result back through
	// the rectangle, which is why this needs a mutable copy and no output.
	CRect rect( 0, 0, 0, 0 );
	pDC->DrawText( rszText.c_str(), &rect, DT_CALCRECT | DT_LEFT );
	return CTRect<int>( rect.left, rect.top, rect.right, rect.bottom );
}
