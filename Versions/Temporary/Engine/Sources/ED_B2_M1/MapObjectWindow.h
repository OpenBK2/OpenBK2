#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "MapObjectMultiState.h"

#include <cstdint>

class CMapObjectWindow : public CResizeDialog, public ICommandHandler
{
	struct SObjectListElement
	{
		string szObjectTypeName;
		CDBID objectDBID;
	};
	typedef hash_map<unsigned, SObjectListElement> CObjectListElementMap;

	bool bCreateControls;
	bool bFull;
	int nStyle;
	//
	CComboBox	wndPalyerComboBox;
	CEdit	wndDirectionEdit;
	CComboBox	wndFilterComboBox;
	CListCtrl	wndObjectList;
	//
	SObjectListElement selectedObjectListElement;
	CObjectListElementMap objectListElementMap;
	//
	int GetSelectedFilterIndex();
	//
	bool GetEditParameters( CMapObjectMultiState::SEditParameters *pEditParameters );
	bool SetEditParameters( const CMapObjectMultiState::SEditParameters &rEditParameters );
	//
	void UpdateObjectsListStyle();
	void SetObjectsListStyle( int nStyle );
	void FillFilterComboBox();
	void FillObjectList();
	//
	void UpdateSelection();
	void ClearSelection();

protected:
	//virtual LRESULT WindowProc( unsigned message, WPARAM wParam, LPARAM lParam );
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	//
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnSize( unsigned nType, int cx, int cy );
	afx_msg void OnSelchangeFilterComboBox();
	afx_msg void OnDirectionRadio();
	afx_msg void OnChangeDirection();
	afx_msg void OnFilterRadio();
	afx_msg void OnItemchangedObjectList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );
	
	//CResizeDialog
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CMapObjectWindow"; }
	bool IsDrawGripper() { return false; }

public:
	static const char FILTER_TYPE[];
	static const char MAPOBJECT_EXTRACTOR_TYPE[];
	static const char SPOT_EXTRACTOR_TYPE[];
	enum
	{ 
		IDD_FULL = IDD_TAB_MI_MAPOBJECT_FULL,
		IDD_NO_BUTTONS = IDD_TAB_MI_MAPOBJECT_NO_BUTTONS,
	};

	CMapObjectWindow( bool _bFull = true, CWnd* pParent = 0 );
	~CMapObjectWindow();

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


