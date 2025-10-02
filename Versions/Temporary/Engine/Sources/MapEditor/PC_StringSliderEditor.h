#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_SLIDER__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_SLIDER__
#pragma once

#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "PC_ItemEditor.h"
#include "PC_EditorSlider.h"


class CPCStringSliderEditor : public CEdit, public CPCItemEditor, public ICommandHandler
{
	string szDefaultValue;
	CPCEditorSlider wndSlider;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnEnChange();
	//
	afx_msg LRESULT OnMessageEditorSliderChange( WPARAM wParam, LPARAM lParam );

	// CPCStringSliderEditor
	CSliderCtrl* GetSlider() { return &wndSlider; }
	void SetCreateControls( bool _bCreateControls ) { bCreateControls = _bCreateControls; }
	bool GetCreateControls() { return bCreateControls; }

public:
	CPCStringSliderEditor();	
	virtual ~CPCStringSliderEditor();
	
	virtual BOOL PreTranslateMessage( MSG* pMsg );

	//CPCItemEditor
	virtual bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	virtual bool PlaceEditor( const CTRect<int> &rPlaceRect );
	virtual bool ActivateEditor( CDialog *pwndActiveDialog );
	//
	virtual void SetValue( const CVariant &rValue );
	virtual void GetValue( CVariant *pValue );
	virtual void SetDefaultValue();
	//
	void ProcessMessage( UINT nMessage, WPARAM wParam, LPARAM lParam );

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

private:
	virtual void OnChangePos( int nPos ) = 0;
	virtual void OnChangeEditBox() = 0;

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_SLIDER__)
