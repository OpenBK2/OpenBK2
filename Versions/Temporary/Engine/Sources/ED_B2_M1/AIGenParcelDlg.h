#if !defined( __AIGEN_PARCEL_DLG__ )
#define __AIGEN_PARCEL_DLG__
#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/ResizeDialog.h"
#include "Stats_B2_M1/DBMapInfo.h"

//
//	AI General Points Parcel dialog		
//

class CAIGenParcelDlg : public CResizeDialog
{
	DECLARE_DYNAMIC( CAIGenParcelDlg )
	CComboBox comboType;
	CEdit edImportance;

	NDb::EParcelType *pType;
	float *pImportance;

	virtual void DoDataExchange( CDataExchange* pDX );
	BOOL OnInitDialog();

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CAIGenParcelDlg )

public:
	enum { IDD = IDD_DLG_AIGEN_PARCEL };

	CAIGenParcelDlg( CWnd *pParentWindow, NDb::EParcelType *_pType, float *_pImportance );
	virtual ~CAIGenParcelDlg() {}

	void OnOK();
};

#endif

