#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "ScriptCameraMovementTypes.h"

//
//
//		SCRIPT CAMERA WINDOW
//
//

class CScriptCameraWindow : public CResizeDialog, public ICommandHandler
{
	CListCtrl lcCameras;
	CButton btnSave;
	CButton btnDel;
	CButton btnRun;

	SScriptCameraWindowData dialogData;
	bool bIsDataSetting;	// data setting flag

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CScriptCameraWindowData )

	LONG nYawTimerID;
	LONG nPitchTimerID;
	LONG nFOVTimerID;
	inline UINT GetYawTimerID() { return 4; }
	inline UINT GetPitchTimerID() { return 5; }
	inline UINT GetFOVTimerID() { return 6; }
	inline UINT GetYawTimerInterval() { return 500; } // Частота в миллисекундах
	inline UINT GetPitchTimerInterval() { return 500; } // Частота в миллисекундах
	inline UINT GetFOVTimerInterval() { return 500; } // Частота в миллисекундах
	void SetYawTimer();
	void SetPitchTimer();
	void SetFOVTimer();
	void KillYawTimer();
	void KillPitchTimer();
	void KillFOVTimer();
	void OnYawTimer();
	void OnPitchTimer();
	void OnFOVTimer();

public:
	enum { IDD = IDD_TAB_MI_SCRIPT_MOVIES };

	CScriptCameraWindow( CWnd* pParentWindow = 0 );
	virtual ~CScriptCameraWindow();

	void DoDataExchange( CDataExchange *pDX );
	BOOL OnInitDialog();

	void GetDialogData( SScriptCameraWindowData *pData );
	void SetDialogData( const SScriptCameraWindowData *pData );
	void ShowManualControls( bool bShow );

	void SetLastAction( SScriptCameraWindowData::EScriptCameraLastAction eAction )
	{
		dialogData.eLastAction = eAction;
	}

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	virtual void NotifyHandler();
	void NotifyHandler( SScriptCameraWindowData::EScriptCameraLastAction eAction );

	virtual void OnOK() {};
	virtual void OnCancel() {};
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedScriptCameraAdd();
	afx_msg void OnBnClickedScriptCameraSave();
	afx_msg void OnBnClickedScriptCameraDelete();
	afx_msg void OnBnClickedScriptCameraRun();
	afx_msg void OnLvnItemchangedListScriptCameras( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnNMDblclkListScriptCameras( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnChangeYaw();
	afx_msg void OnChangePitch();
	afx_msg void OnChangeFOV();
	//
	afx_msg void OnTimer( UINT nIDEvent );
};


