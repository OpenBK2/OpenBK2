#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "PC_ItemEditor.h"

#include <cstdint>

class CPCStringInputEditor : public CEdit, public CPCItemEditor, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CPCStringInputEditor );

	std::string szDefaultValue;
	bool bCreateControls;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnEnChange();

public:
	CPCStringInputEditor();	
	virtual ~CPCStringInputEditor();

	virtual BOOL PreTranslateMessage( MSG* pMsg );

	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	bool PlaceEditor( const CTRect<int> &rPlaceRect );
	bool ActivateEditor( CDialog *pwndActiveDialog );
	//
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
	void SetDefaultValue();
	void EnableEdit( bool bEnable );
	//
	void ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam ) {}

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


