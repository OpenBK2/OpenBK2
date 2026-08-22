#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "PC_ItemEditor.h"

#include <cstdint>

class CPCStringComboEditor : public CComboBox, public CPCItemEditor, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CPCStringComboEditor );
	
	string szDefaultValue;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
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
	void ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam ) {}

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


