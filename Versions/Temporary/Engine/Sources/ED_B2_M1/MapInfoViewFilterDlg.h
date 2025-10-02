#pragma once

#include "MapInfoEditor.h"
#include "ResourceDefines.h"

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
	CMapInfoEditorSettings *pMapEditorSettings;
	CMapInfoEditorSettings defMapEditorSettings;
	//
	void SetDialogData();
	void GetDialogData();
	void Apply();

public:
	enum { IDD = IDD_DLG_MAPINFO_VIEW_FILTER };
	//
	CMapInfoViewFilterDlg( CMapInfoEditorSettings *pMapEditorSettings );
	virtual ~CMapInfoViewFilterDlg() {}

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	INT_PTR DoModal();
	void OnOK();
	void OnCancel();

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

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

