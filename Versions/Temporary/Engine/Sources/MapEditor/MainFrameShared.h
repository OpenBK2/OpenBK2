#pragma once

#include "MapEditorLib/FocusMemory.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Widget.h"
#include "ProgressView.h"

#include <functional>
#include <string>

// What the main frame does that is not drawing a frame.
//
// CMainFrame grew the editor's title bar bookkeeping, the save-changes prompt,
// opening a dropped or passed resource, the link and object pickers, registering
// an XDB, help and the progress dialog. None of that is about the frame being
// MFC, and the wx frame (MainFrameWx.cpp) has to do all of it the same way, so
// it lives here once and both frames call it. What stays in each frame is
// windows: menus, panes, status bar, placement.
//
// Dialogs and message boxes opened from here belong to the main window as
// IMainFrameContainer answers it, which is the frame either way.
namespace NMainFrameShared
{
	// SetWindowTitle's bookkeeping. Folds the fields rRequest names into
	// *pCurrent and, when that changed anything, puts the caption to show in
	// *pszTitle and answers true. False means the caption stays as it is.
	bool UpdateTitle( SSWTParams *pCurrent, const SSWTParams &rRequest, std::string *pszTitle );

	// ID_VIEW_SAVE_CHANGES: asks, saves or throws away, and updates the title's
	// modified mark. False when the user cancelled. rReloadData is the frame's
	// own reload, run after changes are thrown away.
	bool SaveChanges( bool bShowConfirmDialog, const std::function<void()> &rReloadData );

	// A resource from the command line, another instance or a dropped file.
	void OpenResource( const std::string &rszResourceName );

	// IMainFrame::BrowseLink and BrowseForObject.
	bool BrowseLink( std::string *pszResult, const std::string &rszInitialValue, const SPropertyDesc* pPropertyDesc, bool bMultiRef, bool bEnableEdit );
	bool BrowseForObject( CDBID *pObjectDBID, std::string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty );

	// File > Register XDB. rReloadData as for SaveChanges.
	void RegisterXDB( const std::function<void()> &rReloadData );

	// Help > Contents and Help > About.
	std::string GetHelpFilePath();
	bool HasHelpFile( const std::string &rszHelpFilePath );
	void ShowHelpContents( const std::string &rszHelpFilePath );
	void ShowAbout();

	// The recent list a menu command heads, and the first id of its menu: set
	// for ID_MAIN_RECENT_0 and ID_MAIN_RECENT_RESOURCE_0, which is the item a
	// recent menu is rebuilt around. Null for every other command.
	const SUserData::CRecentList* GetRecentList( unsigned nCommandID, unsigned *pnFirstID );
	// The one item a recent menu shows when its list is empty.
	std::string GetRecentEmptyLabel();

	// A command in ID_FIRST_COMMAND_ID..ID_LAST_COMMAND_ID from a menu or an
	// accelerator: run when the handler that owns it says it is enabled.
	void RunUserCommand( unsigned nCommandID );
	// Its menu state. The recent menus' heads are the frame's to rebuild and
	// only get their enabled state here.
	void UpdateUserCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// The progress dialog behind IMainFrame's seven progress calls. Which
	// toolkit draws it is NProgressView's business.
	class CProgressHost
	{
		// Made on first use and owned.
		NProgressView::IView *pView;
		// Where the focus goes back to when the dialog closes.
		CFocusMemory previousFocus;

	public:
		CProgressHost() : pView( 0 ) {}
		~CProgressHost();

		// pOwner is the frame, and has to outlive the dialog.
		void Create( IWidget *pOwner );
		void Destroy();
		void SetTitle( const std::string &rszTitle );
		void SetMessage( const std::string &rszMessage );
		void SetRange( int nStart, int nFinish );
		void SetPosition( int nPosition );
		void IteratePosition();
	};
}
