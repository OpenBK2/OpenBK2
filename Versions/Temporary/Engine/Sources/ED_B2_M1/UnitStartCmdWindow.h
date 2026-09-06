#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "UnitStartCmdData.h"

#include <cstdint>

//
//
//		UNITS START COMMANDS WINDOW
//
//

class CUnitStartCmdWindow : public CResizeDialog, public CUnitStartCmdCommands
{
	// controls
	CButton btnAdd;
	CButton btnDel;
	CButton btnUp;
	CButton btnDown;
	CListCtrl lcCommands;
	//
	bool bIsDataBeginSet;
	SUnitStartCmdWindowData::EAction eLastAction;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CUnitStartCmdWindow )

	// CUnitStartCmdWindow
	void NotifyHandler();
	void NotifyHandler( SUnitStartCmdWindowData::EAction eAction );

	void SetLastAction( SUnitStartCmdWindowData::EAction eAction ) { eLastAction = eAction; }

public:
	// CPaletteCommands. Public now because the dispatch that calls them is on
	// the shared base rather than on this class.
	virtual void GetDialogData( SUnitStartCmdWindowData *pData );
	virtual void SetDialogData( const SUnitStartCmdWindowData *pData );

	enum { IDD = IDD_TAB_MI_UNIT_START_CMD };

	CUnitStartCmdWindow( CWnd *pParentWindow = 0 );
	~CUnitStartCmdWindow();
	//
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	//
	// HandleCommand and UpdateCommand come from CPaletteCommands, which
	// dispatches them to the two methods above.
	//
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDel();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnLvnItemchangedListUnitCmd(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListUnitCmd(NMHDR *pNMHDR, LRESULT *pResult);
};
