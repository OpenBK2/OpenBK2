#if !defined(__MAPINFO_TAB_VSO__)
#define __MAPINFO_TAB_VSO__
#pragma once

#include "../MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "VSOMultiState.h"

class CVSOWindow : public CResizeDialog, public ICommandHandler
{
public:
	struct SObjectListElement
	{
		string szObjectTypeName;
		CDBID objectDBID;
	};
	typedef hash_map<int, SObjectListElement> CObjectListElementMap;
	//
	bool bCreateControls;
	bool bEnableHeight;
	int nStyle;
	//
	CComboBox	wndFilterComboBox;
	CListCtrl	wndObjectList;
	//
	SObjectListElement selectedObjectListElement;
	CObjectListElementMap objectListElementMap;
	//
	int GetSelectedFilterIndex();
	//
	bool GetEditParameters( CVSOMultiState::SEditParameters *pEditParameters );
	bool SetEditParameters( const CVSOMultiState::SEditParameters &rEditParameters );
	//
	void UpdateObjectsListStyle();
	void SetObjectsListStyle( int nStyle );
	void FillFilterComboBox();
	void FillObjectList();
	//
	void UpdateSelection();
	void ClearSelection();
	void EnableHeight( bool bEnableHeight );

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnPointNumberRadio();
	afx_msg void OnStatsTypeRadio();
	afx_msg void OnChangeWidth();
	afx_msg void OnChangeOpacity();
	afx_msg void OnSelchangeFilterComboBox();
	afx_msg void OnItemchangedObjectList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );

	//CResizeDialog
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CVSOWindow"; }
	bool IsDrawGripper() { return false; }

public:
	static const char FILTER_TYPE[];
	static const char EXTRACTOR_TYPE[];
	enum { IDD = IDD_TAB_MI_VSO };

	CVSOWindow( CWnd* pParent = 0 );
	~CVSOWindow();

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__MAPINFO_TAB_VSO__)

