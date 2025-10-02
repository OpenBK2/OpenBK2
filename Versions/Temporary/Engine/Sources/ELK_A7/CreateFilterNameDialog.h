#if !defined(__CREATE_FILTER_NAME_DIALOG__)
#define __CREATE_FILTER_NAME_DIALOG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Resource.h"
#include "..\MapEditorLib\ResizeDialog.h"


class CCreateFilterNameDialog : public CResizeDialog
{
public:
	CCreateFilterNameDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_CREATE_FILTER_NAME };
	CString	m_Name;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeFilterNameEdit();
	DECLARE_MESSAGE_MAP()

protected:
	const static int vID[];

	virtual int GetMinimumXDimension() { return 200; }
	virtual int GetMinimumYDimension() { return 60; }
	virtual bool SerializeToRegistry() { return true; }
	virtual void GetRegistryKey( string *pszRegistryKey );
	virtual bool IsDrawGripper() { return true; }
	
	void UpdateControls();
public:
};

#endif // !defined(__CREATE_FILTER_NAME_DIALOG__)

