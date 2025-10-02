#pragma once

#include "ResourceDefines.h"

#include "MapEditorLib/ResizeDialog.h"
#include <afxwin.h> //CCheckListBox
#include "MapEditorLib/Interface_UserData.h" //CTableSet


class CSelectTablesDialog : public CResizeDialog
{
	bool bCreateControls;
	CCheckListBox wndTablesList;

protected:
	int GetMinimumXDimension() { return 204; }
	int GetMinimumYDimension() { return 106; }

	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CSelectTablesDialog"; }
	bool IsDrawGripper() { return true; }

	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	enum { IDD = IDD_CHOOSE_TABLES };

	CTableSet selectedTables;
	list<string> tables;

	CSelectTablesDialog( CWnd* pParent = NULL );

	DECLARE_MESSAGE_MAP()
};


