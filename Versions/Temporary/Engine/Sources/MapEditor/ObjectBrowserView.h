#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Widget.h"
#include "Misc/Geom.h"

#include <string>

struct IView;

// The database browser's contents -- a list of tables over a tree of each
// table's folders and objects -- behind a boundary that names no toolkit.
//
// The Game Database pane holds it (GDBBrowserPane.cpp). In the MFC editor the
// pane was CDWGDBBrowser, holding a CComboBoxGDBBrowser by value: a combo box of
// the tables the user picked and a CTreeGDBBrowser for each, only the chosen
// table's tree shown; all of those are gone now. The pane adds the tables, switches between
// them and points a tree at its table's folder the first time it is shown; the
// tree does everything else itself, down to handing its selection to the
// Selection Properties pane through CHID_PC_DIALOG. So the boundary is two
// interfaces -- the contents, and one table's tree with the few things the pane
// asks of it.
//
// The link picker, CPCDBLinkDialog, holds the same contents with trees that
// pick an object rather than open it: EKind.

// One table's tree.
struct IObjectTree
{
	virtual ~IObjectTree() {}

	// Whether the tree has been given its table's folder yet.
	virtual bool IsTreeCreated() = 0;
	// The tree as a view: what a folder manipulator is set on.
	virtual IView* GetView() = 0;
	// Fills the tree from the folder.
	virtual void CreateTree() = 0;
	// Hands the selected objects to the properties view that
	// SetPCDialogCommandHandlerID names, and has it build its tree when bUpdate
	// is set.
	virtual void UpdateSelectionManipulator( bool bUpdate ) = 0;
	virtual void SetPCDialogCommandHandlerID( unsigned nPCDialogCommandHandlerID, bool bUpdate ) = 0;
	// The selected object's name, when exactly one row is selected.
	virtual bool GetCurrentTreeItemName( std::string *pszName ) = 0;
	// Selects an object by name, or only remembers it for the table when
	// bUpdateSelection is false.
	virtual bool SetCurrentTreeItemName( const std::string &rszName, bool bUpdateSelection ) = 0;
	// Makes the next UpdateSelectionManipulator select the object remembered
	// for the table rather than keep what is selected: how locating an object
	// brings the browser to it.
	virtual void SetStrongSelection() = 0;
};


struct IObjectBrowser
{
	enum EKind
	{
		// Loading an object opens it in its editor: the browser panes.
		KIND_BROWSER,
		// Loading an object picks it: the link picker.
		KIND_LINK,
	};

	// What the contents tell the window they are in, where the MFC combo box and
	// trees sent it WM_GDB_BROWSER and WM_TREE_GDB_BROWSER.
	struct IListener
	{
		virtual ~IListener() {}
		// Another table was chosen.
		virtual void OnTableSelected() = 0;
		// The selection in pTree changed, and was handed on. The browser pane has
		// no use for this; the link picker shows the selected object.
		virtual void OnTreeSelectionChanged( IObjectTree *pTree ) {}
		// An object in pTree was loaded, by a double click or the Load command --
		// after it was opened, for a browser pane's tree. The link picker takes
		// it as OK.
		virtual void OnTreeLoad( IObjectTree *pTree ) {}
	};

	virtual ~IObjectBrowser() {}

	// Made by NObjectBrowser::CreateWxIn, below; Create and SetBounds were for
	// the MFC pane and went with it.
	virtual void Show( bool bShow ) = 0;
	virtual void EnableEdit( bool bEnable ) = 0;

	virtual void RemoveAllTables() = 0;
	// A table and its tree, not filled yet. Null on failure.
	virtual IObjectTree* AddTable( const std::string &rszTableName ) = 0;
	virtual int GetTableCount() = 0;
	virtual IObjectTree* GetTable( int nIndex ) = 0;
	virtual IObjectTree* GetTable( const std::string &rszTableName ) = 0;
	// Chooses pTree's table, which tells the listener.
	virtual bool ActivateTable( IObjectTree *pTree ) = 0;
	// The chosen table's tree, or null.
	virtual IObjectTree* GetActiveTable() = 0;
	virtual bool GetActiveTableName( std::string *pszTableName ) = 0;
	// Shows the chosen table's tree and hides the others.
	virtual void ShowActiveTable() = 0;

	// What answers CHID_OBJECT_STORAGE for this browser: the objects selected in
	// the chosen table, which the editor's placing tools read.
	virtual ICommandHandler* GetObjectStorage() = 0;
};




class wxWindow;

namespace NObjectBrowser
{
	// The wx contents made straight inside a wx window, for a wx dialog -- the
	// link picker -- or a wx frame's pane, rather than inside an MFC pane.
	// *ppWindow is what to put in the layout, which then does what SetBounds
	// does for an MFC pane; Show shows and hides it. pOwner is what the trees'
	// dialogs open over, and must outlive the contents. nGDBBrowserID is a
	// browser pane's, which its trees give the frame when they take the focus;
	// -1 for a dialog. Owned by the caller; null on failure.
	IObjectBrowser* CreateWxIn( wxWindow *pParent, IWidget *pOwner, IObjectBrowser::IListener *pListener,
															IObjectBrowser::EKind eKind, wxWindow **ppWindow, int nGDBBrowserID = -1 );
}

