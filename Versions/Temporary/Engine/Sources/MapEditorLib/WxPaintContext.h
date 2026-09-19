#pragma once

// The wx front-end's IPaintContext, drawing on a wxDC: MfcPaintContext.h's
// counterpart, for the viewport's wx window.
//
// Header-only because MapEditorLib does not link wx; the modules that do
// include this, as they include WxHostWindow.h.
//
// What it reproduces of the MFC context, because the four input states that
// draw were written against a fresh Windows DC and never set what they did not
// need:
//
//   * A DC's starting state: black text on an opaque white background, which
//     GDI starts a DC with and wx does not -- wx starts with a transparent one.
//   * The stock ANSI_VAR_FONT for both font kinds, read out of Windows.
//   * DRAW_NOT as R2_NOT and DRAW_XOR as R2_XORPEN, which is what wx's wxINVERT
//     and wxXOR are on Windows.
//   * FillRect and FrameRect unaffected by the draw mode: GDI's FillRect and
//     FrameRect paint with PATCOPY whatever the ROP2 is, and a rubber band drawn
//     in DRAW_NOT does not turn a label's background into an inversion.


#include "Interface_Widget.h"

#include <wx/brush.h>
#include <wx/dc.h>
#include <wx/font.h>
#include <wx/pen.h>
#include <wx/window.h>

#include <string>
#include <vector>

class CWxPaintContext : public IPaintContext
{
	struct SSavedState
	{
		wxPen pen;
		wxBrush brush;
		wxFont font;
		wxColour textColour;
		int nBackgroundMode;
		wxRasterOperationMode eLogicalFunction;
	};

	wxDC &rDC;
	const wxWindow *pWindow;
	std::vector<SSavedState> savedStateStack;
	// Where LineTo starts: GDI keeps a current position and wxDC does not.
	wxPoint currentPosition;

	static wxPenStyle ToPenStyle( EPenStyle ePenStyle )
	{
		switch ( ePenStyle )
		{
			case PEN_DOT:
				return wxPENSTYLE_DOT;
			case PEN_DASH:
				// wx makes PS_DASH of this.
				return wxPENSTYLE_LONG_DASH;
			case PEN_SOLID:
			default:
				return wxPENSTYLE_SOLID;
		}
	}

	static wxRasterOperationMode ToLogicalFunction( EDrawMode eDrawMode )
	{
		switch ( eDrawMode )
		{
			case DRAW_NOT:
				return wxINVERT;
			case DRAW_XOR:
				return wxXOR;
			case DRAW_COPY:
			default:
				return wxCOPY;
		}
	}

	wxFont StockFont( EFontKind eFontKind ) const
	{
		// Both kinds are ANSI_VAR_FONT, as in MfcPaintContext.cpp.
#ifdef __WXMSW__
		// Handed to wx as its own string form of a LOGFONT, every field in
		// wxNativeFontInfo::FromString's version 0 order, and not as a LOGFONT:
		// this code sees LOGFONTA and wx is built with LOGFONTW, so the
		// constructor that takes one is not the one wx exports.
		LOGFONTA logFont;
		if ( ::GetObjectA( ::GetStockObject( ANSI_VAR_FONT ), sizeof( logFont ), &logFont ) != 0 )
		{
			const wxString szDescription = wxString::Format( "0;%ld;%ld;%ld;%ld;%ld;%d;%d;%d;%d;%d;%d;%d;%d;%s",
				static_cast<long>( logFont.lfHeight ), static_cast<long>( logFont.lfWidth ),
				static_cast<long>( logFont.lfEscapement ), static_cast<long>( logFont.lfOrientation ),
				static_cast<long>( logFont.lfWeight ),
				logFont.lfItalic, logFont.lfUnderline, logFont.lfStrikeOut, logFont.lfCharSet,
				logFont.lfOutPrecision, logFont.lfClipPrecision, logFont.lfQuality, logFont.lfPitchAndFamily,
				wxString( logFont.lfFaceName ) );
			wxFont font;
			if ( font.SetNativeFontInfo( szDescription ) )
			{
				return font;
			}
		}
#endif
		return *wxSMALL_FONT;
	}

	// A rectangle painted with colour whatever the draw mode, the pen and the
	// brush are, and all three put back.
	void PaintRectangle( const CTRect<int> &rRect, const wxPen &rPen, const wxBrush &rBrush )
	{
		const wxPen oldPen = rDC.GetPen();
		const wxBrush oldBrush = rDC.GetBrush();
		const wxRasterOperationMode eOldFunction = rDC.GetLogicalFunction();
		rDC.SetLogicalFunction( wxCOPY );
		rDC.SetPen( rPen );
		rDC.SetBrush( rBrush );
		rDC.DrawRectangle( rRect.left, rRect.top, rRect.Width(), rRect.Height() );
		rDC.SetBrush( oldBrush );
		rDC.SetPen( oldPen );
		rDC.SetLogicalFunction( eOldFunction );
	}

public:
	// 0x00BBGGRR, which TWidgetColor is documented as.
	static wxColour ToColour( TWidgetColor color )
	{
		return wxColour( color & 0xFF, ( color >> 8 ) & 0xFF, ( color >> 16 ) & 0xFF );
	}

	static TWidgetColor FromColour( const wxColour &rColour )
	{
		return static_cast<TWidgetColor>( rColour.Red() ) | ( static_cast<TWidgetColor>( rColour.Green() ) << 8 ) | ( static_cast<TWidgetColor>( rColour.Blue() ) << 16 );
	}

	CWxPaintContext( wxDC &_rDC, const wxWindow *_pWindow )
		: rDC( _rDC ), pWindow( _pWindow ), currentPosition( 0, 0 )
	{
		rDC.SetTextForeground( *wxBLACK );
		rDC.SetTextBackground( *wxWHITE );
		rDC.SetBackgroundMode( wxBRUSHSTYLE_SOLID );
	}

	virtual ~CWxPaintContext()
	{
		// Anything a state left on the stack is put back, as the MFC context does.
		while ( !savedStateStack.empty() )
		{
			RestoreState();
		}
	}

	// IPaintContext
	virtual void SaveState()
	{
		SSavedState state;
		state.pen = rDC.GetPen();
		state.brush = rDC.GetBrush();
		state.font = rDC.GetFont();
		state.textColour = rDC.GetTextForeground();
		state.nBackgroundMode = rDC.GetBackgroundMode();
		state.eLogicalFunction = rDC.GetLogicalFunction();
		savedStateStack.push_back( state );
	}

	virtual void RestoreState()
	{
		if ( savedStateStack.empty() )
		{
			return;
		}
		const SSavedState &rState = savedStateStack.back();
		rDC.SetPen( rState.pen );
		rDC.SetBrush( rState.brush );
		rDC.SetFont( rState.font );
		rDC.SetTextForeground( rState.textColour );
		rDC.SetBackgroundMode( rState.nBackgroundMode );
		rDC.SetLogicalFunction( rState.eLogicalFunction );
		savedStateStack.pop_back();
	}
	//
	virtual void SetPen( EPenStyle ePenStyle, int nWidth, TWidgetColor color )
	{
		rDC.SetPen( wxPen( ToColour( color ), nWidth, ToPenStyle( ePenStyle ) ) );
	}

	virtual void SetBrush( TWidgetColor color )
	{
		rDC.SetBrush( wxBrush( ToColour( color ) ) );
	}

	virtual void SetFont( EFontKind eFontKind )
	{
		rDC.SetFont( StockFont( eFontKind ) );
	}

	virtual void SetDrawMode( EDrawMode eDrawMode )
	{
		rDC.SetLogicalFunction( ToLogicalFunction( eDrawMode ) );
	}

	virtual void SetTextColor( TWidgetColor color )
	{
		rDC.SetTextForeground( ToColour( color ) );
	}

	virtual TWidgetColor GetTextColor() const
	{
		return FromColour( rDC.GetTextForeground() );
	}

	virtual void SetTextBackgroundOpaque( bool bOpaque )
	{
		rDC.SetBackgroundMode( bOpaque ? wxBRUSHSTYLE_SOLID : wxBRUSHSTYLE_TRANSPARENT );
	}
	//
	virtual void MoveTo( int nX, int nY )
	{
		currentPosition = wxPoint( nX, nY );
	}

	virtual void LineTo( int nX, int nY )
	{
		rDC.DrawLine( currentPosition, wxPoint( nX, nY ) );
		currentPosition = wxPoint( nX, nY );
	}
	//
	virtual void Rectangle( const CTRect<int> &rRect )
	{
		rDC.DrawRectangle( rRect.left, rRect.top, rRect.Width(), rRect.Height() );
	}

	virtual void FillRect( const CTRect<int> &rRect, TWidgetColor color )
	{
		PaintRectangle( rRect, *wxTRANSPARENT_PEN, wxBrush( ToColour( color ) ) );
	}

	virtual void FrameRect( const CTRect<int> &rRect, TWidgetColor color )
	{
		PaintRectangle( rRect, wxPen( ToColour( color ), 1 ), *wxTRANSPARENT_BRUSH );
	}
	//
	virtual void DrawString( int nX, int nY, const std::string &rszText )
	{
		rDC.DrawText( wxString::FromUTF8( rszText.c_str() ), nX, nY );
	}

	virtual CTRect<int> MeasureText( const std::string &rszText ) const
	{
		wxCoord nWidth = 0;
		wxCoord nHeight = 0;
		rDC.GetTextExtent( wxString::FromUTF8( rszText.c_str() ), &nWidth, &nHeight );
		return CTRect<int>( 0, 0, nWidth, nHeight );
	}
};

