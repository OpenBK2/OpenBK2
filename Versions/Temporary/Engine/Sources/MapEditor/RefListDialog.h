#if !defined(__COMMON_CONTROLS__REFLIST_DIALOG__)
#define __COMMON_CONTROLS__REFLIST_DIALOG__
#pragma once

#include "../libdb/Manipulator.h"
#include "ResourceDefines.h"

#include "..\MapEditorLib\ResizeDialog.h"


class CRefListDialog : public CResizeDialog
{
	struct SReferenceObject
	{
		string szTypeName;
		string szObjectName;
	};

	CEdit fieldsCtrl;
	CListCtrl objectsCtrl;
	string szTargetTypeName;
	string szTargetName;
	string szTargetFullName;
	vector<SReferenceObject> referenceObjects;
	CPtr<IManipulator> pCurrentManipulator;
	int nSelectedItem;
	list<string> currentFields;
	list<string> *pReferenceObjectsList;

	void BuildReferenceObjectsList();
	void BuildFieldsListForObject( const SReferenceObject &object );
	
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//
	afx_msg void OnDestroy();
	afx_msg void OnItemChanged( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnSetEmptyCurrent();
	afx_msg void OnClearAll();
	//
	// CResizeDialog
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CRefListDialog"; }
	int GetMinimumXDimension() { return 300; }
	int GetMinimumYDimension() { return 125; }
	bool IsDrawGripper() { return true; }

public:
	enum { IDD = IDD_REF_LIST };
	
	CRefListDialog( CWnd* pParent = NULL );

	void SetData( const string &szTypeName, const string &szName, list<string> *pReferenceObjectsList );
	
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__REFLIST_DIALOG__)
