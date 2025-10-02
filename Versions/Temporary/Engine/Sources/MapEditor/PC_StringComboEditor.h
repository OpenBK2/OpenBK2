#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "PC_ItemEditor.h"


class CPCStringComboEditor : public CComboBox, public CPCItemEditor, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CPCStringComboEditor );
	
	string szDefaultValue;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnEnChange();
	afx_msg void OnSelchange();

	void SetCreateControls( bool _bCreateControls ) { bCreateControls = _bCreateControls; }
	bool GetCreateControls() { return bCreateControls; }

public:
	CPCStringComboEditor();	
	virtual ~CPCStringComboEditor();

	virtual BOOL PreTranslateMessage( MSG* pMsg );

	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	bool PlaceEditor( const CTRect<int> &rPlaceRect );
	bool ActivateEditor( CDialog *pwndActiveDialog );

	virtual void SetValue( const CVariant &rValue );
	virtual void GetValue( CVariant *pValue );
	virtual void SetDefaultValue();
	//
	void ProcessMessage( UINT nMessage, WPARAM wParam, LPARAM lParam ) {}

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


