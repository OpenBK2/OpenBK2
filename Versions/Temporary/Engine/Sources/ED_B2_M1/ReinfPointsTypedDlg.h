#if !defined( __REINF_POINTS_TYPED_DLG__ )
#define __REINF_POINTS_TYPED_DLG__
#pragma once

#include "../MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "ReinfPointsState.h"

//
//	CReinfPointsTypedDlg dialog
//

class CReinfPointsTypedDlg : public CResizeDialog
{
	DECLARE_DYNAMIC( CReinfPointsTypedDlg )
	CReinfPointsState::CTypedTemplateType *pTypedTemplateDlgData;
	CMapInfoEditor *pMapInfoEditor;
	int nCurrentPlayer;
	int nSelectedTemplate;
	int nCurrentReinfPt;
	CListCtrl lcTypedTempl;
	int nTemplatesCount;

	virtual void DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	//
	int GetMinimumXDimension() { return 200; }
	int GetMinimumYDimension() { return 100; }

	DECLARE_MESSAGE_MAP()

public:
	enum { IDD = IDD_DLG_REINFPTS_TEMPLATES };

	CReinfPointsTypedDlg( CWnd *pParentWindow,
												CReinfPointsState::CTypedTemplateType *_pTypedTemplateDlgData,
												CMapInfoEditor *pMapInfoEditor,
												int _nCurrentPlayer,
												int _nCurrentReinfPt );
	virtual ~CReinfPointsTypedDlg() {}

	void OnDestroy();
	BOOL OnInitDialog();
	void SetDialogData();
	void GetDialogData();

	afx_msg void OnBnClickedTypedAdd();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedTypedRemove();
	afx_msg void OnNMClickListReinfPointsTypedTemplate(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
};

#endif // #if !defined( __REINF_POINTS_TYPED_DLG__ )

