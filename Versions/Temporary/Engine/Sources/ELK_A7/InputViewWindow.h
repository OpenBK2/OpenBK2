#if !defined(__ELK_INPUT_VIEW_WINDOW__)
#define __ELK_INPUT_VIEW_WINDOW__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ELK_Types.h"
#include "InputViewDialog.h"

class CInputViewWindow : public CWnd
{
protected:
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	DECLARE_MESSAGE_MAP()

public:
	CInputViewDialog wndForm;
	bool bGameExists;

	CInputViewWindow();
	virtual ~CInputViewWindow();

	void ClearControls();
	void EnableControlsForText( bool bEnable );
	void EnableControlsForFolder( bool bEnable );
	void EnableNextButton( bool bEnable );
	void EnableBackButton( bool bEnable );

	void SetOriginalText( const CString &rstrText );
	void SetTranslatedText( const CString &rstrText );
	void SetDescription( const CString &rstrText );
	void SetState( int nState );
	bool IsTranslatedTextChanged() { return wndForm.bTranslatedTextChanged; }

	void GetTranslatedText( CString *pstrText );
	int GetState();

	void SetMainFrameWindow( CWnd *_pwndMainFrame );
	CEdit* GetTranslateEdit();

	void SelectText( const struct SMainFrameParams::SSearchParam &rSearchParam );
	void LoadGameImage( const string &rszGameImagePath );

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};

#endif // !defined(__ELK_INPUT_VIEW_WINDOW__)

