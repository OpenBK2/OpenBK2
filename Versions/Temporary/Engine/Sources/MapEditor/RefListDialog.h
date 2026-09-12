#pragma once

#include "libdb/Manipulator.h"
#include "ResourceDefines.h"

#include "MapEditorLib/ResizeDialog.h"
#include "RefListView.h"


class CRefListDialog : public CResizeDialog
{
	// The objects, the fields and the clearing are the boundary's, so that the
	// wx dialog does the same things to the database; see RefListView.h.
	typedef NRefList::SReferenceObject SReferenceObject;

	CEdit fieldsCtrl;
	CListCtrl objectsCtrl;
	std::string szTargetTypeName;
	std::string szTargetName;
	std::string szTargetFullName;
	std::vector<SReferenceObject> referenceObjects;
	CPtr<IManipulator> pCurrentManipulator;
	int nSelectedItem;
	std::list<std::string> currentFields;
	std::list<std::string> *pReferenceObjectsList;

	void BuildReferenceObjectsList();
	void BuildFieldsListForObject( const NRefList::SReferenceObject &object );
	
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
	void GetXMLFilePath( std::string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CRefListDialog"; }
	int GetMinimumXDimension() { return 300; }
	int GetMinimumYDimension() { return 125; }
	bool IsDrawGripper() { return true; }

public:
	enum { IDD = IDD_REF_LIST };
	
	CRefListDialog( CWnd* pParent = NULL );

	void SetData( const std::string &szTypeName, const std::string &szName, std::list<std::string> *pReferenceObjectsList );
	
	DECLARE_MESSAGE_MAP()
};


