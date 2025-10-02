#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "../MapEditorLib/Interface_CommandHandler.h"

//
//
//				CAMERA POSITION WINDOW DATA
//
//

struct SCameraPositionWindowData
{
	int nPlayerIndex;
	int nPlayerCount;
	bool bAllParams;
	//	
	SCameraPositionWindowData() :
		nPlayerIndex( -1 ),
		nPlayerCount( 0 ),
		bAllParams( false )
	{
	}
	//
	void Clear()
	{
		nPlayerIndex = -1;
		nPlayerCount = 0;
		bAllParams = false;
	}
};

//
//
//				CAMERA POSITION WINDOW
//
//

class CCameraPositionWindow : public CResizeDialog, public ICommandHandler
{
	CComboBox	wndPalyerComboBox;
	bool bIsDataSetting;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CCameraPositionWindow )

	// CCameraPositionWindow
	void GetDialogData( SCameraPositionWindowData *pData );
	void SetDialogData( const SCameraPositionWindowData *pData );

public:
	enum { IDD = IDD_TAB_MI_START_CAMERA };

	CCameraPositionWindow( CWnd *pParentWindow = 0 );
	virtual ~CCameraPositionWindow();

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	//	ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeOwPlayerComboBox();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnBnClickedParamType();
};

