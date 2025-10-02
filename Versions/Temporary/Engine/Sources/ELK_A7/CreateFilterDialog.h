#if !defined(__CREATE_FILTER_DIALOG__)
#define __CREATE_FILTER_DIALOG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ELK_Types.h"
#include "..\MapEditorLib\ResizeDialog.h"
#include "..\MapEditorLib\Tools_Resources.h"

int CALLBACK FiltersCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

class CCreateFilterDialog : public CResizeDialog
{
	friend int CALLBACK FiltersCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
public:
	CCreateFilterDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_CREATE_FILTER };
	CListBox	m_ConditionsList;
	CCheckListBox	m_FoldersList;
	CListCtrl	m_FiltersList;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnColumnclickFiltersList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickFiltersList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedFiltersList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeConditionsList();
	afx_msg void OnSelchangeAllFoldersList();
	afx_msg void OnAddConditionButton();
	afx_msg void OnRemoveConditionButton();
	afx_msg void OnAddFilterButton();
	afx_msg void OnAddFilterMenu();
	afx_msg void OnDeleteFilterButton();
	afx_msg void OnDeleteFilterMenu();
	afx_msg void OnDblclkAllFoldersList();
	afx_msg void OnRenameFilterButton();
	afx_msg void OnRenameFilterMenu();
	afx_msg void OnDblclkFiltersList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangedState();
	afx_msg void OnState0();
	afx_msg void OnState1();
	afx_msg void OnState2();
	afx_msg void OnState3();
	DECLARE_MESSAGE_MAP()

protected:
	const static int vID[];

	virtual int GetMinimumXDimension() { return 450; }
	virtual int GetMinimumYDimension() { return 400; }
	virtual bool SerializeToRegistry() { return true; }
	virtual void GetRegistryKey( string *pszRegistryKey );
	virtual bool IsDrawGripper() { return true; }

	int nSortColumn;
	vector<bool> bFiltersSortParam;
	bool bCreateControls;
	CSimpleFilterItem lastSimpleFilterItem;

	void CreateControls();
	void ClearControls();
	void UpdateControls();
	
	void FillFilters();
	void FillFolders( const CSimpleFilterItem &rSimpleFilterItem );

	void LoadFilterToControls();
	void GetUniqueFilterName( const string &rszNewFilterName, string *pszFullNewFilterName );

public:
	CEnumFolderMap folderMap;
	CFilterMap filterMap;
	string szCurrentFilter;
};

#endif // !defined(__CREATE_FILTER_DIALOG__)

