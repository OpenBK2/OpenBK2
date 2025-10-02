#pragma once

#include "../MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"

class CMovEditorSettingsDlg : public CResizeDialog
{
	float *pLength;

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CMovEditorSettingsDlg )

	virtual void DoDataExchange( CDataExchange* pDX );
	BOOL OnInitDialog();

	void OnOK();

public:
	enum { IDD = IDD_DLG_MOVED_SETTINGS };

	CMovEditorSettingsDlg( CWnd *pParentWindow, float *_pLength );
	virtual ~CMovEditorSettingsDlg() {}

	//DECLARE_MESSAGE_MAP()
};
