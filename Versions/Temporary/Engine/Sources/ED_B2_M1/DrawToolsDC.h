#pragma once

#include "MapEditorLib/Interface_Widget.h"


// Label and text overlays drawn on top of the viewport.
//
// These take an IPaintContext rather than a CPaintDC because CCameraPositionState
// calls DrawLabelDC from IInputState::Draw, which is toolkit-neutral now. The
// other caller, CCFCSceneB2, is front-end code holding a real CPaintDC and wraps
// it in a CMfcPaintContext for the call.
//
// The DC prefix in the names is kept: it distinguishes these from the scene-space
// CSceneDrawTool drawing that most states use, which is the distinction that
// actually matters at a call site.
namespace NDrawToolsDC
{
	extern const TWidgetColor LABEL_BORDER_COLOR;
	extern const TWidgetColor LABEL_BG_COLOR;
	extern const EFontKind LABEL_MAIN_FONT;
	//
	extern const TWidgetColor SIMPLE_TEXT_COLOR;
	extern const EFontKind SIMPLE_FONT_TYPE;
	//
	extern const TWidgetColor BORDER_BG_COLOR;

	void DrawLabelDC( IPaintContext *pPaintContext, const std::string &szLabel, const CVec2 &vScreenPos );
	void DrawTextDC( IPaintContext *pPaintContext, const std::string &szText, const CVec2 &vScreenPos );

	void DrawFrameBorders( IPaintContext *pPaintContext, const CTRect<int> &rBorder1, const CTRect<int> &rBorder2, const CTRect<int> &rWindow );
};
