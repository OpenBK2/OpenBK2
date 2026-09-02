#include "stdafx.h"

#include "DrawToolsDC.h"

#include <cstdint>

namespace NDrawToolsDC
{
	static int s_nOldBkMode;
	static COLORREF s_oldColor;
	static CBrush *s_pOldBrush;
	static CPen *s_pOldPen;
	static CFont *s_pOldFont;

	const unsigned LABEL_BORDER_COLOR		= RGB(0, 0, 0);
	const unsigned LABEL_BG_COLOR				= RGB(255, 255, 221);
	const unsigned LABEL_MAIN_FONT			= ANSI_VAR_FONT;
	//
	//const unsigned SIMPLE_TEXT_COLOR		= RGB(255, 128, 64);
	const unsigned SIMPLE_TEXT_COLOR		= RGB(255, 255, 255);
	const unsigned SIMPLE_FONT_TYPE			= ANSI_VAR_FONT;
	//
	const unsigned BORDER_BG_COLOR			= RGB(127, 127, 127);

	void BackupDCSettings( CPaintDC *pDC )
	{
		s_pOldBrush = pDC->GetCurrentBrush();
		s_pOldFont = pDC->GetCurrentFont();
		s_pOldPen = pDC->GetCurrentPen();
		s_oldColor = pDC->GetTextColor();
		s_nOldBkMode = pDC->GetBkMode();
	}
	//
	void RestoreDCSettings( CPaintDC *pDC )
	{
		pDC->SelectObject( s_pOldBrush );
		pDC->SelectObject( s_pOldFont );
		pDC->SelectObject( s_pOldPen );
		pDC->SetTextColor( s_oldColor );
		pDC->SetBkMode( s_nOldBkMode );
	}
	//
	void DrawLabelDC( CPaintDC *pDC, const std::string &szLabel, const CVec2 &vScreenPos )
	{
		BackupDCSettings( pDC );

		pDC->SetBkMode( TRANSPARENT );

		CFont font;
		font.CreateStockObject( LABEL_MAIN_FONT );
		pDC->SelectObject( &font );
		//
		CBrush brush;
		brush.CreateSolidBrush( LABEL_BG_COLOR );
		pDC->SelectObject( &brush );
		//
		CPen pen;
		pen.CreatePen( PS_SOLID, 1, (uint32_t)LABEL_BORDER_COLOR );
		pDC->SelectObject( &pen );

		CRect rect( vScreenPos.x, vScreenPos.y, 200, 200 );
		pDC->DrawText( szLabel.c_str(), &rect, DT_CALCRECT | DT_LEFT );
		CRect resizeRect = rect;
		resizeRect.left -= 2;
		resizeRect.top -= 2;
		resizeRect.bottom += 2;
		resizeRect.right += 2;
		pDC->Rectangle( &resizeRect );
		pDC->DrawText( szLabel.c_str(), &rect, DT_LEFT );

		RestoreDCSettings( pDC );
	}
	//
	void DrawTextDC( CPaintDC *pDC, const std::string &szText, const CVec2 &vScreenPos )
	{
		BackupDCSettings( pDC );

		pDC->SetBkMode( TRANSPARENT );

		CFont font;
		font.CreateStockObject( SIMPLE_FONT_TYPE );
		pDC->SelectObject( &font );

		pDC->SetTextColor( SIMPLE_TEXT_COLOR );
		pDC->TextOut( vScreenPos.x, vScreenPos.y, szText.c_str(), szText.length() );

		RestoreDCSettings( pDC );
	}
	//
	void DrawFrameBorders( CPaintDC *pDC, const CRect &rBorder1, const CRect &rBorder2, const CRect &rWindow )
	{
		BackupDCSettings( pDC );

		pDC->SetBkMode( TRANSPARENT );

		//pDC->FillSolidRect( rBorder1, BORDER_BG_COLOR );
		//pDC->FillSolidRect( rBorder2, BORDER_BG_COLOR );

		CBrush solidBrush;
		solidBrush.CreateSolidBrush( SIMPLE_TEXT_COLOR );
		pDC->FrameRect( rWindow, &solidBrush );

		RestoreDCSettings( pDC );
	}
}



