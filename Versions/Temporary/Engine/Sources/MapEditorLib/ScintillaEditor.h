#pragma once

#include "Scintilla/Scintilla.h"

#include <cstdint>

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CScintillaEditorWindow : public CWnd
{
	SciFnDirect pfnScintilla;
	sptr_t pScintilla;
	CWnd* pwndStatusStringWindow;
	//CWnd *pwndTargetWindow;

	void UpdateStatusStringWindow();

protected:
	//afx_msg void OnSetFocus( CWnd* pOldWnd );
	//afx_msg void OnKillFocus( CWnd* pNewWnd );

public:
	CScintillaEditorWindow();
	virtual ~CScintillaEditorWindow();
	//
	virtual BOOL CreateEx( CWnd* pwndParentWindow, uint32_t dwStyleEx, uint32_t dwStyle, const CRect &rStartRect, unsigned nControlID /**, CWnd *_pwndTargetWindow **/ );
	sptr_t Command( int nCommand, uptr_t wParam = 0, sptr_t lParam = 0 );
	//
	void SetStatusStringWindow( CWnd* _pwndStatusStringWindow );
	CWnd* GetStatusStringWindow() { return pwndStatusStringWindow; }
	//	
	void SetText( const std::string &rszText );
	int GetText( std::string *pszText );
	//
	//void SetTargetWindow( CWnd* _pwndTargetWindow ) { pwndTargetWindow = _pwndTargetWindow; }
	//CWnd* GetTargetWindow() { return pwndTargetWindow; }
	//
	DECLARE_MESSAGE_MAP()
};


