#if !defined( __SCAM_ADD_DLG__ )
#define __SCAM_ADD_DLG__
#pragma once

#include "ScriptCameraWindow.h"

class CScriptCameraAddDlg : public CResizeDialog
{
	string *pszNewCameraName;
	vector<NCamera::CCameraPlacement> vExistingCameras;

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CScriptCameraAddDlg )

	virtual void DoDataExchange( CDataExchange* pDX );
	BOOL OnInitDialog();

public:
	enum { IDD = IDD_DLG_SCRIPT_CAMERA_ADD };

	CScriptCameraAddDlg( CWnd *pParentWindow, string *_pszNewCameraName, const vector<NCamera::CCameraPlacement> &rvExistingCameras );
	virtual ~CScriptCameraAddDlg() {}

	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnChangeScriptCameraNameEdit();
};

#endif // if !defined( __SCAM_ADD_DLG__ )

