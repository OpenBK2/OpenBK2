#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/ResizeDialog.h"

//
//
//		ENTER NAME DIALOG
//
//

class CEnterNameDialog : public CResizeDialog
{
	// The starting text in, the accepted name out. The last name used to be a
	// static here; NEnterName keeps it now, for both implementations.
	std::string szName;
	std::string szDlgCaption;
	std::string szLabelText;

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CEnterNameDialog )

public:
	enum { IDD = IDD_DLG_AREA_NAME };

	CEnterNameDialog( CWnd* pParentWindow, const std::string &szDlgCaption,  const std::string &szLabelText,
										const std::string &rszInitialName );
	virtual ~CEnterNameDialog() {}
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();

	virtual void OnOK();
	virtual void OnCancel();

	void GetName( std::string *pName );

	DECLARE_MESSAGE_MAP()
};


