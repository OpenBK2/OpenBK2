#if !defined(__PROGRESS_BAR_WINDOW__)
#define __PROGRESS_BAR_WINDOW__
#pragma once


class CProgressBarWindow : public CDialog
{
public:
	CProgressBarWindow();
	bool Create( CWnd *pParent );

	void Start( int nRange, const string & szCaption );
	void StepIt();
	void Finish();
	void SetCaption( const string & szCaption );

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	// members
private:
	SECProgressCtrl wndProgress;
	CStatic wndCaption;
};

#endif // !defined(__PROGRESS_BAR_WINDOW__)

