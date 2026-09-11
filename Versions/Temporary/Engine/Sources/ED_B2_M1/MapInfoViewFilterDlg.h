#pragma once

#include "MapInfoEditorSettings.h"
#include "ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

//
//
//	CMapInfoViewFilterDlg
//
//

class CMapInfoViewFilterDlg : public CDialog, public ICommandHandler
{
	CListCtrl objTypesList;
	CButton chkWireFrame;
	CButton chkBoundingBoxes;
	CButton chkGrid;
	CButton chkShowTerrain;
	CButton chkShowShadows;
	CButton chkShowWarfog;
	CButton chkShowStats;
	CButton chkMipmap;
	CButton chkOverdraw;
	CComboBox comboGridSize;
	//
	bool bIsDataSetting;
	//
	// The filter being edited, and a copy of it as it was on entry for Cancel to
	// put back. These were the whole editor settings and a copy of all of them,
	// of which the dialog only ever touched viewFilterData; the boundary in
	// MapInfoViewFilter.h hands over just that.
	CMapInfoEditorSettings::SViewFilterData *pViewFilter;
	CMapInfoEditorSettings::SViewFilterData defViewFilter;
	//
	void SetDialogData();
	void GetDialogData();
	void Apply();

public:
	enum { IDD = IDD_DLG_MAPINFO_VIEW_FILTER };
	//
	CMapInfoViewFilterDlg( CWnd *pParent, CMapInfoEditorSettings::SViewFilterData *pViewFilter );
	virtual ~CMapInfoViewFilterDlg() {}

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	INT_PTR DoModal();
	void OnOK();
	void OnCancel();

	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonDefault();
	afx_msg void OnCbnSelchangeComboGridSize() { GetDialogData(); }
	afx_msg void OnBnClickedCheckGrid() { GetDialogData(); }
	afx_msg void OnBnClickedCheckBB() { GetDialogData(); }
	afx_msg void OnBnClickedCheckWF() { GetDialogData(); }
	afx_msg void OnBnClickedCheckTerraF() { GetDialogData(); }
	afx_msg void OnBnClickedCheckShadowsF() { GetDialogData(); }
	afx_msg void OnBnClickedCheckWarFogF() { GetDialogData(); }
	afx_msg void OnBnClickedCheckStatsF() { GetDialogData(); }
	afx_msg void OnBnClickedCheckMipmapF() { GetDialogData(); }
	afx_msg void OnBnClickedCheckOverdrawF() { GetDialogData(); }
	afx_msg void OnLvnItemchangedListObjTypes( NMHDR *pNMHDR, LRESULT *pResult );
};


