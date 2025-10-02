#pragma once

#include "ResourceDefines.h"

#include "MapEditorLib/ResizeDialog.h"


class CCreateMODDialog : public CResizeDialog
{
	CString strFolder;
	CString strName;
	CString strDescription;

	void UpdateControls();

protected:
	int GetMinimumXDimension() { return 300; }
	int GetMinimumYDimension() { return 150; }
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CCreateMODDialog"; }
	bool IsDrawGripper() { return true; }

	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();

	afx_msg void OnEnChangeFolderEdit();
	afx_msg void OnEnChangeNameEdit();

public:
	enum { IDD = IDD_CREATE_MOD };

	CCreateMODDialog( CWnd* pParent = NULL );

	const string GetFolder();
	const CString& GetName() { return strName; }
	const CString& GetDescription() { return strDescription; }

	DECLARE_MESSAGE_MAP()
};


