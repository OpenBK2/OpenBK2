#pragma once

#include "ResourceDefines.h"
#include "PaletteCommands.h"
#include "MapEditorLib/ResizeDialog.h"
#include "DialogData.h"

#include <cstdint>

//
//
//						FORMATION WINDOW
//
//

class CFormationWindow : public CResizeDialog,
                         public CPaletteCommands<SFormationWindowDialogData>
{
	CListCtrl formationsList;
	CButton chkPropMask;

	int nSelectedIndex;
	bool bIsDataSetting;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CFormationWindow )

	// CPointListDialog
	void NotifyHandler();

public:
	// CPaletteCommands. Public now because the dispatch that calls them is on
	// the shared base rather than on this class.
	virtual void GetDialogData( SFormationWindowDialogData *pData );
	virtual void SetDialogData( const SFormationWindowDialogData *pData );

	enum { IDD = IDD_TAB_SQD_FORMATION };

	CFormationWindow( CWnd *pParentWindow = 0 );
	virtual ~CFormationWindow();
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	// HandleCommand and UpdateCommand come from CPaletteCommands, which
	// dispatches them to the two methods above; both implementations of this
	// palette share that.

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnLvnItemchangedPointsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckPropmask();
};


