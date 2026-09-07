#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "MapObjectMultiState.h"
#include "MapObjectView.h"

#include <cstdint>

class CMapObjectWindow : public CResizeDialog, public CMapObjectCommands
{
	struct SObjectListElement
	{
		std::string szObjectTypeName;
		CDBID objectDBID;
	};
	typedef std::unordered_map<unsigned, SObjectListElement> CObjectListElementMap;

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
	void GetXMLFilePath( std::string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CMapObjectWindow"; }
	bool IsDrawGripper() { return false; }

public:
	static const char MAPOBJECT_EXTRACTOR_TYPE[];
	static const char SPOT_EXTRACTOR_TYPE[];
	enum
	{ 
		IDD_FULL = IDD_TAB_MI_MAPOBJECT_FULL,
		IDD_NO_BUTTONS = IDD_TAB_MI_MAPOBJECT_NO_BUTTONS,
	};

	CMapObjectWindow( bool _bFull = true, CWnd* pParent = 0 );
	~CMapObjectWindow();

	//	CMapObjectCommands
	virtual bool GetEditParameters( CMapObjectMultiState::SEditParameters *pEditParameters );
	virtual bool SetEditParameters( const CMapObjectMultiState::SEditParameters &rEditParameters );

	// ICommandHandler
	//
	// Overridden rather than inherited: this palette answers the object storage
	// query, the clear-selection command and the object list's three context
	// menu items as well, and falls through to CMapObjectCommands for the
	// edit-parameter pair.
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


