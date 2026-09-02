#pragma once

#include "libdb/Manipulator.h"
#include "ResourceDefines.h"

#include "MapEditorLib/ResizeDialog.h"

struct IResourceManager;

class CRefListWaitDialog : public CResizeDialog
{

	std::string szTargetTypeName;
	std::string szTargetName;
	std::list<std::string> *pResultList;
	IResourceManager *pResourceManager;
	bool bComplete;
	unsigned nTimer;
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//
	afx_msg void OnDestroy();
	afx_msg void OnTimer( UINT_PTR nIDEvent );

	//
	// CResizeDialog
	void GetXMLFilePath( std::string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CRefListWaitDialog"; }
	int GetMinimumXDimension() { return 200; }
	int GetMinimumYDimension() { return 50; }
	bool IsDrawGripper() { return true; }

public:
	enum { IDD = IDD_REF_LIST_WAIT };

	CRefListWaitDialog( CWnd* pParent );

	void SetData( std::list<std::string> *pResult, const std::string &szTypeName, const std::string &szName, IResourceManager *_pResourceManager ) 
		{ pResultList = pResult; szTargetTypeName = szTypeName; szTargetName = szName; pResourceManager = _pResourceManager; }
	bool IsComplete() { return bComplete; }

	DECLARE_MESSAGE_MAP()
};



