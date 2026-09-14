#include "stdafx.h"

#include "MainFrameWx.h"

#include <cstdlib>

namespace NMainFrameWx
{
	bool IsWanted()
	{
#ifdef OBK2_WITH_WX
		const char *const pszWanted = std::getenv( "OBK2_WX_FRAME" );
		return ( pszWanted != 0 ) && ( pszWanted[0] != '0' ) && ( pszWanted[0] != '\0' );
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

#include "MainFrameParams.h"
#include "MainFrameShared.h"
#include "MapEditorApp.h"
#include "MapEditorSingleton.h"

#include <wx/aui/framemanager.h>
#include <wx/dcclient.h>
#include <wx/iconbndl.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/statusbr.h>
#include <wx/weakref.h>

#include <climits>
#include <map>
#include <vector>

// The main frame, in wx. Slice one of the frame's move: the frame itself, its
// menus, status bar, title, command routing, placement and close; the docking
// panes, the document window and the toolbars are not here yet, and the
// IMainFrame calls for them answer "none" -- every caller already copes with
// that, because CMainFrame could fail to make them too.
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


	// What CFrameWnd shows in the status bar when nothing is highlighted.
	wxString IdleMessage()
	{
		CString strMessage;
		strMessage.LoadString( AFX_IDS_IDLEMESSAGE );
		return wxString::FromUTF8( strMessage.GetString() );
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
				// wx is given the label up to its tab and the native item gets the
				// whole of it back. wx makes the text after a tab a shortcut the
				// frame answers, where MFC's menu bar only shows it -- and read
				// "\t Ctrl+N" as plain N, so typing N anywhere made a new map.
				pMenu->AppendCheckItem( ToWxID( itemInfo.wID ), wxString( pszLabel ).BeforeFirst( '\t' ), CommandPrompt( itemInfo.wID ) );
				MENUITEMINFOW labelInfo = { sizeof( labelInfo ) };
				labelInfo.fMask = MIIM_STRING;
				labelInfo.dwTypeData = pszLabel;
				::SetMenuItemInfoW( pMenu->GetHMenu(), static_cast<UINT>( pMenu->GetMenuItemCount() - 1 ), TRUE, &labelInfo );
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
		wxAuiManager auiManager;
		// Every menu bar the application added, by resource id. The frame owns
		// the attached one while it is attached; this map owns them all.
		std::map<unsigned, wxMenuBar*> menuBars;
		std::string szHelpFilePath;
		CMapEditorSingletonApp mapEditorSingletonApp;
		SMainFrameParams params;
		SSWTParams currentSWTParams;
		NMainFrameShared::CProgressHost progress;

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
			// The area documents open in: an MDI client's colour until the
			// document window is here.
			auiManager.SetManagedWindow( this );
			wxPanel *const pWorkspace = NWx::Child<wxPanel>( this, wxID_ANY );
			pWorkspace->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
			auiManager.AddPane( pWorkspace, wxAuiPaneInfo().Name( "workspace" ).CenterPane() );
			//
			Singleton<IMainFrameContainer>()->Set( this, this );
			for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
			{
				pApp->GetEditorModules()[nModuleIndex]->ModuleCreateControls();
			}
			Singleton<IEditorContainer>()->CreateControls();
			for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
			{
				pApp->GetEditorModules()[nModuleIndex]->ModulePostCreateControls();
			}
			Singleton<IEditorContainer>()->PostCreateControls();
			auiManager.Update();
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
			Update();
		}

		// IWidget: what dialogs, message boxes and popup menus are owned by.
		// The MFC front-end's side of that is a CWnd, so it is the attached one.
		virtual void* GetNativeWidget()
		{
			return static_cast<CWnd*>( &mfcWindow );
		}

		// IMainFrame
		virtual bool GetToolBarButtonLeftBottomPos( const CTPoint<int> &rMousePoint, unsigned nButtonID, CTPoint<int> *pLeftBottomPos )
		{
			return false;
		}

		virtual IFrameWindow* CreateChildFrame( unsigned nResource )
		{
			NotYet( "the document window" );
			return 0;
		}

		virtual bool SetChildFrameWindowContents( IFrameWindow *pChildFrame, IWidget *pContents )
		{
			return false;
		}

		virtual IDockPanel* CreateControlBar( unsigned *pnID, const std::string &rszTitle, const unsigned nStyle, const unsigned nPlace, const float fRate, const int nWidth )
		{
			NotYet( rszTitle.c_str() );
			return 0;
		}

		virtual bool SetControlBarWindowContents( IDockPanel *pDockPanel, IWidget *pContents )
		{
			return false;
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

		virtual bool AddToolBarResource( const unsigned nStandartResourceID, const unsigned nLargeResourceID )
		{
			return false;
		}

		virtual void CreateToolBar( unsigned *pnID, const std::string &rszTitle, const unsigned nButtonCount, const unsigned *pButtonIDMap,
																const uint32_t dwAlignment, const unsigned nStyle, const bool bDocked, const bool bVisible, const bool bMainToolBar )
		{
			NotYet( rszTitle.c_str() );
		}

		virtual IToolBar* GetToolBar( unsigned nID )
		{
			return 0;
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

		// ILogger: the Log pane is not here yet.
		virtual void Log( ELogOutputType eLogOutputType, const std::string &szText )
		{
			DebugTrace( "%s", szText.c_str() );
		}

		virtual void ClearLog()
		{
		}

		virtual void SaveObjectStorage( int nGDBBrowserID )
		{
			Singleton<IUserDataContainer>()->Get()->nFocusedGDBBrowserID = nGDBBrowserID;
			RestoreObjectStorage();
		}

		virtual void RestoreObjectStorage()
		{
			// No Game Database pane to make the object storage yet.
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
				case ID_VIEW_SHOW_LOG:
				case ID_VIEW_SHOW_GDB_BROWSER:
					// The panes they show are not here yet.
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
		// CMainFrame::ReloadData tells each Game Database pane; there are none yet.
		void ReloadData()
		{
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
				default:
					break;
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
		void OnMenuOpen( wxMenuEvent &rEvent )
		{
			rEvent.Skip();
			wxMenu *const pMenu = rEvent.GetMenu();
			if ( pMenu == nullptr )
			{
				return;
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
				pItem->Check( bCheck );
			}
		}

		// A menu item's state: CMainFrame's own handlers, then the user command
		// range. The toolbar, pane, Reset GUI and Customize items belong to parts
		// of the frame that are not here yet, and stay off.
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
					( *pbEnable ) = true;
					return;
				default:
					break;
			}
			if ( ( nCommandID >= ID_FIRST_COMMAND_ID ) && ( nCommandID <= ID_LAST_COMMAND_ID ) )
			{
				NMainFrameShared::UpdateUserCommand( nCommandID, pbEnable, pbCheck );
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
