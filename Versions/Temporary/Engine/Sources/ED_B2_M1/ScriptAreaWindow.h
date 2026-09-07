#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResizeDialog.h"
// SScriptAreaWindowData and CScriptAreaCommands moved here: the wx palette
// needs the struct without MFC, and the state was including this dialog header
// to get it.
#include "ScriptAreaView.h"

#include <cstdint>

//
//
//		SCRIPT AREA WINDOW
//
//

class CScriptAreaWindow : public CResizeDialog, public CScriptAreaCommands
{
	// controls
	CButton rbnCircle;
	CButton rbnRectangle;
	CListCtrl lcAreas;
	//
	bool bIsDataBeginSet;
	SScriptAreaWindowData dialogData;

	bool IsDrawGripper() { return false; }
	//
	void NotifyHandler();
	void UpdateControls();

public:
	enum { IDD = IDD_TAB_MI_SCRIPT_AREA };

	CScriptAreaWindow( CWnd* pParentWindow = 0 );
	virtual ~CScriptAreaWindow();

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	virtual void OnOK() {}
	virtual void OnCancel() {}

	//	CScriptAreaCommands
	//
	// The dispatch itself is inherited: this palette answers the two dialog-data
	// commands and nothing else, so there is no HandleCommand or UpdateCommand
	// here at all.
	virtual void GetDialogData( SScriptAreaWindowData *pData );
	virtual void SetDialogData( const SScriptAreaWindowData *pData );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnItemchangedAreaList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnDestroy();
	afx_msg void OnButtonDel();
	afx_msg void OnRadioCircle();
	afx_msg void OnRadioRectangle();
	afx_msg void OnButtonSelect();
};


