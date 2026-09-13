#pragma once

#include "ResourceDefines.h"
#include "BitFieldView.h"
#include "MapEditorLib/ResizeDialog.h"
#include <afxwin.h> //CCheckListBox

#include <cstdint>

// The MFC half of NBitField. Which names are listed, which are checked and what
// OK writes come from NBitField, which the wx dialog uses as well.
class CBinaryBitFieldDialog : public CResizeDialog
{
	bool bCreateControls;
	CCheckListBox wndTablesList;
	std::string szFileName;
	std::vector<NBitField::SField> fields;
	uint8_t *pData;
	int nSize;

protected:
	int GetMinimumXDimension() { return 204; }
	int GetMinimumYDimension() { return 106; }

	void GetXMLFilePath( std::string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CBinaryBitFieldDialog"; }
	bool IsDrawGripper() { return true; }

	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	enum { IDD = IDD_BIT_FIELD };

	CBinaryBitFieldDialog( const std::string &_szFileName, uint8_t *_pData, const int _nSize, CWnd *pwndParent  );
	~CBinaryBitFieldDialog();

	DECLARE_MESSAGE_MAP()
};
