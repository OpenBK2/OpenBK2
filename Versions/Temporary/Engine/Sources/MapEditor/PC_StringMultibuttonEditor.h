#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_MULTIBUTTON__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_MULTIBUTTON__
#pragma once

#include "ResourceDefines.h"
#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "PC_ItemEditor.h"
#include "PC_EditorButton.h"


class CPCStringMultibuttonEditor : public CEdit, public CPCItemEditor, public ICommandHandler
{
	string szDefaultValue;
	CPCEditorButtonList buttonList;
	int nButtonCount;
	bool bIgnoreFocusChange;
	bool bMultiLine;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnEnChange();
	afx_msg void OnDestroy();
	//
	afx_msg LRESULT OnMessageEditorButtonChange( WPARAM wParam, LPARAM lParam );
	//
	void SetMultiLine( bool _bMultiLine ) { bMultiLine = _bMultiLine; }
	void SetCreateControls( bool _bCreateControls ) { bCreateControls = _bCreateControls; }
	bool GetCreateControls() { return bCreateControls; }

public:

	CPCStringMultibuttonEditor( int _nButtonCount );
	virtual ~CPCStringMultibuttonEditor();
	
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
	void ProcessMessage( UINT nMessage, WPARAM wParam, LPARAM lParam ) {}

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	// CPCStringNewBrowseEditor
	virtual void GetButtonTitle( CString *pstrTitle, int nButtonIndex ) = 0;
	virtual void OnButtonPressed( int nButtonIndex ) = 0;

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_MULTIBUTTON__)
