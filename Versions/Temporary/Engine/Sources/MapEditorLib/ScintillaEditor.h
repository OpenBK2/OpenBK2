#if !defined(__COMMON_CONTROLS__SCINTILLA_EDITOR_WINDOW__)
#define __COMMON_CONTROLS__SCINTILLA_EDITOR_WINDOW__
#pragma once



class CScintillaEditorWindow : public CWnd
{
	int ( *pfnScintilla )( void*, int, int, int );
	void *pScintilla;
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
	virtual BOOL CreateEx( CWnd* pwndParentWindow, DWORD dwStyleEx, DWORD dwStyle, const CRect &rStartRect, UINT nControlID /**, CWnd *_pwndTargetWindow **/ );
	int Command( int nCommand, int wParam = 0, int lParam = 0 );
	//
	void SetStatusStringWindow( CWnd* _pwndStatusStringWindow );
	CWnd* GetStatusStringWindow() { return pwndStatusStringWindow; }
	//	
	void SetText( const string &rszText );
	int GetText( string *pszText );
	//
	//void SetTargetWindow( CWnd* _pwndTargetWindow ) { pwndTargetWindow = _pwndTargetWindow; }
	//CWnd* GetTargetWindow() { return pwndTargetWindow; }
	//
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__SCINTILLA_EDITOR_WINDOW__)
