#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_INPUT__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_INPUT__
#pragma once

#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "PC_ItemEditor.h"


class CPCStringInputEditor : public CEdit, public CPCItemEditor, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CPCStringInputEditor );

	string szDefaultValue;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnEnChange();

public:
	CPCStringInputEditor();	
	virtual ~CPCStringInputEditor();

	virtual BOOL PreTranslateMessage( MSG* pMsg );

	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	bool PlaceEditor( const CTRect<int> &rPlaceRect );
	bool ActivateEditor( CDialog *pwndActiveDialog );
	//
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
	void SetDefaultValue();
	void EnableEdit( bool bEnable );
	//
	void ProcessMessage( UINT nMessage, WPARAM wParam, LPARAM lParam ) {}

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_INPUT__)

