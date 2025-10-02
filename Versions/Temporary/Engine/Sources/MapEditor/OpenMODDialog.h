#pragma once

#include "ResourceDefines.h"

#include "../MapEditorLib/ResizeDialog.h"
#include "../Main/Mods.h"


class COpenMODDialog : public CResizeDialog
{
	bool bCreateControls;
	CString strDescription;
	CComboBox wndNameComboBox;
	int nMODIndex;
	vector<NMOD::SMOD> modList;

	void UpdateControls();

protected:
	int GetMinimumXDimension() { return 300; }
	int GetMinimumYDimension() { return 150; }
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "COpenMODDialog"; }
	bool IsDrawGripper() { return true; }

	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();

	afx_msg void OnCbnSelchangeNameCombo();

public:
	enum { IDD = IDD_OPEN_MOD };

	COpenMODDialog( CWnd* pParent = NULL );

	bool GetMOD( NMOD::SMOD *pMOD );

	DECLARE_MESSAGE_MAP()
};


