#pragma once

//#include "3DTab_GDBBrowser.h"
#include "ObjectBrowserView.h"
#include "Empty_GDBBrowser.h"
#include "libdb/Manipulator.h"

#include "MapEditorLib/Interface_UserData.h" //CTableSet

#include <cstdint>

class CDWGDBBrowser : public SECControlBar, public ICommandHandler, public IObjectBrowser::IListener
{
	int nGDBBrowserID;
	bool bCreateControls;

	//C3DTabGDBBrowser wndContents;
	// The tables and their trees, NObjectBrowser's; owned.
	IObjectBrowser *pContents;
	CEmptyGDBBrowser wndEmptyContents;

	CPtr<IManipulator> pTableManipulator;

	CTableSet selectedTables;
	std::list<std::string> tables;
	std::string szCurrentTable;

	void SetTableManipulator( IManipulator *_pTableManipulator );
	virtual BOOL OnGripperClose();

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnDestroy();
	afx_msg void OnSize( unsigned nType, int cx, int cy );
	afx_msg void OnLButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonUp( unsigned nFlags, CPoint point );
	afx_msg LRESULT OnTabSelected( WPARAM wParam, LPARAM lParam );
	afx_msg void OnTabSelected();

	//virtual LRESULT WindowProc( unsigned message, WPARAM wParam, LPARAM lParam) ;

	void CreateTabs();
	void SelectTables();
	void ClearTable();
	void SelectObjectSet( const SObjectSet &rObjectSet );
	//
	void New( const std::string &rszObjectTypeName );
	void Open( const std::string &rszObjectTypeName );
	void OnRecentList( int nIndex, bool bMainObject );
	//
	void OnCheckOut();
	void OnCheckIn();
	void OnGetLatest();
	void LocateObject();

public:
	CDWGDBBrowser( int _nGDBBrowserID );
	virtual ~CDWGDBBrowser();

	// What answers CHID_OBJECT_STORAGE while this browser has the focus.
	ICommandHandler *GetContents() { return ( pContents != 0 ) ? pContents->GetObjectStorage() : 0; }
	int GetDWGDBBrowserID() const { return nGDBBrowserID; }
	void EnableEdit( bool bEnable ) { if ( pContents != 0 ) { pContents->EnableEdit( bEnable ); } }

	// IObjectBrowser::IListener
	virtual void OnTableSelected() { OnTabSelected(); }

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


