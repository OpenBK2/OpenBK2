#pragma once


namespace NDrawToolsDC
{
	extern const unsigned LABEL_BORDER_COLOR;
	extern const unsigned LABEL_BG_COLOR;
	extern const unsigned LABEL_MAIN_FONT;
	//
	extern const unsigned SIMPLE_TEXT_COLOR;
	extern const unsigned SIMPLE_FONT_TYPE;
	//
	extern const unsigned BORDER_BG_COLOR;

	void BackupDCSettings( CPaintDC *pDC );
	void RestoreDCSettings( CPaintDC *pDC );

	void DrawLabelDC( CPaintDC *pDC, const string &szLabel, const CVec2 &vScreenPos );
	void DrawTextDC( CPaintDC *pDC, const string &szText, const CVec2 &vScreenPos );

	void DrawFrameBorders( CPaintDC *pDC, const CRect &rBorder1, const CRect &rBorder2, const CRect &rWindow );
};



