#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include <afxwin.h> //CCheckListBox

typedef hash_map< string, int > CIniValues;

class CBinaryBitFieldDialog : public CResizeDialog
{
	bool bCreateControls;
	CCheckListBox wndTablesList;
	string szFileName;
	hash_map< string, int > name2value;
	hash_map< int, string > value2name;
	const BYTE *pData;
	int nSize;

protected:
	int GetMinimumXDimension() { return 204; }
	int GetMinimumYDimension() { return 106; }

	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CBinaryBitFieldDialog"; }
	bool IsDrawGripper() { return true; }

	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	enum { IDD = IDD_BIT_FIELD };

	CBinaryBitFieldDialog( const string &_szFileName, const BYTE *_pData, const int _nSize, CWnd *pwndParent  );
	~CBinaryBitFieldDialog();

	DECLARE_MESSAGE_MAP()
};


