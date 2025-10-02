#if !defined(__ELK_TRANSLATE_EDIT__)
#define __ELK_TRANSLATE_EDIT__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTranslateEdit : public CEdit
{
public:
	CTranslateEdit();

public:
	protected:
public:
	virtual ~CTranslateEdit();
protected:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP()

	bool bIgnoreSymbol;
};


class CTranslateButton : public CButton
{
public:
	CTranslateButton();

public:
	virtual ~CTranslateButton();
protected:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP()

	bool bIgnoreSymbol;
};

#endif // !defined(__ELK_TRANSLATE_EDIT__)

