#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResizeDialog.h"
#include "DialogData.h"

#include <cstdint>

//
//
//						FORMATION WINDOW
//
//

class CFormationWindow : public CResizeDialog, public ICommandHandler
{
	CListCtrl formationsList;
	CButton chkPropMask;

	int nSelectedIndex;
	bool bIsDataSetting;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CFormationWindow )

	// CPointListDialog
	void NotifyHandler();

	void GetDialogData( SFormationWindowDialogData *pData );
	void SetDialogData( const SFormationWindowDialogData *pData );

public:
	enum { IDD = IDD_TAB_SQD_FORMATION };

	CFormationWindow( CWnd *pParentWindow = 0 );
	virtual ~CFormationWindow();
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	//ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, uint32_t dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnLvnItemchangedPointsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckPropmask();
};


