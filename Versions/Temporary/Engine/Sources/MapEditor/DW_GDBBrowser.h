#pragma once

//#include "3DTab_GDBBrowser.h"
#include "ComboBox_GDBBrowser.h"
#include "Empty_GDBBrowser.h"

#include "MapEditorLib/Interface_UserData.h" //CTableSet

#include <cstdint>

class CDWGDBBrowser : public SECControlBar, public ICommandHandler
{
	int nGDBBrowserID;
	bool bCreateControls;

	//C3DTabGDBBrowser wndContents;
	CComboBoxGDBBrowser wndContents;
	CEmptyGDBBrowser wndEmptyContents;

	CPtr<IManipulator> pTableManipulator;

	CTableSet selectedTables;
	list<string> tables;
	string szCurrentTable;

	void SetTableManipulator( IManipulator *_pTableManipulator );
	virtual BOOL OnGripperClose();

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnDestroy();
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnRButtonUp( UINT nFlags, CPoint point );
	afx_msg LRESULT OnTabSelected( WPARAM wParam, LPARAM lParam );
	afx_msg void OnTabSelected();

	//virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam) ;

	void CreateTabs();
	void SelectTables();
	void ClearTable();
	void SelectObjectSet( const SObjectSet &rObjectSet );
	//
	void New( const string &rszObjectTypeName );
	void Open( const string &rszObjectTypeName );
	void OnRecentList( int nIndex, bool bMainObject );
	//
	void OnCheckOut();
	void OnCheckIn();
	void OnGetLatest();
	void LocateObject();

public:
	CDWGDBBrowser( int _nGDBBrowserID );
	virtual ~CDWGDBBrowser();

	CComboBoxGDBBrowser *GetContents() { return &wndContents; }
	int GetDWGDBBrowserID() const { return nGDBBrowserID; }
	void EnableEdit( bool bEnable ) { wndContents.EnableEdit( bEnable ); }

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, uint32_t dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


