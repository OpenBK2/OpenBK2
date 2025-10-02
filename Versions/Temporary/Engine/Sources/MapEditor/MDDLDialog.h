#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/Interface_Controller.h"

class CMDDLDialog : public CDialog
{
	CListBox wndValueList;
	
	UINT nCommandID;
	CDescriptionList valueList;

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();

	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnSelChange();
	afx_msg void OnListKillFocus();

public:
	enum { IDD = IDD_MENU_DROP_DOWN_LIST };

	CMDDLDialog();
	void SetParams( UINT _nCommandID, const CDescriptionList &rValueList );
	void CreateList();
	DECLARE_MESSAGE_MAP()
};


