#pragma once

#include "ResourceDefines.h"
#include "CameraPositionData.h"
#include "MapEditorLib/ResizeDialog.h"

#include <cstdint>

// SCameraPositionWindowData and the ICommandHandler dispatch moved to
// CameraPositionData.h, so the wx palette can share them rather than restate
// them. Nothing about either changed.

//
//
//				CAMERA POSITION WINDOW
//
//

class CCameraPositionWindow : public CResizeDialog, public CCameraPositionCommands
{
	CComboBox	wndPalyerComboBox;
	bool bIsDataSetting;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CCameraPositionWindow )

public:
	// CCameraPositionCommands. Public now because the dispatch that calls them
	// is on the shared base rather than on this class.
	virtual void GetDialogData( SCameraPositionWindowData *pData );
	virtual void SetDialogData( const SCameraPositionWindowData *pData );

	enum { IDD = IDD_TAB_MI_START_CAMERA };

	CCameraPositionWindow( CWnd *pParentWindow = 0 );
	virtual ~CCameraPositionWindow();

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	// HandleCommand and UpdateCommand come from CCameraPositionCommands, which
	// dispatches them to the two methods above; both implementations of this
	// palette share that.

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeOwPlayerComboBox();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnBnClickedParamType();
};


