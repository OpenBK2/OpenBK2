#pragma once

// The wx main frame's docking panes. Front-end private: only MainFrameWx.cpp
// includes this.
//
// Two kinds, because what goes in them differs.
//
// The frame's own three -- Game Database, Selection Properties and Log -- hold
// the wx views, made straight inside a wx panel, around the contents CMainFrame's
// panes share with these: CGDBBrowserContents, the property pane, and
// CLogPaneContents. They are wx all the way down.
//
// The panes the editors ask for through IMainFrame::CreateControlBar are
// filled by the editors, and the editors make MFC windows in them -- a
// Stingray shortcut bar, or a CWxHostWindow around their wx contents -- as
// children of ToCWnd( pane ). So those panes are a wx panel with a CWnd
// subclassed over its handle: the editors get the CWnd they expect, and MFC's
// handling of what a child sends its parent, reflected notifications included,
// still runs before wx's.

#ifdef OBK2_WITH_WX

#include "GDBBrowserPane.h"
#include "LogPane.h"
#include "PropertyPaneView.h"
#include "MapEditorLib/Interface_Widget.h"

#include <wx/aui/auibar.h>
#include <wx/aui/framemanager.h>
#include <wx/bitmap.h>
#include <wx/panel.h>
#include <wx/weakref.h>

#include <map>
#include <string>

namespace NMainFrameWxPanes
{
	// The frame's wxAUI manager, with one thing wxAuiManager keeps to itself:
	// which pane's own frame is where. A Stingray control bar answered right
	// clicks on its caption and border; wxAUI draws those on the managed window
	// and does nothing with a right click there.
	class CAuiManager : public wxAuiManager
	{
		bool bUpdatePending = false;

	public:
		// The window of the pane whose caption, gripper or border is at rPoint,
		// in the managed window's client coordinates. Null anywhere else,
		// the pane's buttons and its own window included.
		wxWindow* PaneFrameAt( const wxPoint &rPoint );

		// Update once what is being done now has returned, however many times it
		// is asked for meanwhile: CFrameWnd::ShowControlBar's bDelay, which is how
		// the MFC frame shows and hides the editors' panes and toolbars. Update
		// lays the frame out and paints it at once, and an editor hides its
		// toolbars in the middle of taking itself apart -- in the height state,
		// after the scene has let go of the terrain and before the state has, a
		// paint then asked the scene for a terrain height and ended the process.
		void UpdateLater();
	};


	// MFC's docking arguments as a wxAUI pane: AFX_IDW_DOCKBAR_* as the side, the
	// width across the side, and fRate as the share of the side the pane takes
	// among the others docked there.
	wxAuiPaneInfo DockedPaneInfo( const wxString &rName, const std::string &rszTitle, unsigned nPlace, float fRate, int nWidth );


	// A wx panel that MFC windows can be children of.
	class CMfcPanel : public wxPanel
	{
		CWnd mfcWindow;
		// The editor's contents window, kept the panel's size as
		// CDefaultDockingWindow kept it its inside's.
		HWND hwndContents;

		void OnSize( wxSizeEvent &rEvent );

	public:
		explicit CMfcPanel( wxWindow *pParent );
		virtual ~CMfcPanel();

		CWnd* GetMfcWindow() { return &mfcWindow; }
		void SetContents( HWND _hwndContents );
		void FitContents();
	};


	// The document window IMainFrame::CreateChildFrame makes: a CMfcPanel
	// filling the frame's workspace, as a maximised MDI child fills the MDI
	// client. The editor makes its view in it as CChildFrameBase always has, a
	// child of ToCWnd( frame window ).
	class CFrameWindow : public IFrameWindow
	{
		wxWeakRef<CMfcPanel> pPanel;

	public:
		explicit CFrameWindow( CMfcPanel *_pPanel ) : pPanel( _pPanel ) {}

		CMfcPanel* GetPanel() const { return pPanel; }

		// IFrameWindow. Maximize has nothing to do: the panel always fills the
		// workspace. Focus goes to the panel, as SetFocus on an MDI child left it
		// on the frame rather than its contents.
		virtual void* GetNativeWidget();
		virtual void Show( bool bShow );
		virtual void Maximize() {}
		virtual void Focus();
		virtual void Destroy();
	};


	// The IDockPanel an editor gets: a CMfcPanel in the frame's wxAUI manager.
	class CDockPanel : public IDockPanel
	{
		CAuiManager *pManager;
		wxWeakRef<CMfcPanel> pPanel;
		// The frame's: whether the manager has laid the frame out yet. Until it
		// has, a layout would size the docks against the frame's size before it
		// is placed, and wxAUI keeps the sizes it gives a dock the first time.
		const bool *pbLaidOut;

	public:
		CDockPanel( CAuiManager *_pManager, CMfcPanel *_pPanel, const bool *_pbLaidOut )
			: pManager( _pManager ), pPanel( _pPanel ), pbLaidOut( _pbLaidOut ) {}

		CMfcPanel* GetPanel() const { return pPanel; }

		// IDockPanel. Show tells the manager, and the manager lays the frame out
		// again once the caller has returned (CAuiManager::UpdateLater);
		// ShowWithoutLayout only shows or hides the panel's window, and the
		// manager's next layout has the last word -- as a hide ShowControlBar
		// delays had over a later ShowWindow on the bar, which is what the editors
		// do when they make their panes.
		virtual void* GetNativeWidget();
		virtual void Show( bool bShow );
		virtual void ShowWithoutLayout( bool bShow );
		virtual bool IsVisible() const;
		virtual bool IsAlive() const;
		virtual void Destroy();
		virtual void Redraw();
	};


	// Every toolbar button picture the frame knows, by command -- the pool
	// SECToolBarManager kept. A toolbar asks for its buttons' pictures by
	// command, whichever resource they came from.
	class CToolBarImages
	{
		std::map<unsigned, wxBitmap> bitmaps;

	public:
		// A TOOLBAR resource and the BITMAP of the same id, in the module the
		// resource handle finds them: the nth button that is not a separator is
		// the nth square of the strip. Light grey is transparent, as in every
		// MFC toolbar bitmap.
		bool AddToolBarResource( unsigned nResourceID );
		// An icon for one command, over whatever picture a toolbar resource gave
		// it, as SECToolBarManager::AddCommandIconResource.
		void AddIcon( unsigned nCommandID, unsigned nIconID );
		// Null when no resource gave the command a picture.
		wxBitmap Get( unsigned nCommandID ) const;
	};


	// The IToolBar an editor gets: a wxAuiToolBar pane of the frame's manager.
	class CToolBar : public IToolBar
	{
		CAuiManager *pManager;
		wxWeakRef<wxAuiToolBar> pToolBar;
		// As CDockPanel's.
		const bool *pbLaidOut;

	public:
		CToolBar( CAuiManager *_pManager, wxAuiToolBar *_pToolBar, const bool *_pbLaidOut )
			: pManager( _pManager ), pToolBar( _pToolBar ), pbLaidOut( _pbLaidOut ) {}

		wxAuiToolBar* GetToolBar() const { return pToolBar; }

		// IToolBar
		virtual void Show( bool bShow );
		virtual bool IsVisible() const;
	};


	// Log: CLogPaneContents around the wx log view.
	class CLogPane
	{
		CLogPaneContents contents;
		wxWeakRef<wxPanel> pPanel;

	public:
		// Creates the pane in pFrame and adds it to pManager.
		bool Create( wxWindow *pFrame, wxAuiManager *pManager );
		wxPanel* GetPanel() const { return pPanel; }
		CLogPaneContents& GetContents() { return contents; }
	};


	// Selection Properties: the wx property pane.
	class CPropertiesPane
	{
		// Owned.
		IPropertyPane *pPropertyPane;
		wxWeakRef<wxPanel> pPanel;

	public:
		CPropertiesPane() : pPropertyPane( 0 ) {}
		~CPropertiesPane();

		// pOwner is what the grid's buttons open their dialogs over.
		bool Create( wxWindow *pFrame, wxAuiManager *pManager, IWidget *pOwner );
		wxPanel* GetPanel() const { return pPanel; }
		void EnableEdit( bool bEnable );
	};


	// One Game Database pane: CGDBBrowserContents around the wx object browser,
	// over an empty face shown while no table is chosen.
	class CGDBBrowserPane : public CGDBBrowserContents::IPane
	{
		CGDBBrowserContents contents;
		IWidget *pOwner;
		wxWeakRef<wxPanel> pPanel;
		wxWeakRef<wxPanel> pEmpty;

	public:
		CGDBBrowserPane( int nGDBBrowserID, IWidget *_pOwner ) : contents( this, nGDBBrowserID ), pOwner( _pOwner ) {}

		// Creates the pane in pFrame, adds it to pManager captioned for its place
		// in the list, and fills it.
		bool Create( wxWindow *pFrame, wxAuiManager *pManager, int nWindowIndex );
		wxPanel* GetPanel() const { return pPanel; }
		CGDBBrowserContents& GetContents() { return contents; }

		// CGDBBrowserContents::IPane
		virtual void LayoutContents();
		virtual void ShowEmpty( bool bEmpty );
		virtual IWidget* GetOwner() { return pOwner; }
	};
}

#endif // OBK2_WITH_WX
