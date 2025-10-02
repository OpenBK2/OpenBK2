#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "ReinfPointsState.h"

//
// CReinfPointsTypedTemplateAddDlg dialog
//

class CReinfPointsTypedTemplateAddDlg : public CResizeDialog
{
	DECLARE_DYNAMIC( CReinfPointsTypedTemplateAddDlg )
	CComboBox wndComboType;
	CEdit wndTempEdit;

	CMapInfoEditor *pMapInfoEditor;
	string *pDlgData;
	string szLink;
	CReinfPointsState::STypedTemplate typedTemplate;

	virtual void DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	BOOL OnInitDialog();

	int GetMinimumXDimension() { return 200; }
	int GetMinimumYDimension() { return 100; }

public:
	enum { IDD = IDD_DLG_REINFPTS_ADD_TEMPLATE };

	CReinfPointsTypedTemplateAddDlg( CWnd *pParentWindow, string *pTypeDlgData, CMapInfoEditor *_pMapInfoEditor );
	virtual ~CReinfPointsTypedTemplateAddDlg() {}

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedTemplate();
};


