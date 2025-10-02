#if !defined(__TREE__GDB_BROWSER__NEW_OBJECT_DIALOG__)
#define __TREE__GDB_BROWSER__NEW_OBJECT_DIALOG__
#pragma once

#include "ResourceDefines.h"
#include "..\MapEditorLib\Interface_Builder.h"
#include "..\MapEditorLib\ResizeDialog.h"


class CNewObjectDialog : public CResizeDialog
{
	bool bCreateControls;
	bool bEnableType;
	int nObjectTypeNameIndex;
	string szObjectTypeNamePostrfix;
	vector<string> objectTypeNameList;

	CComboBox wndObjectTypeComboBox;
	CString	strName;
	int nNeedExport;

	SBuildDataParams *pBuildDataParams;

	void UpdateOKButton();
	void UpdateTitle();
	void CreateTypeList();
	void UpdateTypePostfix();

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	afx_msg void OnNameEditChange();
	afx_msg void OnSelchangeObjectTypeComboBox();
	afx_msg void OnExportCheckBox();
	afx_msg void OnAddTypeCheckBox();

public:
	enum { IDD = IDD_NEW_OBJECT };
	//
	CNewObjectDialog( CWnd* pParent = NULL );
	//
	int GetMinimumXDimension() { return 335; }
	int GetMinimumYDimension() { return 105; }

	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CNewObjectDialog"; }
	bool IsDrawGripper() { return true; }

	void SetBuildDataParams( const vector<string> &rObjectTypeNameList, int _nObjectTypeNameIndex, SBuildDataParams *_pBuildDataParams );
	//
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__TREE__GDB_BROWSER__NEW_OBJECT_DIALOG__)

