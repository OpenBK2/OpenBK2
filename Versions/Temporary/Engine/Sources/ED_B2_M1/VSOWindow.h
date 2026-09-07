#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "VSOMultiState.h"
#include "VSOView.h"

#include <cstdint>

class CVSOWindow : public CResizeDialog, public CVSOCommands
{
public:
	struct SObjectListElement
	{
		std::string szObjectTypeName;
		CDBID objectDBID;
	};
	typedef std::unordered_map<int, SObjectListElement> CObjectListElementMap;
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
	afx_msg void OnSize( unsigned nType, int cx, int cy );
	afx_msg void OnPointNumberRadio();
	afx_msg void OnStatsTypeRadio();
	afx_msg void OnChangeWidth();
	afx_msg void OnChangeOpacity();
	afx_msg void OnSelchangeFilterComboBox();
	afx_msg void OnItemchangedObjectList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );

	//CResizeDialog
	void GetXMLFilePath( std::string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CVSOWindow"; }
	bool IsDrawGripper() { return false; }

public:
	static const char EXTRACTOR_TYPE[];
	enum { IDD = IDD_TAB_MI_VSO };

	CVSOWindow( CWnd* pParent = 0 );
	~CVSOWindow();

	//	CVSOCommands
	virtual bool GetEditParameters( CVSOMultiState::SEditParameters *pEditParameters );
	virtual bool SetEditParameters( const CVSOMultiState::SEditParameters &rEditParameters );

	// ICommandHandler
	//
	// Overridden rather than inherited: this palette answers the object storage
	// query, clear selection, enable height and the object list's three context
	// menu items as well, and falls through to CVSOCommands for the
	// edit-parameter pair.
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


