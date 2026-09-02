#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "PC_ItemEditor.h"
#include "PC_EditorButton.h"

#include <cstdint>

class CPCStringMultibuttonEditor : public CEdit, public CPCItemEditor, public ICommandHandler
{
	std::string szDefaultValue;
	CPCEditorButtonList buttonList;
	int nButtonCount;
	bool bIgnoreFocusChange;
	bool bMultiLine;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
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
	virtual bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	virtual bool PlaceEditor( const CTRect<int> &rPlaceRect );
	virtual bool ActivateEditor( CDialog *pwndActiveDialog );
	//
	virtual void SetValue( const CVariant &rValue );
	virtual void GetValue( CVariant *pValue );
	virtual void SetDefaultValue();
	//
	void ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam ) {}

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// CPCStringNewBrowseEditor
	virtual void GetButtonTitle( CString *pstrTitle, int nButtonIndex ) = 0;
	virtual void OnButtonPressed( int nButtonIndex ) = 0;

	DECLARE_MESSAGE_MAP()
};


