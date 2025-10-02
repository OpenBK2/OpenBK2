#pragma once

#include "../libdb/Manipulator.h"
#include "ResourceDefines.h"

#include "../MapEditorLib/ResizeDialog.h"

interface IResourceManager;

class CRefListWaitDialog : public CResizeDialog
{

	string szTargetTypeName;
	string szTargetName;
	list<string> *pResultList;
	IResourceManager *pResourceManager;
	bool bComplete;
	UINT nTimer;
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//
	afx_msg void OnDestroy();
	afx_msg void OnTimer( UINT nIDEvent );

	//
	// CResizeDialog
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CRefListWaitDialog"; }
	int GetMinimumXDimension() { return 200; }
	int GetMinimumYDimension() { return 50; }
	bool IsDrawGripper() { return true; }

public:
	enum { IDD = IDD_REF_LIST_WAIT };

	CRefListWaitDialog( CWnd* pParent );

	void SetData( list<string> *pResult, const string &szTypeName, const string &szName, IResourceManager *_pResourceManager ) 
		{ pResultList = pResult; szTargetTypeName = szTypeName; szTargetName = szName; pResourceManager = _pResourceManager; }
	bool IsComplete() { return bComplete; }

	DECLARE_MESSAGE_MAP()
};



