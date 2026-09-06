#pragma once

#include "Interface_Widget.h"
#include "MapEditorLib_export.h"

#include <vector>

// The MFC front-end's IPaintContext, drawing on a CDC.
//
// Front-end private in the same sense as MfcWidget.h: it names MFC, so the
// interface layer and the domain editors include Interface_Widget.h and this
// only appears where a front-end builds one. The viewport's paint handler makes
// one over its CPaintDC and hands it to the active input state; NDrawToolsDC and
// the four states that draw see nothing but IPaintContext.
//
// Pens and brushes are members rather than locals because a GDI object must
// outlive its selection into the DC. Selecting one and letting it go out of
// scope is the classic way to leave a DC pointing at freed memory, and the code
// this replaces avoided it only by keeping every pen alive to the end of the
// function it was created in.
class MAPEDITORLIB_EXPORT CMfcPaintContext : public IPaintContext
{
	struct SSavedState
	{
		CGdiObject *pOldPen;
		CGdiObject *pOldBrush;
		CGdiObject *pOldFont;
		COLORREF oldTextColor;
		int nOldBkMode;
		int nOldROP2;
	};

	CDC *pDC;
	CPen pen;
	CBrush brush;
	CFont font;
	std::vector<SSavedState> savedStateStack;

public:
	explicit CMfcPaintContext( CDC *_pDC );
	virtual ~CMfcPaintContext();

	// IPaintContext
	virtual void SaveState();
	virtual void RestoreState();
	//
	virtual void SetPen( EPenStyle ePenStyle, int nWidth, TWidgetColor color );
	virtual void SetBrush( TWidgetColor color );
	virtual void SetFont( EFontKind eFontKind );
	virtual void SetDrawMode( EDrawMode eDrawMode );
	virtual void SetTextColor( TWidgetColor color );
	virtual TWidgetColor GetTextColor() const;
	virtual void SetTextBackgroundOpaque( bool bOpaque );
	//
	virtual void MoveTo( int nX, int nY );
	virtual void LineTo( int nX, int nY );
	//
	virtual void Rectangle( const CTRect<int> &rRect );
	virtual void FillRect( const CTRect<int> &rRect, TWidgetColor color );
	virtual void FrameRect( const CTRect<int> &rRect, TWidgetColor color );
	//
	virtual void DrawString( int nX, int nY, const std::string &rszText );
	virtual CTRect<int> MeasureText( const std::string &rszText ) const;
};
