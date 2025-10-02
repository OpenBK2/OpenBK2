#if !defined(__POINTS_LIST_DIALOG__)
#define __POINTS_LIST_DIALOG__
#pragma once

#include "DialogData.h"

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "../MapEditorLib/Interface_CommandHandler.h"


//
//						POINTS LIST DIALOG
//

class CPointListDialog : public CResizeDialog, public ICommandHandler
{
	CButton chkPass;
	CButton chkPropMask;
	CComboBox seasonCombo;
	CListCtrl pointsList;
	CString szLabel;

	UINT nInstanceID;

	static list<CPointListDialog*> otherDialogs;

	int nSelectedIndex;
	bool bIsDataSetting;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CPointsListDialog )

	// CPointListDialog
	void NotifyHandler();

	void GetDialogData( SPointListDialogData *pData );
	void SetDialogData( const SPointListDialogData *pData );

public:
	enum { IDD = IDD_TAB_BLD_POINTS };

	CPointListDialog( CWnd *pParentWindow = 0 )
		: CResizeDialog( CPointListDialog::IDD, pParentWindow ),
		nInstanceID( 0 )
	{	
		NI_ASSERT( pParentWindow, "CPointListDialog::CPointListDialog() :: Empty constructor call" );
	}
	CPointListDialog( UINT nInstanceID, const CString &rszLabel );
	virtual ~CPointListDialog();
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	//ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeSettingSelectCombo();
	afx_msg void OnBnClickedCheckPropmask();
	afx_msg void OnLvnItemchangedPointsList( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnBnClickedCheckPassability();
	afx_msg void OnBnClickedButtonResetState();
};

#endif // #if !defined(__POINTS_LIST_DIALOG__)

