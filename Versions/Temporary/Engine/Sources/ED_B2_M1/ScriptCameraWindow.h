#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/ResizeDialog.h"
// Was ScriptCameraMovementTypes.h, which held only the run dialog's type
// mnemonics and otherwise passed this state header through.
#include "ScriptCameraState.h"
#include "ScriptCameraView.h"

#include <cstdint>

//
//
//		SCRIPT CAMERA WINDOW
//
//

class CScriptCameraWindow : public CResizeDialog, public CScriptCameraCommands
{
	CListCtrl lcCameras;
	CButton btnSave;
	CButton btnDel;

	SScriptCameraWindowData dialogData;
	bool bIsDataSetting;	// data setting flag

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CScriptCameraWindowData )

	int32_t nYawTimerID;
	int32_t nPitchTimerID;
	int32_t nFOVTimerID;
	inline unsigned GetYawTimerID() { return 4; }
	inline unsigned GetPitchTimerID() { return 5; }
	inline unsigned GetFOVTimerID() { return 6; }
	inline unsigned GetYawTimerInterval() { return 500; } // Частота в миллисекундах
	inline unsigned GetPitchTimerInterval() { return 500; } // Частота в миллисекундах
	inline unsigned GetFOVTimerInterval() { return 500; } // Частота в миллисекундах
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

	//	CScriptCameraCommands
	virtual void GetDialogData( SScriptCameraWindowData *pData );
	virtual void SetDialogData( const SScriptCameraWindowData *pData );
	void ShowManualControls( bool bShow );

	void SetLastAction( SScriptCameraWindowData::EScriptCameraLastAction eAction )
	{
		dialogData.eLastAction = eAction;
	}

	// ICommandHandler
	//
	// Overridden rather than inherited: this palette answers the manual
	// controls command and a get and a set for each of yaw, pitch and FOV, and
	// falls through to CScriptCameraCommands for the dialog-data pair.
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	virtual void NotifyHandler();
	void NotifyHandler( SScriptCameraWindowData::EScriptCameraLastAction eAction );

	virtual void OnOK() {};
	virtual void OnCancel() {};
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedScriptCameraAdd();
	afx_msg void OnBnClickedScriptCameraSave();
	afx_msg void OnBnClickedScriptCameraDelete();
	afx_msg void OnLvnItemchangedListScriptCameras( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnNMDblclkListScriptCameras( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnChangeYaw();
	afx_msg void OnChangePitch();
	afx_msg void OnChangeFOV();
	//
	afx_msg void OnTimer( UINT_PTR nIDEvent );
};


