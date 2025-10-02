#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_EDITOR_SLIDER__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_EDITOR_SLIDER__
#pragma once



class CPCEditorSlider : public CSliderCtrl
{
	CWnd *pwndTargetWindow;
	CWnd *pwndNextWindow;
	CWnd *pwndPreviousWindow;

protected:
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void ParentNotify ( UINT message, LPARAM lParam );
public:
	CPCEditorSlider();	
	virtual ~CPCEditorSlider();
	//
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	//
	void SetTargetWindow( CWnd *_pwndTargetWindow ) { pwndTargetWindow = _pwndTargetWindow; }
	void SetNextWindow( CWnd *_pwndNextWindow ) { pwndNextWindow = _pwndNextWindow; }
	void SetPreviousWindow( CWnd *_pwndPreviousWindow ) { pwndPreviousWindow = _pwndPreviousWindow; }
	//
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_EDITOR_SLIDER__)

