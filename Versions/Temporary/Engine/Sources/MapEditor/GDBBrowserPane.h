#pragma once

#include "ObjectBrowserView.h"
#include "libdb/Manipulator.h"

#include "MapEditorLib/Interface_UserData.h" //CTableSet

#include <cstdint>
#include <list>
#include <string>

// A Game Database pane's contents and what the pane does with them, apart from
// the pane.
//
// CDWGDBBrowser chose the tables the browser shows and kept them in the user
// data, pointed each table's tree at its folder the first time it was shown,
// and answered CHID_MAIN -- New, Open, Close, Save, the recent lists, the MODs,
// Select Tables and Properties. None of that is about the pane being a Stingray
// control bar, and the wx main frame's Game Database panes do all of it the
// same way, so it lives here and each pane holds one. The pane makes the
// browser in its own toolkit, with this as its listener, and hands it over.
class CGDBBrowserContents : public ICommandHandler, public IObjectBrowser::IListener
{
public:
	// What the contents ask of the pane they are in.
	struct IPane
	{
		virtual ~IPane() {}
		// A table's tree was shown: place the browser in the pane again.
		virtual void LayoutContents() = 0;
		// No table is chosen: the pane shows its empty face, not the browser.
		virtual void ShowEmpty( bool bEmpty ) = 0;
		// What the Select Tables dialog opens over.
		virtual IWidget* GetOwner() = 0;
	};

private:
	// Borrowed; the pane holds this.
	IPane *pPane;
	int nGDBBrowserID;
	bool bCreateControls;
	// Owned, once handed over.
	IObjectBrowser *pContents;

	CPtr<IManipulator> pTableManipulator;

	CTableSet selectedTables;
	std::list<std::string> tables;
	std::string szCurrentTable;

	void SetTableManipulator( IManipulator *_pTableManipulator );
	void CreateTabs();
	void SelectTables();
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
	CGDBBrowserContents( IPane *_pPane, int _nGDBBrowserID );
	virtual ~CGDBBrowserContents();

	// Takes the browser the pane made and created with this as its listener,
	// reads the tables the user chose for this pane, and fills it.
	void Start( IObjectBrowser *pBrowser );
	// Before the pane's window goes: stops answering CHID_OBJECT_STORAGE and
	// CHID_MAIN, and keeps the chosen tables in the user data.
	void Stop();

	IObjectBrowser* GetBrowser() const { return pContents; }
	// What answers CHID_OBJECT_STORAGE while this pane has the focus.
	ICommandHandler* GetObjectStorage() { return ( pContents != 0 ) ? pContents->GetObjectStorage() : 0; }
	int GetID() const { return nGDBBrowserID; }
	void EnableEdit( bool bEnable ) { if ( pContents != 0 ) { pContents->EnableEdit( bEnable ); } }

	// IObjectBrowser::IListener
	virtual void OnTableSelected();

	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};
