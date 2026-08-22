
#pragma once
class CTranslateEdit : public CEdit
{
public:
	CTranslateEdit();

public:
	protected:
public:
	virtual ~CTranslateEdit();
protected:
	afx_msg void OnKeyDown(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
	afx_msg void OnChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);

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
	afx_msg void OnKeyDown(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
	afx_msg void OnChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);

	DECLARE_MESSAGE_MAP()

	bool bIgnoreSymbol;
};


