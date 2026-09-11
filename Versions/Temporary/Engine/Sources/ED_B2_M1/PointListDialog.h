#pragma once

#include "DialogData.h"
#include "PointListView.h"

#include "ResourceDefines.h"
#include "MapEditorLib/ResizeDialog.h"

#include <cstdint>

//
//						POINTS LIST DIALOG
//

// It was an ICommandHandler with a static list of every instance, and the last
// one constructed registered for CHID_POINTS_LIST_DIALOG and looked the others
// up by instance ID. NPointListView does that now for both implementations;
// this class only answers for itself.
class CPointListDialog : public CResizeDialog, public NPointListView::IPointList
{
	CButton chkPass;
	CButton chkPropMask;
	CComboBox seasonCombo;
	CListCtrl pointsList;
	CString szLabel;

	unsigned nInstanceID;

	int nSelectedIndex;
	bool bIsDataSetting;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CPointsListDialog )

	// CPointListDialog
	void NotifyHandler();

public:
	// NPointListView::IPointList
	virtual unsigned GetInstanceID() const { return nInstanceID; }
	virtual void GetDialogData( SPointListDialogData *pData );
	virtual void SetDialogData( const SPointListDialogData *pData );
	virtual void FollowSeason( NDb::ESeason eSeason );

	enum { IDD = IDD_TAB_BLD_POINTS };

	CPointListDialog( CWnd *pParentWindow = 0 )
		: CResizeDialog( CPointListDialog::IDD, pParentWindow ),
		nInstanceID( 0 )
	{	
		NI_ASSERT( pParentWindow, "CPointListDialog::CPointListDialog() :: Empty constructor call" );
	}
	CPointListDialog( unsigned nInstanceID, const CString &rszLabel );
	virtual ~CPointListDialog();
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeSettingSelectCombo();
	afx_msg void OnBnClickedCheckPropmask();
	afx_msg void OnLvnItemchangedPointsList( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnBnClickedCheckPassability();
	afx_msg void OnBnClickedButtonResetState();
};


