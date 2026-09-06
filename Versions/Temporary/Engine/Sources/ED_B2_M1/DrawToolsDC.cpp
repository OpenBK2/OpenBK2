#include "stdafx.h"

#include "DrawToolsDC.h"

#include <cstdint>

namespace NDrawToolsDC
{
	// RGB() packs 0x00BBGGRR, which is what TWidgetColor is documented to be, so
	// these constants are unchanged in value as well as in meaning.
	const TWidgetColor LABEL_BORDER_COLOR	= RGB(0, 0, 0);
	const TWidgetColor LABEL_BG_COLOR			= RGB(255, 255, 221);
	const EFontKind LABEL_MAIN_FONT				= FONT_LABEL;
	//
	//const TWidgetColor SIMPLE_TEXT_COLOR	= RGB(255, 128, 64);
	const TWidgetColor SIMPLE_TEXT_COLOR	= RGB(255, 255, 255);
	const EFontKind SIMPLE_FONT_TYPE			= FONT_SMALL;
	//
	const TWidgetColor BORDER_BG_COLOR		= RGB(127, 127, 127);

	// The DC state these used to back up and restore by hand, into file-scope
	// statics, is IPaintContext::SaveState/RestoreState now. The statics were a
	// single level deep and shared between all three functions, so a nested call
	// would have clobbered them; the context keeps a stack instead.
	void DrawLabelDC( IPaintContext *pPaintContext, const std::string &szLabel, const CVec2 &vScreenPos )
	{
		pPaintContext->SaveState();

		pPaintContext->SetTextBackgroundOpaque( false );
		pPaintContext->SetFont( LABEL_MAIN_FONT );
		pPaintContext->SetBrush( LABEL_BG_COLOR );
		pPaintContext->SetPen( PEN_SOLID, 1, LABEL_BORDER_COLOR );

		// Measure first, then draw the box the text will sit in, then the text.
		const CTRect<int> textRect = pPaintContext->MeasureText( szLabel );
		const int nLeft = static_cast<int>( vScreenPos.x );
		const int nTop = static_cast<int>( vScreenPos.y );
		const CTRect<int> placedRect( nLeft, nTop,
																	nLeft + textRect.Width(), nTop + textRect.Height() );
		//
		const CTRect<int> borderRect( placedRect.left - 2, placedRect.top - 2,
																	placedRect.right + 2, placedRect.bottom + 2 );
		pPaintContext->Rectangle( borderRect );
		pPaintContext->DrawString( placedRect.left, placedRect.top, szLabel );

		pPaintContext->RestoreState();
	}
	//
	void DrawTextDC( IPaintContext *pPaintContext, const std::string &szText, const CVec2 &vScreenPos )
	{
		pPaintContext->SaveState();

		pPaintContext->SetTextBackgroundOpaque( false );
		pPaintContext->SetFont( SIMPLE_FONT_TYPE );
		pPaintContext->SetTextColor( SIMPLE_TEXT_COLOR );
		pPaintContext->DrawString( static_cast<int>( vScreenPos.x ), static_cast<int>( vScreenPos.y ), szText );

		pPaintContext->RestoreState();
	}
	//
	void DrawFrameBorders( IPaintContext *pPaintContext, const CTRect<int> &rBorder1, const CTRect<int> &rBorder2, const CTRect<int> &rWindow )
	{
		pPaintContext->SaveState();

		pPaintContext->SetTextBackgroundOpaque( false );
		//
		// The two border rectangles are unused: filling them was commented out
		// before this port and is left that way. They stay in the signature
		// because the caller still computes and passes them.
		pPaintContext->FrameRect( rWindow, SIMPLE_TEXT_COLOR );

		pPaintContext->RestoreState();
	}
}
