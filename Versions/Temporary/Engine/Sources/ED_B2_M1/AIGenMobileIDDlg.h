#if !defined( __AIGEN_MOBILE_DLG__ )
#define __AIGEN_MOBILE_DLG__
#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"

//
//		CAIGenMobileDlg
//

class CAIGenMobileDlg : public CResizeDialog
{
	DECLARE_DYNAMIC( CAIGenMobileDlg )
	CEdit edMobileID;
	int *pMobileID;

	virtual void DoDataExchange( CDataExchange* pDX );
	BOOL OnInitDialog();

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CAIGenMobileDlg );

public:
	enum { IDD = IDD_DLG_AIGEN_MOBILE_ID };

	CAIGenMobileDlg( CWnd *pParentWindow, int *_pMobileID );
	virtual ~CAIGenMobileDlg() {}

	void OnOK();
};

#endif
