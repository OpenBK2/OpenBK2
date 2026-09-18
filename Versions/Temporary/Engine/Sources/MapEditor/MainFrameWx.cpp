#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "MainFrameWx.h"

#include <cstdlib>

namespace NMainFrameWx
{
	bool IsWanted()
	{
#ifdef OBK2_WITH_WX
		return NToolkit::UseWxFrame();
#else
		return false;
#endif
	}
}


#ifndef OBK2_WITH_WX

namespace NMainFrameWx
{
	bool Create()
	{
		return false;
	}

	void Show()
	{
	}
}

#else // OBK2_WITH_WX

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_ChildFrame.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/MapEditorModule.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxOwnership.h"

#include "MainFrame.h"
#include "MainFrameParams.h"
#include "MainFrameShared.h"
#include "MainFrameWxPanes.h"
#include "MapEditorApp.h"
#include "MapEditorSingleton.h"
#include "ResourceDefines.h"

#include <fmt/printf.h>

#include <wx/aui/auibar.h>
#include <wx/aui/barartmsw.h>
#include <wx/aui/framemanager.h>
#include <wx/dcclient.h>
#include <wx/iconbndl.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/statusbr.h>
#include <wx/weakref.h>

#include <climits>
#include <list>
#include <map>
#include <memory>
#include <vector>

// The main frame, in wx: the frame itself, its menus, status bar, title,
// command routing, placement and close, its docking panes in wxAUI -- its own
// three and the editors' -- the document window and the toolbars (see
// MainFrameWxPanes.h), and their layout, kept between sessions and put back
// by Reset GUI. Customize, Stingray's toolbar editor, has no counterpart.
//
// What stays MFC for now, on purpose. The editors and dialogs still want the
// main window as a CWnd -- AfxGetMainWnd(), MainFrameWnd(), a CDialog's owner --
// so the frame's handle is attached to one and that is the MFC application's
// main window. Attached, not subclassed: MFC's handle map knows the window, and
// its window procedure is still wx's alone.
//
// Where this differs from CMainFrame in ways that show:
//
//   * menu shortcuts are live. A wx menu item's label after a tab is a shortcut
//     wx registers for the frame; MFC's menu bar only displays that text, and
//     the editor's own windows translate IDA_MAIN for themselves.

namespace
{
	// MainFrame_Consts.cpp's status bar: the message pane, which stretches, and
	// two the editors write coordinates and counts into, 500 and 200 pixels of
	// text.
	const int N_STATUSBAR_PANES = 3;
	const int STATUSBAR_TEXT_WIDTHS[N_STATUSBAR_PANES] = { -1, 500, 200 };

	// barstat.cpp's CX_PANE_BORDER: what MFC adds to a pane's text width.
	const int N_MFC_PANE_BORDER = 6;
	// What wx adds instead (wxStatusBar::MSWGetMetrics, private): a text margin
	// of 8 with themes, which every Windows this runs on has, and the size
	// grip's 20 on the last pane.
	const int N_WX_TEXT_MARGIN = 8;
	const int N_WX_GRIP_WIDTH = 20;


	// The status bar CMainFrame has, measured the way MFC measures one.
	class CClassicStatusBar : public wxStatusBar
	{
	public:
		CClassicStatusBar( wxWindow *pParent, wxWindowID id, long nStyle, const wxString &rName )
			: wxStatusBar( pParent, id, nStyle, rName )
		{
			// wxStatusBar's constructor already asked for the best size and wx
			// kept the answer, which was wxStatusBar's own: this class's
			// DoGetBestSize does not exist yet while a base constructor runs.
			// Measured: the bar kept a best height of 23 until this was added.
			InvalidateBestSize();
		}

		// Panes the widths CStatusBar::UpdateAllPanes gives these text widths.
		// The space between panes is added by both, so it cancels out.
		void SetClassicWidths( int nCount, const int *pnTextWidths )
		{
			std::vector<int> widths( pnTextWidths, pnTextWidths + nCount );
			for ( int nIndex = 0; nIndex < nCount; ++nIndex )
			{
				if ( widths[nIndex] < 0 )
				{
					continue;
				}
				widths[nIndex] += N_MFC_PANE_BORDER - N_WX_TEXT_MARGIN;
				if ( nIndex == nCount - 1 )
				{
					widths[nIndex] -= N_WX_GRIP_WIDTH;
				}
			}
			SetStatusWidths( nCount, widths.data() );
		}

	protected:
		// CStatusBar::CalcFixedLayout's height: the font's height less its
		// internal leading and a pixel, the control's vertical border twice, a
		// system border above and below, and the control bar's two-pixel top
		// border. wx's own is the font's height and four borders, which came out
		// four pixels taller.
		virtual wxSize DoGetBestSize() const override
		{
			wxSize size = wxStatusBar::DoGetBestSize();
			wxClientDC dc( const_cast<CClassicStatusBar*>( this ) );
			dc.SetFont( GetFont() );
			const wxFontMetrics metrics = dc.GetFontMetrics();
			size.y = metrics.height - metrics.internalLeading - 1 + GetBorderY() * 2 + ::GetSystemMetrics( SM_CYBORDER ) * 2 + 2;
			return size;
		}
	};


	void NotYet( const char *pszWhat )
	{
		DebugTrace( "wx main frame: %s is not in this frame yet", pszWhat );
	}


	// A Win32 command id as a wx menu item's id. The editor's own ids go in as
	// they are. MFC's -- ID_APP_EXIT is 0xE141 -- fit neither of the ranges wx
	// accepts for a menu item: a positive id has to fit a short, and the
	// negative short 0xE141 turns into lands among the ids wx generates, which
	// it refuses unless it reserved them itself. So each of those gets an id of
	// its own, counted up from above every id the editor defines, and is mapped
	// back when chosen.
	class CCommandIDs
	{
		static const int N_FIRST_OWN_ID = 32000;
		std::map<unsigned, int> wxIDs;
		std::map<int, unsigned> commandIDs;

	public:
		int ToWx( unsigned nCommandID )
		{
			if ( nCommandID <= static_cast<unsigned>( SHRT_MAX ) )
			{
				return static_cast<int>( nCommandID );
			}
			const std::map<unsigned, int>::const_iterator posWxID = wxIDs.find( nCommandID );
			if ( posWxID != wxIDs.end() )
			{
				return posWxID->second;
			}
			const int nWxID = N_FIRST_OWN_ID + static_cast<int>( wxIDs.size() );
			wxIDs[nCommandID] = nWxID;
			commandIDs[nWxID] = nCommandID;
			return nWxID;
		}

		// Also what a WM_COMMAND someone posted with the Win32 id comes back as:
		// wx makes the 16 bits a negative short.
		unsigned ToCommand( int nWxID ) const
		{
			const std::map<int, unsigned>::const_iterator posCommandID = commandIDs.find( nWxID );
			if ( posCommandID != commandIDs.end() )
			{
				return posCommandID->second;
			}
			return static_cast<unsigned>( nWxID ) & 0xFFFF;
		}
	};

	// One frame per process, so one table.
	CCommandIDs s_commandIDs;


	int ToWxID( unsigned nCommandID )
	{
		return s_commandIDs.ToWx( nCommandID );
	}


	unsigned ToCommandID( int nWxID )
	{
		return s_commandIDs.ToCommand( nWxID );
	}


	// What CFrameWnd shows in the status bar while a menu item is highlighted:
	// the string with the command's id, up to its newline.
	wxString CommandPrompt( unsigned nCommandID )
	{
		CString strPrompt;
		if ( ( nCommandID == 0 ) || !strPrompt.LoadString( nCommandID ) )
		{
			return wxString();
		}
		const int nNewLine = strPrompt.Find( '\n' );
		if ( nNewLine >= 0 )
		{
			strPrompt = strPrompt.Left( nNewLine );
		}
		return wxString::FromUTF8( strPrompt.GetString() );
	}


	// A toolbar button's tooltip: the command's string after its newline, where
	// MFC's toolbars found it.
	wxString CommandTooltip( unsigned nCommandID )
	{
		CString strPrompt;
		if ( ( nCommandID == 0 ) || !strPrompt.LoadString( nCommandID ) )
		{
			return wxString();
		}
		const int nNewLine = strPrompt.Find( '\n' );
		return ( nNewLine >= 0 ) ? wxString::FromUTF8( strPrompt.Mid( nNewLine + 1 ).GetString() ) : wxString();
	}


	// ToolBarButtonsMap.h's two-part buttons: a toolbar button and the command
	// its arrow runs. 0 for a button without one.
	unsigned ArrowCommand( unsigned nCommandID )
	{
		switch ( nCommandID )
		{
			case ID_CC_UNDO:
				return ID_CC_UNDO_ARROW;
			case ID_CC_REDO:
				return ID_CC_REDO_ARROW;
			default:
				return 0;
		}
	}


	// What CFrameWnd shows in the status bar when nothing is highlighted.
	wxString IdleMessage()
	{
		CString strMessage;
		strMessage.LoadString( AFX_IDS_IDLEMESSAGE );
		return wxString::FromUTF8( strMessage.GetString() );
	}


	// A command item at nPosition. wx is given the label up to its tab and the
	// native item gets the whole of it back: wx makes the text after a tab a
	// shortcut the frame answers, where MFC's menu bar only shows it -- and read
	// "\t Ctrl+N" as plain N, so typing N anywhere made a new map.
	wxMenuItem* InsertCommandItem( wxMenu *pMenu, size_t nPosition, unsigned nCommandID, const wxString &rLabel, const wxString &rHelp )
	{
		wxMenuItem *const pItem = pMenu->InsertCheckItem( nPosition, ToWxID( nCommandID ), rLabel.BeforeFirst( '\t' ), rHelp );
		std::wstring wszLabel = rLabel.ToStdWstring();
		MENUITEMINFOW labelInfo = { sizeof( labelInfo ) };
		labelInfo.fMask = MIIM_STRING;
		labelInfo.dwTypeData = &wszLabel[0];
		::SetMenuItemInfoW( pMenu->GetHMenu(), static_cast<UINT>( nPosition ), TRUE, &labelInfo );
		return pItem;
	}


	// A menu resource's popup as a wx menu. Every command is a check item,
	// because the editor's handlers may check any of them and wx refuses to
	// check a plain one; unchecked, the two look the same.
	wxMenu* MenuFromNative( HMENU hMenu )
	{
		wxMenu *const pMenu = new wxMenu();
		const int nCount = ::GetMenuItemCount( hMenu );
		for ( int nIndex = 0; nIndex < nCount; ++nIndex )
		{
			wchar_t pszLabel[256] = { 0 };
			MENUITEMINFOW itemInfo = { sizeof( itemInfo ) };
			itemInfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
			itemInfo.dwTypeData = pszLabel;
			itemInfo.cch = 255;
			if ( !::GetMenuItemInfoW( hMenu, nIndex, TRUE, &itemInfo ) )
			{
				continue;
			}
			if ( ( itemInfo.fType & MFT_SEPARATOR ) != 0 )
			{
				pMenu->AppendSeparator();
			}
			else if ( itemInfo.hSubMenu != 0 )
			{
				pMenu->AppendSubMenu( MenuFromNative( itemInfo.hSubMenu ), wxString( pszLabel ) );
			}
			else
			{
				InsertCommandItem( pMenu, pMenu->GetMenuItemCount(), itemInfo.wID, wxString( pszLabel ), CommandPrompt( itemInfo.wID ) );
			}
		}
		return pMenu;
	}


	// The menu resource CEditorAppSpecific::CreateMenus names, read from the
	// module that has it -- the resource handle the caller set, as MFC's
	// LoadMenu would find it.
	wxMenuBar* MenuBarFromResource( unsigned nResourceID )
	{
		const HINSTANCE hInstance = AfxFindResourceHandle( MAKEINTRESOURCE( nResourceID ), RT_MENU );
		const HMENU hMenu = ::LoadMenuW( hInstance, MAKEINTRESOURCEW( nResourceID ) );
		if ( hMenu == 0 )
		{
			return nullptr;
		}
		wxMenuBar *const pMenuBar = new wxMenuBar();
		const int nCount = ::GetMenuItemCount( hMenu );
		for ( int nIndex = 0; nIndex < nCount; ++nIndex )
		{
			wchar_t pszLabel[256] = { 0 };
			MENUITEMINFOW itemInfo = { sizeof( itemInfo ) };
			itemInfo.fMask = MIIM_STRING | MIIM_SUBMENU;
			itemInfo.dwTypeData = pszLabel;
			itemInfo.cch = 255;
			if ( ::GetMenuItemInfoW( hMenu, nIndex, TRUE, &itemInfo ) && ( itemInfo.hSubMenu != 0 ) )
			{
				pMenuBar->Append( MenuFromNative( itemInfo.hSubMenu ), wxString( pszLabel ) );
			}
		}
		::DestroyMenu( hMenu );
		return pMenuBar;
	}


	// A recent list's menu, rebuilt the way CMainFrame::OnUpdateUserCommand
	// rebuilds it: one item per name, or the one "empty" item.
	void FillRecentMenu( wxMenu *pMenu, const SUserData::CRecentList &rRecentList, unsigned nFirstID )
	{
		while ( pMenu->GetMenuItemCount() > 0 )
		{
			pMenu->Destroy( pMenu->FindItemByPosition( 0 ) );
		}
		unsigned nRecentCount = 0;
		for ( SUserData::CRecentList::const_iterator itRecentName = rRecentList.begin(); itRecentName != rRecentList.end(); ++itRecentName )
		{
			pMenu->AppendCheckItem( ToWxID( nFirstID + nRecentCount ), wxString::FromUTF8( itRecentName->c_str() ) );
			++nRecentCount;
		}
		if ( nRecentCount == 0 )
		{
			pMenu->AppendCheckItem( ToWxID( nFirstID ), wxString::FromUTF8( NMainFrameShared::GetRecentEmptyLabel().c_str() ) );
		}
	}


	// IDR_EDITORTYPE, the icon CFrameWnd::LoadFrame gives the MFC frame, in
	// the two sizes the title bar and the taskbar use.
	wxIconBundle LoadFrameIcons()
	{
		wxIconBundle icons;
		const HINSTANCE hInstance = AfxFindResourceHandle( MAKEINTRESOURCE( IDR_EDITORTYPE ), RT_GROUP_ICON );
		for ( const int nMetric : { SM_CXSMICON, SM_CXICON } )
		{
			const int nSize = ::GetSystemMetrics( nMetric );
			const HICON hIcon = static_cast<HICON>( ::LoadImage( hInstance, MAKEINTRESOURCE( IDR_EDITORTYPE ), IMAGE_ICON, nSize, nSize, 0 ) );
			if ( hIcon == 0 )
			{
				continue;
			}
			wxIcon icon;
			// The icon takes the handle and destroys it.
			if ( icon.CreateFromHICON( hIcon ) )
			{
				icons.AddIcon( icon );
			}
			else
			{
				::DestroyIcon( hIcon );
			}
		}
		return icons;
	}


	class CWxMainFrame : public wxFrame, public IMainFrame, public ICommandHandler, public IWidget
	{
		// A CWnd over the frame's handle, for as long as the handle lives.
		class CMfcWindow : public CWnd
		{
		public:
			virtual ~CMfcWindow()
			{
				// Before CWnd's destructor, which would destroy the window.
				Detach();
			}
		};

		CMfcWindow mfcWindow;
		NMainFrameWxPanes::CAuiManager auiManager;
		// Every menu bar the application added, by resource id. The frame owns
		// the attached one while it is attached; this map owns them all.
		std::map<unsigned, wxMenuBar*> menuBars;
		std::string szHelpFilePath;
		CMapEditorSingletonApp mapEditorSingletonApp;
		SMainFrameParams params;
		SSWTParams currentSWTParams;
		NMainFrameShared::CProgressHost progress;
		// The frame's own panes, as CMainFrame's wndLog, wndPropertyBrowser and
		// gdbBrowserList. Destroyed on close, before the frame's windows.
		std::unique_ptr<NMainFrameWxPanes::CLogPane> pLogPane;
		std::unique_ptr<NMainFrameWxPanes::CPropertiesPane> pPropertiesPane;
		std::list<std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane>> gdbBrowserPanes;
		// The editors' panes. Never removed, as CMainFrame's handles are not: an
		// editor may still hold one after its window is gone, and IsAlive is how
		// it finds out.
		std::list<std::unique_ptr<NMainFrameWxPanes::CDockPanel>> dockPanels;
		// Where documents open, and the document windows made there -- kept for
		// the same reason as the editors' panes.
		wxPanel *pWorkspace = nullptr;
		std::list<std::unique_ptr<NMainFrameWxPanes::CFrameWindow>> frameWindows;
		// The toolbars, the frame's and the editors', by bar id, and the pictures
		// their buttons are drawn from. The next id for a toolbar that asks for
		// one, as CMainFrame counts them, and the next position in the toolbar row.
		NMainFrameWxPanes::CToolBarImages toolBarImages;
		std::map<unsigned, std::unique_ptr<NMainFrameWxPanes::CToolBar>> toolBars;
		unsigned nFreeToolBarID = AFX_IDW_TOOLBAR + 9;
		int nNextToolBarPosition = 0;
		// The layout the frame has before a saved one is read: what a first run
		// gets, and what Reset GUI goes back to.
		wxString szDefaultLayout;
		// Whether the manager has laid the frame out. The first layout waits for
		// ShowFrame, when the frame has its size: wxAUI limits a dock to a third
		// of the frame the first time it sizes it, and keeps what it gave.
		bool bLaidOut = false;

	public:
		CWxMainFrame()
			: wxFrame( nullptr, wxID_ANY, wxString::FromUTF8( Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() ) )
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_VIEW, this );
			mfcWindow.Attach( GetHWND() );
			Bind( wxEVT_CLOSE_WINDOW, &CWxMainFrame::OnCloseWindow, this );
			Bind( wxEVT_MENU, &CWxMainFrame::OnMenu, this );
			Bind( wxEVT_MENU_OPEN, &CWxMainFrame::OnMenuOpen, this );
			Bind( wxEVT_DROP_FILES, &CWxMainFrame::OnDropFiles, this );
			Bind( wxEVT_UPDATE_UI, &CWxMainFrame::OnUpdateUI, this );
			Bind( wxEVT_RIGHT_DOWN, &CWxMainFrame::OnRightDown, this );
			Bind( wxEVT_RIGHT_UP, &CWxMainFrame::OnRightUp, this );
			Bind( wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &CWxMainFrame::OnToolDropDown, this );
		}

		virtual ~CWxMainFrame()
		{
			auiManager.UnInit();
			if ( CWinApp *const pApp = AfxGetApp() )
			{
				if ( pApp->m_pMainWnd == &mfcWindow )
				{
					pApp->m_pMainWnd = 0;
				}
			}
			mfcWindow.Detach();
			DetachMenuBar();
			for ( std::map<unsigned, wxMenuBar*>::iterator itMenuBar = menuBars.begin(); itMenuBar != menuBars.end(); ++itMenuBar )
			{
				delete itMenuBar->second;
			}
			menuBars.clear();
			if ( ICommandHandlerContainer *const pCommandHandlerContainer = Singleton<ICommandHandlerContainer>() )
			{
				pCommandHandlerContainer->Remove( CHID_VIEW );
			}
			// CMainFrame ends the message loop from OnNcDestroy, which MFC does
			// for the application's main window. MFC does not own this one.
			::PostQuitMessage( 0 );
		}

		CWnd* GetMfcWindow()
		{
			return &mfcWindow;
		}

		virtual wxStatusBar* OnCreateStatusBar( int nNumber, long nStyle, wxWindowID id, const wxString &rName ) override
		{
			CClassicStatusBar *const pStatusBar = NWx::Child<CClassicStatusBar>( this, id, nStyle, rName );
			pStatusBar->SetFieldsCount( nNumber );
			return pStatusBar;
		}

		// What CMainFrame::OnCreate does, in its order, less what is not here yet.
		void Build()
		{
			CEditorApp *const pApp = dynamic_cast<CEditorApp*>( AfxGetApp() );
			//
			mapEditorSingletonApp.CreateMapFile( GetHWND() );
			params.Load( true );
			szHelpFilePath = NMainFrameShared::GetHelpFilePath();
			SetIcons( LoadFrameIcons() );
			//
			// OnCreateStatusBar makes it a CClassicStatusBar.
			CreateStatusBar( N_STATUSBAR_PANES );
			static_cast<CClassicStatusBar*>( GetStatusBar() )->SetClassicWidths( N_STATUSBAR_PANES, STATUSBAR_TEXT_WIDTHS );
			SetStatusText( IdleMessage(), 0 );
			//
			pApp->CreateMenus( this );
			//
			// The area documents open in, an MDI client's colour, which the
			// document window fills.
			auiManager.SetManagedWindow( this );
			pWorkspace = NWx::Child<wxPanel>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );
			pWorkspace->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
			pWorkspace->SetSizer( new wxBoxSizer( wxVERTICAL ) );
			auiManager.AddPane( pWorkspace, wxAuiPaneInfo().Name( "workspace" ).CenterPane() );
			CreateMainToolBars();
			CreatePanes();
			//
			Singleton<IMainFrameContainer>()->Set( this, this );
			for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
			{
				pApp->GetEditorModules()[nModuleIndex]->ModuleCreateControls();
			}
			Singleton<IEditorContainer>()->CreateControls();
			// Where CMainFrame loads its bar state: every pane and toolbar is made,
			// and the editors have not yet hidden theirs.
			szDefaultLayout = auiManager.SavePerspective();
			LoadLayout();
			for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
			{
				pApp->GetEditorModules()[nModuleIndex]->ModulePostCreateControls();
			}
			Singleton<IEditorContainer>()->PostCreateControls();
			//
			if ( ( params.rect.Width() != 0 ) && ( params.rect.Height() != 0 ) )
			{
				// The rectangle both frames save is GetWindowPlacement's restored
				// one, in workspace coordinates rather than screen ones, so it
				// goes back the same way. Hidden: Show shows it.
				WINDOWPLACEMENT windowPlacement = { sizeof( windowPlacement ) };
				::GetWindowPlacement( GetHWND(), &windowPlacement );
				windowPlacement.rcNormalPosition.left = params.rect.left;
				windowPlacement.rcNormalPosition.top = params.rect.top;
				windowPlacement.rcNormalPosition.right = params.rect.right;
				windowPlacement.rcNormalPosition.bottom = params.rect.bottom;
				windowPlacement.showCmd = SW_HIDE;
				::SetWindowPlacement( GetHWND(), &windowPlacement );
			}
			//
			RegisterObjectStorage();
			DragAcceptFiles( true );
			//
			SSWTParams swtParams;
			swtParams.dwFlags = SWT_ALL;
			swtParams.bFillMODFromBase = true;
			swtParams.bModified = false;
			SetWindowTitle( swtParams );
		}

		void ShowFrame()
		{
			// Before Show, so it opens maximised rather than growing into it.
			if ( params.bMaximized )
			{
				Maximize( true );
			}
			Show( true );
			// Now that the frame has the size it will have.
			auiManager.Update();
			bLaidOut = true;
			Update();
		}

		// IWidget: what dialogs, message boxes and popup menus are owned by.
		// The MFC front-end's side of that is a CWnd, so it is the attached one.
		virtual void* GetNativeWidget()
		{
			return static_cast<CWnd*>( &mfcWindow );
		}

		// IMainFrame
		// As CMainFrame's: the button under the mouse, then the button in the
		// first shown toolbar that has it -- in screen coordinates.
		virtual bool GetToolBarButtonLeftBottomPos( const CTPoint<int> &rMousePoint, unsigned nButtonID, CTPoint<int> *pLeftBottomPos )
		{
			const wxPoint point( rMousePoint.x, rMousePoint.y );
			for ( int nPass = 0; nPass < 2; ++nPass )
			{
				for ( std::map<unsigned, std::unique_ptr<NMainFrameWxPanes::CToolBar>>::const_iterator itToolBar = toolBars.begin(); itToolBar != toolBars.end(); ++itToolBar )
				{
					wxAuiToolBar *const pToolBar = itToolBar->second->GetToolBar();
					if ( ( pToolBar == nullptr ) || !pToolBar->IsShownOnScreen() )
					{
						continue;
					}
					const wxRect toolRect = pToolBar->GetToolRect( ToWxID( nButtonID ) );
					if ( toolRect.IsEmpty() )
					{
						continue;
					}
					const wxRect screenRect( pToolBar->ClientToScreen( toolRect.GetPosition() ), toolRect.GetSize() );
					if ( ( nPass == 1 ) || screenRect.Contains( point ) )
					{
						if ( pLeftBottomPos != 0 )
						{
							pLeftBottomPos->x = screenRect.GetLeft();
							pLeftBottomPos->y = screenRect.GetBottom() + 1;
						}
						return true;
					}
				}
			}
			return false;
		}

		virtual IFrameWindow* CreateChildFrame( unsigned nResource )
		{
			if ( pWorkspace == nullptr )
			{
				return 0;
			}
			NMainFrameWxPanes::CMfcPanel *const pPanel = NWx::Child<NMainFrameWxPanes::CMfcPanel>( pWorkspace );
			pWorkspace->GetSizer()->Add( pPanel, wxSizerFlags( 1 ).Expand() );
			pWorkspace->Layout();
			frameWindows.push_back( std::unique_ptr<NMainFrameWxPanes::CFrameWindow>( new NMainFrameWxPanes::CFrameWindow( pPanel ) ) );
			return frameWindows.back().get();
		}

		virtual bool SetChildFrameWindowContents( IFrameWindow *pChildFrame, IWidget *pContents )
		{
			NMainFrameWxPanes::CFrameWindow *const pHandle = static_cast<NMainFrameWxPanes::CFrameWindow*>( pChildFrame );
			if ( ( pHandle == 0 ) || ( pHandle->GetPanel() == nullptr ) )
			{
				return false;
			}
			// A wx view made in the panel is laid out in it; anything else is an
			// MFC window kept its size.
			if ( wxWindow *const pWxContents = ToWxWindow( pContents ) )
			{
				pHandle->GetPanel()->SetWxContents( pWxContents );
				return true;
			}
			const CWnd *const pwndContents = ToCWnd( pContents );
			pHandle->GetPanel()->SetContents( ( pwndContents != 0 ) ? pwndContents->GetSafeHwnd() : 0 );
			return true;
		}

		virtual IDockPanel* CreateControlBar( unsigned *pnID, const std::string &rszTitle, const unsigned nStyle, const unsigned nPlace, const float fRate, const int nWidth )
		{
			NI_ASSERT( pnID != 0, "CWxMainFrame::CreateControlBar() pnID == 0" );
			// The id made unique among the panes, as SECControlBar::GetUniqueBarID
			// made it among the bars, and handed back the same way.
			while ( auiManager.GetPane( PaneName( *pnID ) ).IsOk() )
			{
				++( *pnID );
			}
			NMainFrameWxPanes::CMfcPanel *const pPanel = NWx::Child<NMainFrameWxPanes::CMfcPanel>( this );
			auiManager.AddPane( pPanel, NMainFrameWxPanes::DockedPaneInfo( PaneName( *pnID ), rszTitle, nPlace, fRate, nWidth ) );
			if ( bLaidOut )
			{
				auiManager.Update();
			}
			dockPanels.push_back( std::unique_ptr<NMainFrameWxPanes::CDockPanel>( new NMainFrameWxPanes::CDockPanel( &auiManager, pPanel, &bLaidOut ) ) );
			return dockPanels.back().get();
		}

		virtual bool SetControlBarWindowContents( IDockPanel *pDockPanel, IWidget *pContents )
		{
			NMainFrameWxPanes::CDockPanel *const pHandle = static_cast<NMainFrameWxPanes::CDockPanel*>( pDockPanel );
			if ( ( pHandle == 0 ) || ( pHandle->GetPanel() == nullptr ) )
			{
				return false;
			}
			// As SetChildFrameWindowContents.
			if ( wxWindow *const pWxContents = ToWxWindow( pContents ) )
			{
				pHandle->GetPanel()->SetWxContents( pWxContents );
				return true;
			}
			const CWnd *const pwndContents = ToCWnd( pContents );
			pHandle->GetPanel()->SetContents( ( pwndContents != 0 ) ? pwndContents->GetSafeHwnd() : 0 );
			return true;
		}

		virtual bool AddMenuResources( std::vector<unsigned> &rMenuIDList )
		{
			if ( rMenuIDList.empty() )
			{
				return false;
			}
			for ( std::vector<unsigned>::const_iterator itMenuID = rMenuIDList.begin(); itMenuID != rMenuIDList.end(); ++itMenuID )
			{
				if ( menuBars.find( *itMenuID ) != menuBars.end() )
				{
					continue;
				}
				if ( wxMenuBar *const pMenuBar = MenuBarFromResource( *itMenuID ) )
				{
					menuBars[*itMenuID] = pMenuBar;
				}
			}
			ShowMenu( rMenuIDList[0] );
			return true;
		}

		virtual void ShowMenu( const unsigned nResourceID )
		{
			const std::map<unsigned, wxMenuBar*>::const_iterator posMenuBar = menuBars.find( nResourceID );
			if ( posMenuBar != menuBars.end() )
			{
				// Detaches the one shown, which stays in the map.
				SetMenuBar( posMenuBar->second );
			}
		}

		// The large pictures were never used: the toolbars are small.
		virtual bool AddToolBarResource( const unsigned nStandartResourceID, const unsigned nLargeResourceID )
		{
			return toolBarImages.AddToolBarResource( nStandartResourceID );
		}

		// SECToolBarManager::DefineDefaultToolBar, docked along the top where
		// nStyle says, next to the toolbar made before it.
		virtual void CreateToolBar( unsigned *pnID, const std::string &rszTitle, const unsigned nButtonCount, const unsigned *pButtonIDMap,
																const uint32_t dwAlignment, const unsigned nStyle, const bool bDocked, const bool bVisible, const bool bMainToolBar )
		{
			NI_ASSERT( pnID != 0, "CWxMainFrame::CreateToolBar() pnID == 0" );
			if ( ( *pnID ) == 0xFFFFFFFF )
			{
				( *pnID ) = nFreeToolBarID;
				++nFreeToolBarID;
			}
			wxAuiToolBar *const pToolBar = NWx::Child<wxAuiToolBar>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE );
			pToolBar->SetArtProvider( new wxAuiMSWToolBarArt() );
			pToolBar->SetToolBitmapSize( wxSize( 16, 16 ) );
			for ( unsigned nButton = 0; nButton < nButtonCount; ++nButton )
			{
				const unsigned nCommandID = pButtonIDMap[nButton];
				if ( nCommandID == ID_SEPARATOR )
				{
					pToolBar->AddSeparator();
					continue;
				}
				// Every button is a check item, for the handlers that check one,
				// but the two-part ones, whose arrow wx will not draw on one.
				const bool bTwoPart = ( ArrowCommand( nCommandID ) != 0 );
				pToolBar->AddTool( ToWxID( nCommandID ), wxString(), toolBarImages.Get( nCommandID ), CommandTooltip( nCommandID ),
													 bTwoPart ? wxITEM_NORMAL : wxITEM_CHECK );
				if ( bTwoPart )
				{
					pToolBar->SetToolDropDown( ToWxID( nCommandID ), true );
				}
			}
			pToolBar->Realize();
			//
			// Share a row in creation order, so the toolbars start side by side.
			// wxAUI moves overlapping positions apart to fit each visible toolbar.
			wxAuiPaneInfo info;
			info.Name( wxString::Format( "ToolBar%u", *pnID ) ).Caption( wxString::FromUTF8( rszTitle.c_str() ) ).ToolbarPane().Row( 0 ).Position( nNextToolBarPosition );
			switch ( nStyle )
			{
				case AFX_IDW_DOCKBAR_BOTTOM:
					info.Bottom();
					break;
				case AFX_IDW_DOCKBAR_LEFT:
					info.Left();
					break;
				case AFX_IDW_DOCKBAR_RIGHT:
					info.Right();
					break;
				default:
					info.Top();
					break;
			}
			info.Show( bVisible );
			++nNextToolBarPosition;
			auiManager.AddPane( pToolBar, info );
			if ( bLaidOut )
			{
				auiManager.Update();
			}
			toolBars[*pnID] = std::unique_ptr<NMainFrameWxPanes::CToolBar>( new NMainFrameWxPanes::CToolBar( &auiManager, pToolBar, &bLaidOut ) );
		}

		virtual IToolBar* GetToolBar( unsigned nID )
		{
			const std::map<unsigned, std::unique_ptr<NMainFrameWxPanes::CToolBar>>::const_iterator posToolBar = toolBars.find( nID );
			return ( posToolBar != toolBars.end() ) ? posToolBar->second.get() : 0;
		}

		virtual void SetStatusBarText( int nPaneIndex, const std::string &szText )
		{
			if ( ( nPaneIndex >= 0 ) && ( nPaneIndex < N_STATUSBAR_PANES ) )
			{
				SetStatusText( wxString::FromUTF8( szText.c_str() ), nPaneIndex );
			}
		}

		virtual void SetWindowTitle( const SSWTParams &rSWTParams )
		{
			std::string szTitle;
			if ( NMainFrameShared::UpdateTitle( &currentSWTParams, rSWTParams, &szTitle ) )
			{
				SetTitle( wxString::FromUTF8( szTitle.c_str() ) );
			}
		}

		// The menu event the command's item would send, queued: OnMenu answers
		// it after the caller returns, as it answered the WM_COMMAND the callers
		// posted to the main window before (ToCommandID maps both back).
		virtual void PostCommand( unsigned nCommandID )
		{
			QueueEvent( new wxCommandEvent( wxEVT_MENU, ToWxID( nCommandID ) ) );
		}

		// ILogger
		virtual void Log( ELogOutputType eLogOutputType, const std::string &szText )
		{
			if ( pLogPane )
			{
				pLogPane->GetContents().Log( eLogOutputType, szText );
			}
		}

		virtual void ClearLog()
		{
			if ( pLogPane )
			{
				pLogPane->GetContents().ClearLog();
			}
		}

		virtual void SaveObjectStorage( int nGDBBrowserID )
		{
			Singleton<IUserDataContainer>()->Get()->nFocusedGDBBrowserID = nGDBBrowserID;
			RestoreObjectStorage();
		}

		// The focused Game Database pane answers CHID_OBJECT_STORAGE and CHID_MAIN.
		virtual void RestoreObjectStorage()
		{
			if ( NMainFrameWxPanes::CGDBBrowserPane *const pPane = FindGDBBrowserPane( Singleton<IUserDataContainer>()->Get()->nFocusedGDBBrowserID ) )
			{
				SetFocusedGDBBrowserPane( pPane );
			}
		}

		virtual bool BrowseLink( std::string *pszResult, const std::string &rszInitialValue, const SPropertyDesc *pPropertyDesc, bool bMultiRef, bool bEnableEdit )
		{
			return NMainFrameShared::BrowseLink( pszResult, rszInitialValue, pPropertyDesc, bMultiRef, bEnableEdit );
		}

		virtual bool BrowseForObject( CDBID *pObjectDBID, std::string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty )
		{
			return NMainFrameShared::BrowseForObject( pObjectDBID, pszObjectTypeName, bEnableEdit, bEnableEmpty );
		}

		virtual void CreateProgressDialog()
		{
			progress.Create( this );
		}

		virtual void DestroyProgressDialog()
		{
			progress.Destroy();
		}

		virtual void SetProgressDialogTitle( const std::string &rszTitle )
		{
			progress.SetTitle( rszTitle );
		}

		virtual void SetProgressDialogMessage( const std::string &rszMessage )
		{
			progress.SetMessage( rszMessage );
		}

		virtual void SetProgressDialogRange( int nStart, int nFinish )
		{
			progress.SetRange( nStart, nFinish );
		}

		virtual void SetProgressDialogPosition( int nPosition )
		{
			progress.SetPosition( nPosition );
		}

		virtual void IterateProgressDialogPosition()
		{
			progress.IteratePosition();
		}

		// ICommandHandler, as CHID_VIEW.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			switch ( nCommandID )
			{
				case ID_VIEW_SHOW_PROPERTY_BROWSER:
					ShowPane( pPropertiesPane ? pPropertiesPane->GetPanel() : nullptr, dwData != 0 );
					return true;
				case ID_VIEW_SHOW_LOG:
					ShowPane( pLogPane ? pLogPane->GetPanel() : nullptr, dwData != 0 );
					return true;
				case ID_VIEW_SHOW_GDB_BROWSER:
					if ( !gdbBrowserPanes.empty() )
					{
						ShowPane( gdbBrowserPanes.front()->GetPanel(), dwData != 0 );
					}
					return true;
				case ID_VIEW_SAVE_CHANGES:
					return NMainFrameShared::SaveChanges( dwData > 0, [this]() { ReloadData(); } );
				case ID_VIEW_RELOAD:
					ReloadData();
					return true;
				default:
					return false;
			}
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			switch ( nCommandID )
			{
				case ID_VIEW_SHOW_PROPERTY_BROWSER:
				case ID_VIEW_SHOW_LOG:
				case ID_VIEW_SHOW_GDB_BROWSER:
				case ID_VIEW_SAVE_CHANGES:
				case ID_VIEW_RELOAD:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				default:
					return false;
			}
		}

	private:
		// CMainFrame::ReloadData: each Game Database pane reads its tables again.
		void ReloadData()
		{
			for ( std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				rpPane->GetContents().HandleCommand( ID_MAIN_RELOAD, 0 );
			}
		}

		static wxString PaneName( unsigned nID )
		{
			return wxString::Format( "Pane%u", nID );
		}

		// Where the layout is kept: beside the MFC frame's bar state, in the
		// application's registry key, but a section of its own, since the two
		// frames' layouts are nothing alike.
		static CString LayoutSection()
		{
			CString strSection;
			strSection.LoadString( IDS_REGISTRY_KEY_WINDOWBAR );
			return strSection + "-wx";
		}

		void SaveLayout()
		{
			AfxGetApp()->WriteProfileString( LayoutSection(), "Layout", auiManager.SavePerspective().utf8_str() );
		}

		void LoadLayout()
		{
			const CString strLayout = AfxGetApp()->GetProfileString( LayoutSection(), "Layout", "" );
			if ( !strLayout.IsEmpty() )
			{
				ApplyLayout( wxString::FromUTF8( strLayout.GetString() ) );
			}
		}

		// wxAuiManager::LoadPerspective, less two things it does that do not suit
		// a layout kept across builds and sessions: it hides every pane the layout
		// does not name, and it gives the rest the captions they had when it was
		// saved. A pane the layout does not know keeps what it had, and every pane
		// keeps the caption the frame gave it.
		bool ApplyLayout( const wxString &rLayout )
		{
			wxAuiPaneInfoArray &rPanes = auiManager.GetAllPanes();
			std::vector<wxAuiPaneInfo> before;
			for ( size_t nPane = 0; nPane < rPanes.GetCount(); ++nPane )
			{
				before.push_back( rPanes.Item( nPane ) );
			}
			if ( !auiManager.LoadPerspective( rLayout, false ) )
			{
				return false;
			}
			for ( size_t nPane = 0; ( nPane < rPanes.GetCount() ) && ( nPane < before.size() ); ++nPane )
			{
				wxAuiPaneInfo &rPane = rPanes.Item( nPane );
				if ( rLayout.Find( "name=" + before[nPane].name + ";" ) == wxNOT_FOUND )
				{
					rPane.SafeSet( before[nPane] );
				}
				rPane.Caption( before[nPane].caption );
			}
			return true;
		}

		// CMainFrame::OnResetGUI: the layout a first run gets, the frame's own
		// panes shown, the editors' panes as their defaults say, and the result
		// kept at once.
		void ResetLayout()
		{
			ApplyLayout( szDefaultLayout );
			if ( pLogPane )
			{
				auiManager.GetPane( pLogPane->GetPanel() ).Show( true );
			}
			if ( pPropertiesPane )
			{
				auiManager.GetPane( pPropertiesPane->GetPanel() ).Show( true );
			}
			for ( std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				auiManager.GetPane( rpPane->GetPanel() ).Show( true );
			}
			Singleton<IEditorContainer>()->ResetGUI();
			auiManager.Update();
			SaveLayout();
		}

		// The start of CMainFrame::OnCreate: its toolbar resources, the game
		// icon for Run Game, and its six toolbars from its own tables.
		void CreateMainToolBars()
		{
			for ( int nToolBar = 0; nToolBar < TOOLBARS_COUNT; ++nToolBar )
			{
				toolBarImages.AddToolBarResource( CMainFrame::TOOLBAR_ID[nToolBar] );
			}
			toolBarImages.AddIcon( ID_TOOLS_RUN_GAME, IDI_GAME_LAUNCH );
			for ( int nToolBar = 0; nToolBar < TOOLBARS_COUNT; ++nToolBar )
			{
				CString strName;
				strName.LoadString( CMainFrame::TOOLBAR_NAME_ID[nToolBar] );
				unsigned nID = CMainFrame::TOOLBAR_CONTROL_ID[nToolBar];
				CreateToolBar( &nID, std::string( strName.GetString() ), CMainFrame::TOOLBAR_ELEMENTS_COUNT[nToolBar],
											 CMainFrame::TOOLBAR_ELEMENTS_ID[nToolBar], CMainFrame::TOOLBAR_STYLE[nToolBar], AFX_IDW_DOCKBAR_TOP,
											 true, CMainFrame::TOOLBAR_SHOW[nToolBar], false );
			}
		}

		// A View > toolbar command's toolbar: CMainFrame::OnViewToolBar's.
		NMainFrameWxPanes::CToolBar* ViewToolBar( unsigned nCommandID )
		{
			const std::map<unsigned, std::unique_ptr<NMainFrameWxPanes::CToolBar>>::const_iterator posToolBar =
				toolBars.find( CMainFrame::TOOLBAR_CONTROL_ID[nCommandID - ID_VIEW_TOOLBAR_MAIN] );
			return ( posToolBar != toolBars.end() ) ? posToolBar->second.get() : nullptr;
		}

		// A toolbar button's state, asked on idle as MFC's toolbars asked
		// through CCmdUI: the menus' own answer. wxAuiToolBar sends this for its
		// buttons.
		//
		// Only for those. wx sends this on idle for every window, with the
		// window's own id, and it reaches the frame: answering them all disabled
		// the frame and everything in it, since no command has their ids. Menus
		// send it too as they open, for submenus as well, under ids wx made up;
		// answering those checked a submenu, and the editor went with no trace.
		// OnMenuOpen already sets the menus' states.
		void OnUpdateUI( wxUpdateUIEvent &rEvent )
		{
			// wxAuiToolBar also sends an update for its own window. Only answer
			// button updates: disabling the toolbar itself disables its drag grip.
			wxAuiToolBar *const pToolBar = wxDynamicCast( rEvent.GetEventObject(), wxAuiToolBar );
			if ( ( pToolBar == nullptr ) || ( pToolBar->FindTool( rEvent.GetId() ) == nullptr ) )
			{
				rEvent.Skip();
				return;
			}
			const unsigned nCommandID = ToCommandID( rEvent.GetId() );
			bool bEnable = false;
			bool bCheck = false;
			UpdateMenuCommand( nCommandID, &bEnable, &bCheck );
			rEvent.Enable( bEnable );
			if ( ArrowCommand( nCommandID ) == 0 )
			{
				rEvent.Check( bCheck );
			}
		}

		// The arrow of a two-part button runs its own command, which shows the
		// undo or redo list under the button.
		void OnToolDropDown( wxAuiToolBarEvent &rEvent )
		{
			const unsigned nArrowID = ArrowCommand( ToCommandID( rEvent.GetId() ) );
			if ( !rEvent.IsDropDownClicked() || ( nArrowID == 0 ) )
			{
				rEvent.Skip();
				return;
			}
			NMainFrameShared::RunUserCommand( nArrowID );
		}

		// What CMainFrame::OnCreate makes, in its order: Log, Selection
		// Properties, then the Game Database panes the user data lists, one to
		// begin with. The order matters: a browser hands its selection to
		// Selection Properties as it fills, so that has to be there first.
		void CreatePanes()
		{
			pLogPane.reset( new NMainFrameWxPanes::CLogPane() );
			pLogPane->Create( this, &auiManager );
			pPropertiesPane.reset( new NMainFrameWxPanes::CPropertiesPane() );
			pPropertiesPane->Create( this, &auiManager, this );
			//
			SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
			if ( pUserData->gdbBrowserIDList.empty() )
			{
				pUserData->gdbBrowserIDList.push_back( FreeGDBBrowserID() );
			}
			int nWindowIndex = 0;
			for ( std::list<int>::const_iterator itGDBBrowserID = pUserData->gdbBrowserIDList.begin(); itGDBBrowserID != pUserData->gdbBrowserIDList.end(); ++itGDBBrowserID )
			{
				AddGDBBrowserPane( *itGDBBrowserID, nWindowIndex );
				++nWindowIndex;
			}
		}

		NMainFrameWxPanes::CGDBBrowserPane* AddGDBBrowserPane( int nGDBBrowserID, int nWindowIndex )
		{
			std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> pPane( new NMainFrameWxPanes::CGDBBrowserPane( nGDBBrowserID, this ) );
			if ( !pPane->Create( this, &auiManager, nWindowIndex ) )
			{
				return nullptr;
			}
			gdbBrowserPanes.push_back( std::move( pPane ) );
			return gdbBrowserPanes.back().get();
		}

		// The end of CMainFrame::OnCreate: the focused Game Database pane, or the
		// first, answers for the object storage, and every pane takes the
		// disable_edit setting.
		void RegisterObjectStorage()
		{
			SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
			const bool bEnableEdit = ( NGlobal::GetVar( "disable_edit", 0 ) == 0 );
			NMainFrameWxPanes::CGDBBrowserPane *pFocusedPane = FindGDBBrowserPane( pUserData->nFocusedGDBBrowserID );
			if ( ( pFocusedPane == nullptr ) && !gdbBrowserPanes.empty() )
			{
				pFocusedPane = gdbBrowserPanes.front().get();
				pUserData->nFocusedGDBBrowserID = pFocusedPane->GetContents().GetID();
			}
			if ( pFocusedPane != nullptr )
			{
				SetFocusedGDBBrowserPane( pFocusedPane );
			}
			for ( std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				rpPane->GetContents().EnableEdit( bEnableEdit );
			}
			if ( pPropertiesPane )
			{
				pPropertiesPane->EnableEdit( bEnableEdit );
			}
		}

		void SetFocusedGDBBrowserPane( NMainFrameWxPanes::CGDBBrowserPane *pPane )
		{
			ICommandHandlerContainer *const pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
			pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, pPane->GetContents().GetObjectStorage() );
			pCommandHandlerContainer->Set( CHID_MAIN, &( pPane->GetContents() ) );
		}

		NMainFrameWxPanes::CGDBBrowserPane* FindGDBBrowserPane( int nGDBBrowserID ) const
		{
			for ( const std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				if ( rpPane->GetContents().GetID() == nGDBBrowserID )
				{
					return rpPane.get();
				}
			}
			return nullptr;
		}

		NMainFrameWxPanes::CGDBBrowserPane* GDBBrowserPaneAt( int nIndex ) const
		{
			for ( const std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				if ( nIndex == 0 )
				{
					return rpPane.get();
				}
				--nIndex;
			}
			return nullptr;
		}

		// SECControlBar::GetUniqueBarID for a Game Database pane.
		int FreeGDBBrowserID() const
		{
			int nGDBBrowserID = ID_DW_GDB_BROWSER;
			while ( FindGDBBrowserPane( nGDBBrowserID ) != nullptr )
			{
				++nGDBBrowserID;
			}
			return nGDBBrowserID;
		}

		// CMainFrame::OnDWGDBBrowserNew: a pane more, focused.
		void NewGDBBrowserPane()
		{
			if ( gdbBrowserPanes.size() >= static_cast<size_t>( ID_VIEW_DW_GDB_BROWSER_LAST - ID_VIEW_DW_GDB_BROWSER_FIRST ) )
			{
				return;
			}
			SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
			const int nGDBBrowserID = FreeGDBBrowserID();
			NMainFrameWxPanes::CGDBBrowserPane *const pPane = AddGDBBrowserPane( nGDBBrowserID, static_cast<int>( gdbBrowserPanes.size() ) );
			if ( pPane == nullptr )
			{
				return;
			}
			auiManager.Update();
			pPane->GetContents().EnableEdit( NGlobal::GetVar( "disable_edit", 0 ) == 0 );
			SetFocusedGDBBrowserPane( pPane );
			pUserData->nFocusedGDBBrowserID = nGDBBrowserID;
			pUserData->gdbBrowserIDList.push_back( nGDBBrowserID );
		}

		// CMainFrame::OnDWGDBBrowserRemove: the focused pane goes, while it is not
		// the last; the ones after it are renumbered and the first is focused.
		void RemoveGDBBrowserPane()
		{
			if ( gdbBrowserPanes.size() <= 1 )
			{
				return;
			}
			SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
			std::list<int>::iterator itGDBBrowserID = pUserData->gdbBrowserIDList.begin();
			int nWindowIndex = 0;
			CString strDWName;
			strDWName.LoadString( IDS_DW_GDB_BROWSE_NAME );
			for ( std::list<std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane>>::iterator itPane = gdbBrowserPanes.begin(); itPane != gdbBrowserPanes.end(); ++itPane )
			{
				if ( ( *itPane )->GetContents().GetID() == pUserData->nFocusedGDBBrowserID )
				{
					( *itPane )->GetContents().Stop();
					wxWindow *const pPanel = ( *itPane )->GetPanel();
					itPane = gdbBrowserPanes.erase( itPane );
					if ( pPanel != nullptr )
					{
						auiManager.DetachPane( pPanel );
						pPanel->Destroy();
					}
					if ( itGDBBrowserID != pUserData->gdbBrowserIDList.end() )
					{
						pUserData->gdbBrowserIDList.erase( itGDBBrowserID );
					}
					for ( ; itPane != gdbBrowserPanes.end(); ++itPane )
					{
						auiManager.GetPane( ( *itPane )->GetPanel() ).Caption( wxString::FromUTF8( fmt::sprintf( strDWName.GetString(), nWindowIndex ).c_str() ) );
						++nWindowIndex;
					}
					SetFocusedGDBBrowserPane( gdbBrowserPanes.front().get() );
					pUserData->nFocusedGDBBrowserID = gdbBrowserPanes.front()->GetContents().GetID();
					auiManager.Update();
					return;
				}
				if ( itGDBBrowserID != pUserData->gdbBrowserIDList.end() )
				{
					++itGDBBrowserID;
				}
				++nWindowIndex;
			}
		}

		// Before the frame's windows go, after the frame stopped being the main
		// frame: what CDWGDBBrowser::OnDestroy did, then the views.
		void DestroyPanes()
		{
			for ( std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				rpPane->GetContents().Stop();
			}
			gdbBrowserPanes.clear();
			pPropertiesPane.reset();
			pLogPane.reset();
		}

		bool IsPaneShown( wxWindow *pPanel )
		{
			return ( pPanel != nullptr ) && auiManager.GetPane( pPanel ).IsShown();
		}

		void ShowPane( wxWindow *pPanel, bool bShow )
		{
			if ( pPanel != nullptr )
			{
				auiManager.GetPane( pPanel ).Show( bShow );
				auiManager.Update();
			}
		}

		// View > Game Database: one item per pane, rebuilt when the menu opens, as
		// CMainFrame::OnUpdateDWGDBBrowserWindow rebuilds it -- before the three
		// fixed items, the separator, New Window and Remove Window.
		void FillGDBBrowserMenu( wxMenu *pMenu )
		{
			const size_t N_FIXED_ITEMS = 3;
			while ( pMenu->GetMenuItemCount() > N_FIXED_ITEMS )
			{
				pMenu->Destroy( pMenu->FindItemByPosition( 0 ) );
			}
			CString strMenuLabel;
			CString strMenuLabelShort;
			strMenuLabel.LoadString( IDS_DW_GDB_BROWSE_MENU_LABEL );
			strMenuLabelShort.LoadString( IDS_DW_GDB_BROWSE_MENU_LABEL_SHORT );
			int nWindowIndex = 0;
			for ( ; nWindowIndex < static_cast<int>( gdbBrowserPanes.size() ); ++nWindowIndex )
			{
				const std::string szLabel = fmt::sprintf( ( ( nWindowIndex == 0 ) ? strMenuLabel : strMenuLabelShort ).GetString(), nWindowIndex );
				InsertCommandItem( pMenu, nWindowIndex, ID_VIEW_DW_GDB_BROWSER_FIRST + nWindowIndex, wxString::FromUTF8( szLabel.c_str() ), wxString() );
			}
			if ( nWindowIndex == 0 )
			{
				CString strEmptyLabel;
				strEmptyLabel.LoadString( IDS_DW_GDB_BROWSE_EMPTY_MENU_LABEL );
				InsertCommandItem( pMenu, 0, ID_VIEW_DW_GDB_BROWSER_FIRST, wxString::FromUTF8( strEmptyLabel.GetString() ), wxString() );
			}
		}

		// CMainFrame::OnClose, in its order, less what is not here yet.
		void OnCloseWindow( wxCloseEvent &rEvent )
		{
			if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
			{
				if ( rEvent.CanVeto() )
				{
					rEvent.Veto();
				}
				return;
			}
			progress.Destroy();
			//
			CEditorApp *const pApp = dynamic_cast<CEditorApp*>( AfxGetApp() );
			Singleton<IEditorContainer>()->DestroyActiveEditor( false );
			Singleton<IEditorContainer>()->PreDestroyControls();
			for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
			{
				pApp->GetEditorModules()[nModuleIndex]->ModulePreDestroyControls();
			}
			// Where CMainFrame saves its bar state: the editors have put their own
			// panes' visibility away and hidden them.
			SaveLayout();
			Singleton<IChildFrameContainer>()->Destroy();
			Singleton<IEditorContainer>()->DestroyControls();
			for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
			{
				pApp->GetEditorModules()[nModuleIndex]->ModuleDestroyControls();
			}
			//
			WINDOWPLACEMENT windowPlacement = { sizeof( windowPlacement ) };
			::GetWindowPlacement( GetHWND(), &windowPlacement );
			params.bMaximized = ( windowPlacement.showCmd == SW_SHOWMAXIMIZED );
			params.rect = CTRect<int>( windowPlacement.rcNormalPosition.left,
																 windowPlacement.rcNormalPosition.top,
																 windowPlacement.rcNormalPosition.right,
																 windowPlacement.rcNormalPosition.bottom );
			params.Save( true );
			//
			mapEditorSingletonApp.RemoveMapFile();
			Singleton<IMainFrameContainer>()->Set( 0, 0 );
			DestroyPanes();
			Destroy();
		}

		void OnMenu( wxCommandEvent &rEvent )
		{
			const unsigned nCommandID = ToCommandID( rEvent.GetId() );
			switch ( nCommandID )
			{
				case ID_MAIN_REGISTER_XDB:
					NMainFrameShared::RegisterXDB( [this]() { ReloadData(); } );
					return;
				case ID_HELP_CONTENTS:
					NMainFrameShared::ShowHelpContents( szHelpFilePath );
					return;
				case ID_HELP_ABOUT:
					NMainFrameShared::ShowAbout();
					return;
				case ID_APP_EXIT:
					// CWinApp::OnAppExit: close the main window.
					Close();
					return;
				case ID_VIEW_DW_PROPERTY_BROWSER:
					if ( pPropertiesPane )
					{
						ShowPane( pPropertiesPane->GetPanel(), !IsPaneShown( pPropertiesPane->GetPanel() ) );
					}
					return;
				case ID_VIEW_DW_LOG:
					if ( pLogPane )
					{
						ShowPane( pLogPane->GetPanel(), !IsPaneShown( pLogPane->GetPanel() ) );
					}
					return;
				case ID_VIEW_DW_GDB_BROWSER_NEW:
					NewGDBBrowserPane();
					return;
				case ID_VIEW_RESET_GUI:
					ResetLayout();
					return;
				case ID_VIEW_DW_GDB_BROWSER_REMOVE:
					RemoveGDBBrowserPane();
					return;
				default:
					break;
			}
			if ( ( nCommandID >= ID_VIEW_DW_GDB_BROWSER_FIRST ) && ( nCommandID <= ID_VIEW_DW_GDB_BROWSER_LAST ) )
			{
				if ( NMainFrameWxPanes::CGDBBrowserPane *const pPane = GDBBrowserPaneAt( nCommandID - ID_VIEW_DW_GDB_BROWSER_FIRST ) )
				{
					ShowPane( pPane->GetPanel(), !IsPaneShown( pPane->GetPanel() ) );
				}
				return;
			}
			if ( ( nCommandID >= ID_VIEW_TOOLBAR_MAIN ) && ( nCommandID <= ID_VIEW_TOOLBAR_VIEW ) )
			{
				if ( NMainFrameWxPanes::CToolBar *const pToolBar = ViewToolBar( nCommandID ) )
				{
					pToolBar->Show( !pToolBar->IsVisible() );
				}
				return;
			}
			if ( ( nCommandID >= ID_FIRST_COMMAND_ID ) && ( nCommandID <= ID_LAST_COMMAND_ID ) )
			{
				NMainFrameShared::RunUserCommand( nCommandID );
				return;
			}
			rEvent.Skip();
		}

		// CFrameWnd::OnInitMenuPopup, which asks each item's handler for its state
		// when its menu opens. A popup is asked through its first item and changes
		// nothing about itself: only the recent lists' handler does anything
		// there, rebuilding the menu it heads.
		//
		// Only the frame's own menus: its menu bar's and the popups it shows itself.
		// wxEVT_MENU_OPEN propagates up from any window, so a view's context menu
		// -- the property grid's, say -- arrives here too; asking the frame's
		// handlers about its items disabled them all, and checking one that is not
		// a check item raised wx's assert and left the menu unshown. The view that
		// popped the menu up has already set its states. GetWindow() is the frame
		// for a menu bar's menus and the invoking window for a popup.
		void OnMenuOpen( wxMenuEvent &rEvent )
		{
			rEvent.Skip();
			wxMenu *const pMenu = rEvent.GetMenu();
			if ( ( pMenu == nullptr ) || ( pMenu->GetWindow() != this ) )
			{
				return;
			}
			if ( ( pMenu->GetMenuItemCount() > 0 ) && ( ToCommandID( pMenu->FindItemByPosition( 0 )->GetId() ) == ID_VIEW_DW_GDB_BROWSER_FIRST ) )
			{
				FillGDBBrowserMenu( pMenu );
			}
			for ( size_t nIndex = 0; nIndex < pMenu->GetMenuItemCount(); ++nIndex )
			{
				wxMenuItem *const pItem = pMenu->FindItemByPosition( nIndex );
				if ( pItem->IsSeparator() )
				{
					continue;
				}
				if ( wxMenu *const pSubMenu = pItem->GetSubMenu() )
				{
					if ( pSubMenu->GetMenuItemCount() > 0 )
					{
						unsigned nFirstID = 0;
						const unsigned nHeadID = ToCommandID( pSubMenu->FindItemByPosition( 0 )->GetId() );
						if ( const SUserData::CRecentList *pRecentList = NMainFrameShared::GetRecentList( nHeadID, &nFirstID ) )
						{
							FillRecentMenu( pSubMenu, *pRecentList, nFirstID );
						}
					}
					continue;
				}
				bool bEnable = false;
				bool bCheck = false;
				UpdateMenuCommand( ToCommandID( pItem->GetId() ), &bEnable, &bCheck );
				pItem->Enable( bEnable );
				// The frame's command items are all check items (InsertCommandItem),
				// but an item added some other way may not be, and wx asserts on
				// checking a plain one.
				if ( pItem->IsCheckable() )
				{
					pItem->Check( bCheck );
				}
			}
		}

		// A menu item's or toolbar button's state: CMainFrame's own handlers, then
		// the user command range. Customize, Stingray's toolbar editor, has no wx
		// counterpart and stays off.
		void UpdateMenuCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			switch ( nCommandID )
			{
				case ID_HELP_CONTENTS:
					( *pbEnable ) = NMainFrameShared::HasHelpFile( szHelpFilePath );
					return;
				case ID_HELP_ABOUT:
				case ID_APP_EXIT:
				case ID_VIEW_RESET_GUI:
					( *pbEnable ) = true;
					return;
				case ID_VIEW_DW_PROPERTY_BROWSER:
					( *pbEnable ) = true;
					( *pbCheck ) = pPropertiesPane && IsPaneShown( pPropertiesPane->GetPanel() );
					return;
				case ID_VIEW_DW_LOG:
					( *pbEnable ) = true;
					( *pbCheck ) = pLogPane && IsPaneShown( pLogPane->GetPanel() );
					return;
				case ID_VIEW_DW_GDB_BROWSER_NEW:
					( *pbEnable ) = gdbBrowserPanes.size() < static_cast<size_t>( ID_VIEW_DW_GDB_BROWSER_LAST - ID_VIEW_DW_GDB_BROWSER_FIRST );
					return;
				case ID_VIEW_DW_GDB_BROWSER_REMOVE:
					( *pbEnable ) = ( gdbBrowserPanes.size() > 1 ) && ( FindGDBBrowserPane( Singleton<IUserDataContainer>()->Get()->nFocusedGDBBrowserID ) != nullptr );
					return;
				default:
					break;
			}
			if ( ( nCommandID >= ID_VIEW_DW_GDB_BROWSER_FIRST ) && ( nCommandID <= ID_VIEW_DW_GDB_BROWSER_LAST ) )
			{
				const NMainFrameWxPanes::CGDBBrowserPane *const pPane = GDBBrowserPaneAt( nCommandID - ID_VIEW_DW_GDB_BROWSER_FIRST );
				( *pbEnable ) = ( pPane != nullptr );
				( *pbCheck ) = ( pPane != nullptr ) && IsPaneShown( pPane->GetPanel() );
				return;
			}
			if ( ( nCommandID >= ID_VIEW_TOOLBAR_MAIN ) && ( nCommandID <= ID_VIEW_TOOLBAR_VIEW ) )
			{
				const NMainFrameWxPanes::CToolBar *const pToolBar = ViewToolBar( nCommandID );
				( *pbEnable ) = ( pToolBar != nullptr );
				( *pbCheck ) = ( pToolBar != nullptr ) && pToolBar->IsVisible();
				return;
			}
			if ( ( nCommandID >= ID_FIRST_COMMAND_ID ) && ( nCommandID <= ID_LAST_COMMAND_ID ) )
			{
				NMainFrameShared::UpdateUserCommand( nCommandID, pbEnable, pbCheck );
			}
		}

		// The Game Database pane whose caption or border is at rPoint.
		NMainFrameWxPanes::CGDBBrowserPane* GDBBrowserPaneFramedAt( const wxPoint &rPoint )
		{
			const wxWindow *const pWindow = auiManager.PaneFrameAt( rPoint );
			if ( pWindow == nullptr )
			{
				return nullptr;
			}
			for ( const std::unique_ptr<NMainFrameWxPanes::CGDBBrowserPane> &rpPane : gdbBrowserPanes )
			{
				if ( rpPane->GetPanel() == pWindow )
				{
					return rpPane.get();
				}
			}
			return nullptr;
		}

		// CDWGDBBrowser::OnRButtonDown: a right click on a Game Database pane's
		// own caption or border makes that browser the focused one.
		void OnRightDown( wxMouseEvent &rEvent )
		{
			rEvent.Skip();
			if ( NMainFrameWxPanes::CGDBBrowserPane *const pPane = GDBBrowserPaneFramedAt( rEvent.GetPosition() ) )
			{
				SaveObjectStorage( pPane->GetContents().GetID() );
			}
		}

		// CDWGDBBrowser::OnRButtonUp: IDM_MAIN_CONTEXT_MENU's DW_GDB_BROWSER menu
		// -- Select Tables, Refresh Tables, Save All Tables, Register XDB -- where
		// the click was. Its items take their state as it opens, as the menu
		// bar's do, and choosing one goes to OnMenu.
		void OnRightUp( wxMouseEvent &rEvent )
		{
			rEvent.Skip();
			if ( GDBBrowserPaneFramedAt( rEvent.GetPosition() ) == nullptr )
			{
				return;
			}
			const HINSTANCE hInstance = AfxFindResourceHandle( MAKEINTRESOURCE( IDM_MAIN_CONTEXT_MENU ), RT_MENU );
			const HMENU hMenu = ::LoadMenuW( hInstance, MAKEINTRESOURCEW( IDM_MAIN_CONTEXT_MENU ) );
			if ( hMenu == 0 )
			{
				return;
			}
			std::unique_ptr<wxMenu> pMenu;
			if ( const HMENU hSubMenu = ::GetSubMenu( hMenu, MCMN_DW_GDB_BROWSER ) )
			{
				pMenu.reset( MenuFromNative( hSubMenu ) );
			}
			::DestroyMenu( hMenu );
			if ( pMenu )
			{
				PopupMenu( pMenu.get(), rEvent.GetPosition() );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
			}
		}

		void OnDropFiles( wxDropFilesEvent &rEvent )
		{
			// The first file only, as CMainFrame::OnDropFiles.
			if ( rEvent.GetNumberOfFiles() > 0 )
			{
				NMainFrameShared::OpenResource( std::string( rEvent.GetFiles()[0].utf8_str() ) );
			}
		}

		// The two messages CMainFrame answers that wx has no event for here.
		virtual WXLRESULT MSWWindowProc( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam ) override
		{
			switch ( nMsg )
			{
				case WM_COPYDATA:
				{
					// A second editor started with a file to open: see
					// CMapEditorSingletonChecker::OpenFileOnApp.
					const COPYDATASTRUCT *const pCopyData = reinterpret_cast<const COPYDATASTRUCT*>( lParam );
					if ( ( wParam == 0 ) && ( pCopyData != 0 ) && ( pCopyData->dwData == CMapEditorSingletonBase::OPEN_FILE ) )
					{
						NMainFrameShared::OpenResource( std::string( static_cast<const char*>( pCopyData->lpData ) ) );
						return TRUE;
					}
					break;
				}
				case WM_QUERYENDSESSION:
					if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, false ) )
					{
						return FALSE;
					}
					break;
				default:
					break;
			}
			return wxFrame::MSWWindowProc( nMsg, wParam, lParam );
		}
	};


	// wx owns the frame and destroys it when it closes; this goes null then.
	wxWeakRef<CWxMainFrame> s_pFrame;
}


namespace NMainFrameWx
{
	bool Create()
	{
		s_pFrame = NWx::TopLevel<CWxMainFrame>();
		// Before Build, which is where the editors make their controls and may
		// already ask for the main window.
		AfxGetApp()->m_pMainWnd = s_pFrame->GetMfcWindow();
		s_pFrame->Build();
		return true;
	}


	void Show()
	{
		if ( s_pFrame )
		{
			s_pFrame->ShowFrame();
		}
	}
}

#endif // OBK2_WITH_WX
