#pragma once

#include "ScriptCameraWindow.h"

class CScriptCameraAddDlg : public CResizeDialog
{
	std::string *pszNewCameraName;
	std::vector<NCamera::CCameraPlacement> vExistingCameras;

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CScriptCameraAddDlg )

	virtual void DoDataExchange( CDataExchange* pDX );
	BOOL OnInitDialog();

public:
	enum { IDD = IDD_DLG_SCRIPT_CAMERA_ADD };

	CScriptCameraAddDlg( CWnd *pParentWindow, std::string *_pszNewCameraName, const std::vector<NCamera::CCameraPlacement> &rvExistingCameras );
	virtual ~CScriptCameraAddDlg() {}

	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnChangeScriptCameraNameEdit();
};


