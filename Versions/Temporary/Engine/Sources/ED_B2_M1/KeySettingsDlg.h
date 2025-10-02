#pragma once

#include "../MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"

#include "../Stats_B2_M1/DBMapInfo.h"

class CMovEditorKeySettingsDlg : public CResizeDialog
{
	NDb::SScriptMovieKeyPos *pKey;
	string *pszName;

	// CDialog controls
	CEdit wndEditName;
	CButton rbnInFlat;
	CButton rbnInTangent;
	CButton rbnOutFlat;
	CButton rbnOutTangent;
	CEdit wndEditParam;

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CMovEditorKeySettingsDlg )

	virtual void DoDataExchange( CDataExchange *pDX );
	BOOL OnInitDialog();
	void OnOK();

	void GetDialogData();
	void SetDialogData();

public:
	enum { IDD = IDD_DLG_MOVED_KEY_SETTINGS };

	CMovEditorKeySettingsDlg( CWnd *pParentWindow, NDb::SScriptMovieKeyPos *_pKey, string *_pszName );
	virtual ~CMovEditorKeySettingsDlg() {}

	//DECLARE_MESSAGE_MAP()
};
