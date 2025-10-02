#if !defined( __ENTER_NAME_DIALOG__ )
#define __ENTER_NAME_DIALOG__
#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"

//
//
//		ENTER NAME DIALOG
//
//

class CEnterNameDialog : public CResizeDialog
{
	static string szLastName;
	string szName;
	string szDlgCaption;
	string szLabelText;

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CEnterNameDialog )

public:
	enum { IDD = IDD_DLG_AREA_NAME };

	CEnterNameDialog( CWnd* pParentWindow, const string &szDlgCaption,  const string &szLabelText );
	virtual ~CEnterNameDialog() {}
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();

	virtual void OnOK();
	virtual void OnCancel();

	void GetName( string *pName );

	DECLARE_MESSAGE_MAP()
};

#endif // #if !defined( __ENTER_NAME_DIALOG__ )

