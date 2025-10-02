
#pragma once
#include "../MapEditorLib/ResizeDialog.h"
#include "TranslateEdit.h"

class CInputViewDialog : public CResizeDialog
{
public:
	friend class CInputViewWindow;
	
	CInputViewDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_INPUT_VIEW };
	CTranslateEdit	m_TranslateEdit;
	CTranslateEdit	m_OriginalEdit;
	CTranslateEdit	m_DescriptionEdit;
	CTranslateButton	m_NotTranslatedButton;
	CTranslateButton	m_OutdatedButton;
	CTranslateButton	m_TranslatedButton;
	CTranslateButton	m_ApprovedButton;
	CTranslateButton	m_NextButton;
	CTranslateButton	m_BackButton;
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnNotTranslatedRadioButton();
	afx_msg void OnTranslatedRadioButton();
	afx_msg void OnApprovedRadioButton();
	afx_msg void OnChangeTranslateEdit();
	afx_msg void OnBackButton();
	afx_msg void OnNextButton();
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

protected:
	const static int vID[];

	CBitmap gameImageBitmap;
	CTPoint<int> gameImageSize;
	string szGameImagePath;

	HICON hNextIcon;
	HICON hBackIcon;
	CWnd *pwndMainFrame;
	bool bTranslatedTextChanged;
	bool bManualState;
	CString strInitialTranslatedText;
	int nInitialState;

	void LoadGameImage( const string &rszGameImagePath );
};


