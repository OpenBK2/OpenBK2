#if !defined( __SCAM_RUN__ )
#define __SCAM_RUN__
#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "ScriptCameraMovementTypes.h"

class CScriptCameraRunDlg : public CResizeDialog
{
	// controls
	CComboBox cbType;
	CComboBox cbStartCam;
	CComboBox cbFinishCam;
	CEdit edTime;
	CEdit edLSpeed;
	CEdit edASpeed;
	CEdit edTarget;
	CEdit edSpline1;
	CEdit edSpline2;

	bool bCreateControls;
	bool bSpeed;

	// data
	SScriptCameraRunDlgData *pDialogData;
	CString szScriptText;

public:
	enum { IDD = IDD_DLG_SCRIPT_CAMERA_RUN };

	CScriptCameraRunDlg(	CWnd* _pParentWindow, SScriptCameraRunDlgData *_pDialogData );
	virtual ~CScriptCameraRunDlg() {}

	void SetDialogData( const SScriptCameraRunDlgData &rDialogData );

	void DoDataExchange( CDataExchange *pDX );
	BOOL OnInitDialog();
	void OnOK();

	void CopyScriptToClipboard();
	void SetControlsMask();

	int GetMinimumXDimension() { return 200; }
	int GetMinimumYDimension() { return 100; }
	bool IsDrawGripper() { return false; }

	DECLARE_MESSAGE_MAP()
	afx_msg void OnCbnSelchangeScamTypeCombo();
	afx_msg void OnEnUpdateScrunEditTime();
};

#endif // if !defined( __SCAM_RUN__ )
