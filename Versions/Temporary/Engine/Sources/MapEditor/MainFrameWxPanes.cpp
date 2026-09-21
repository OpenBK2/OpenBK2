#include "stdafx.h"
#include "MapEditorLib/Resources.h"

#include "MainFrameWxPanes.h"
#include "NativeDockHint.h"

#include <set>
#include <tuple>
#include <vector>


#include <fmt/printf.h>
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxResourceImages.h"
#include "ResourceDefines.h"

#include <wx/icon.h>

#include <wx/settings.h>
#include <wx/sizer.h>

namespace NMainFrameWxPanes
{
	namespace
	{
		wxString LoadCaption( UINT nID )
		{
			std::string strCaption = NResources::GetString( nID );
			return wxString::FromUTF8( strCaption.c_str() );
		}


		// A panel whose one child fills it.
		wxPanel* CreateFilledPanel( wxWindow *pParent )
		{
			wxPanel *const pPanel = NWx::Child<wxPanel>( pParent, wxID_ANY );
			pPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
			return pPanel;
		}
	}


	CAuiManager::CAuiManager() = default;

	CAuiManager::~CAuiManager()
	{
		UnInit();
	}

	void CAuiManager::SetManagedWindow( wxWindow *pWindow )
	{
		UnInit();
		if ( pWindow == nullptr )
		{
			return;
		}
		wxAuiManager::SetManagedWindow( pWindow );
#ifdef __WXGTK3__
		// wxGTK's X11 overlay paints the frame, below the native 3D viewport.
		// Use a mouse-transparent popup for both docking and sash previews.
		// Bind after wxAUI so our sash drawing runs before its frame handlers.
		pWindow->Bind( wxEVT_MOTION, &CAuiManager::OnHintMotion, this );
		pWindow->Bind( wxEVT_LEFT_UP, &CAuiManager::OnHintLeftUp, this );
		pWindow->Bind( wxEVT_DESTROY, &CAuiManager::OnHintDestroy, this );
#endif
	}

	void CAuiManager::UnInit()
	{
#ifdef __WXGTK3__
		if ( wxWindow *const pWindow = GetManagedWindow() )
		{
			pWindow->Unbind( wxEVT_MOTION, &CAuiManager::OnHintMotion, this );
			pWindow->Unbind( wxEVT_LEFT_UP, &CAuiManager::OnHintLeftUp, this );
			pWindow->Unbind( wxEVT_DESTROY, &CAuiManager::OnHintDestroy, this );
		}
		pNativeHint.reset();
#endif
		wxAuiManager::UnInit();
	}

#ifdef __WXGTK3__
	void CAuiManager::ShowHint( const wxRect &rScreenRect )
	{
		if ( GetManagedWindow() == nullptr || rScreenRect.IsEmpty() )
		{
			HideHint();
			return;
		}
		if ( !pNativeHint )
		{
			pNativeHint = std::make_unique<CNativeDockHint>( GetManagedWindow() );
		}
		pNativeHint->Show( rScreenRect, false );
	}

	void CAuiManager::HideHint()
	{
		if ( pNativeHint )
		{
			pNativeHint->Hide();
		}
		wxAuiManager::HideHint();
	}

	void CAuiManager::OnHintMotion( wxMouseEvent &rEvent )
	{
		if ( m_action == actionResize && !HasLiveResize() && m_actionPart != nullptr )
		{
			// wxAUI draws sash hints directly into its overlay, bypassing ShowHint.
			// Keep its sash position calculation and let its release handler apply
			// the layout; only the temporary drawing uses our native popup.
			wxPoint pos = m_actionPart->rect.GetPosition();
			if ( m_actionPart->orientation == wxHORIZONTAL )
			{
				pos.y = (std::max)( 0, rEvent.GetY() - m_actionOffset.y );
			}
			else
			{
				pos.x = (std::max)( 0, rEvent.GetX() - m_actionOffset.x );
			}
			m_actionHintRect = wxRect( pos, m_actionPart->rect.GetSize() );
			if ( !pNativeHint )
			{
				pNativeHint = std::make_unique<CNativeDockHint>( GetManagedWindow() );
			}
			pNativeHint->Show( wxRect( m_frame->ClientToScreen( pos ), m_actionHintRect.GetSize() ), true );
			return;
		}
		rEvent.Skip();
	}

	void CAuiManager::OnHintLeftUp( wxMouseEvent &rEvent )
	{
		if ( m_action == actionResize )
		{
			HideHint();
		}
		rEvent.Skip();
	}

	void CAuiManager::OnHintDestroy( wxWindowDestroyEvent &rEvent )
	{
		if ( rEvent.GetEventObject() == GetManagedWindow() )
		{
			UnInit();
		}
		rEvent.Skip();
	}
#endif

	wxWindow* CAuiManager::PaneFrameAt( const wxPoint &rPoint )
	{
		const wxAuiDockUIPart *const pPart = HitTest( rPoint.x, rPoint.y );
		if ( ( pPart == nullptr ) || ( pPart->pane == nullptr ) )
		{
			return nullptr;
		}
		switch ( pPart->type )
		{
			case wxAuiDockUIPart::typeCaption:
			case wxAuiDockUIPart::typeGripper:
			case wxAuiDockUIPart::typePaneBorder:
				return pPart->pane->window;
			default:
				return nullptr;
		}
	}


	void CAuiManager::RememberDockSizes()
	{
		for ( wxAuiPaneInfo &rPane : m_panes )
		{
			if ( rPane.IsToolbar() || rPane.dock_direction == wxAUI_DOCK_CENTER )
			{
				continue;
			}
			for ( const wxAuiDockInfo &rDock : m_docks )
			{
				if ( rDock.dock_direction == rPane.dock_direction && rDock.dock_layer == rPane.dock_layer &&
						 rDock.dock_row == rPane.dock_row && rDock.size > 0 )
				{
					rPane.dock_size = rDock.size;
					break;
				}
			}
		}
	}


	wxString CAuiManager::SaveEditorLayout()
	{
		RememberDockSizes();
		wxString layout = SavePerspective();
		std::set<std::tuple<int, int, int>> docks;
		for ( const wxAuiDockInfo &rDock : m_docks )
		{
			docks.emplace( rDock.dock_direction, rDock.dock_layer, rDock.dock_row );
		}
		// SavePerspective omits docks whose panes are all hidden. Retain their
		// last sizes using the same format, compatible with existing layouts.
		for ( const wxAuiPaneInfo &rPane : m_panes )
		{
			if ( rPane.dock_size > 0 && !rPane.IsToolbar() && rPane.dock_direction != wxAUI_DOCK_CENTER &&
					 docks.emplace( rPane.dock_direction, rPane.dock_layer, rPane.dock_row ).second )
			{
				layout += wxString::Format( "dock_size(%d,%d,%d)=%d|", rPane.dock_direction,
															 rPane.dock_layer, rPane.dock_row, rPane.dock_size );
			}
		}
		return layout;
	}


	bool CAuiManager::LoadEditorLayout( const wxString &rLayout )
	{
		std::vector<wxAuiPaneInfo> before;
		for ( const wxAuiPaneInfo &rPane : m_panes )
		{
			before.push_back( rPane );
		}
		const wxAuiDockInfoArray previousDocks = m_docks;
		const bool bPreviouslyMaximized = m_hasMaximized;
		if ( !LoadPerspective( rLayout, false ) )
		{
			// wxAUI can reject a layout after partially applying it. Leave the
			// current arrangement intact if a saved layout cannot be read.
			for ( size_t nPane = 0; nPane < before.size(); ++nPane )
			{
				m_panes.Item( nPane ).SafeSet( before[nPane] );
			}
			m_docks = previousDocks;
			m_hasMaximized = bPreviouslyMaximized;
			return false;
		}
		for ( size_t nPane = 0; nPane < before.size(); ++nPane )
		{
			wxAuiPaneInfo &rPane = m_panes.Item( nPane );
			// Layouts from older builds may not know about new panes. Keep them
			// and current captions, rather than hiding or renaming them.
			if ( rLayout.Find( "name=" + before[nPane].name + ";" ) == wxNOT_FOUND )
			{
				rPane.SafeSet( before[nPane] );
			}
			rPane.Caption( before[nPane].caption );
		}
		// PostCreateControls hides inactive editor panes before the first
		// layout. Copy sizes now, before wxAUI removes their empty docks.
		RememberDockSizes();
		return true;
	}


	void CAuiManager::UpdateLater()
	{
		RememberDockSizes();
		wxWindow *const pFrame = GetManagedWindow();
		if ( ( pFrame == nullptr ) || bUpdatePending )
		{
			return;
		}
		bUpdatePending = true;
		// Queued on the frame, whose destruction drops it along with the manager.
		pFrame->CallAfter( [this]()
		{
			bUpdatePending = false;
			// The manager lets go of the frame as the frame closes, which can come
			// between the asking and the doing.
			if ( GetManagedWindow() != nullptr )
			{
				Update();
			}
		} );
	}


	wxAuiPaneInfo DockedPaneInfo( const wxString &rName, const std::string &rszTitle, unsigned nPlace, float fRate, int nWidth )
	{
		wxAuiPaneInfo info;
		info.Name( rName ).Caption( wxString::FromUTF8( rszTitle.c_str() ) ).CloseButton( true ).MaximizeButton( false );
		const bool bAcross = ( nPlace == NMainFrameBar::DOCK_TOP ) || ( nPlace == NMainFrameBar::DOCK_BOTTOM );
		switch ( nPlace )
		{
			case NMainFrameBar::DOCK_RIGHT:
				info.Right();
				break;
			case NMainFrameBar::DOCK_TOP:
				info.Top();
				break;
			case NMainFrameBar::DOCK_BOTTOM:
				info.Bottom();
				break;
			default:
				info.Left();
				break;
		}
		if ( bAcross )
		{
			info.BestSize( -1, nWidth );
		}
		else
		{
			info.BestSize( nWidth, -1 );
		}
		// wxAUI splits a side among its panes by proportion, 100000 each by
		// default; DockControlBarEx split it by rate.
		if ( fRate > 0.0f )
		{
			info.dock_proportion = static_cast<int>( fRate * 100000.0f );
		}
		return info;
	}


	// No wxTAB_TRAVERSAL: nothing in the panel is wx's to navigate between.
	bool CToolBarImages::AddToolBarResource( unsigned nResourceID )
	{
		// This was MFC's CToolBarData, read raw out of a RT_TOOLBAR block:
		// version, width, height, item count, then a command id per item.
		const NResources::SToolBarEntry *const pToolBar = NResources::GetToolBar( nResourceID );
		if ( pToolBar == nullptr )
		{
			return false;
		}
		const int nWidth = pToolBar->nWidth;
		const int nHeight = pToolBar->nHeight;
		const int nCount = static_cast<int>( pToolBar->nCount );
		if ( ( nWidth <= 0 ) || ( nHeight <= 0 ) )
		{
			return false;
		}
		wxImage strip = NWxResourceImages::LoadStrip( nResourceID );
		if ( !strip.IsOk() )
		{
			return false;
		}
		strip.SetMaskColour( 192, 192, 192 );
		int nImage = 0;
		for ( int nItem = 0; nItem < nCount; ++nItem )
		{
			const unsigned nCommandID = pToolBar->pCommands[nItem];
			if ( nCommandID == 0 )
			{
				continue;
			}
			const wxRect rect( nImage * nWidth, 0, nWidth, nHeight );
			if ( rect.GetRight() < strip.GetWidth() )
			{
				bitmaps[nCommandID] = wxBitmap( strip.GetSubImage( rect ) );
			}
			++nImage;
		}
		return true;
	}


	void CToolBarImages::AddIcon( unsigned nCommandID, unsigned nIconID )
	{
		// 16x16, the size the toolbars draw at; wx takes it from whichever image
		// in the .ico is nearest.
		const wxIconBundle icons = NWxResourceImages::LoadIconBundle( nIconID );
		const wxIcon icon = icons.GetIcon( wxSize( 16, 16 ) );
		if ( icon.IsOk() )
		{
			bitmaps[nCommandID] = wxBitmap( icon );
		}
	}


	wxBitmap CToolBarImages::Get( unsigned nCommandID ) const
	{
		const std::map<unsigned, wxBitmap>::const_iterator posBitmap = bitmaps.find( nCommandID );
		return ( posBitmap != bitmaps.end() ) ? posBitmap->second : wxBitmap();
	}


	void CToolBar::Show( bool bShow )
	{
		if ( pToolBar )
		{
			pManager->GetPane( pToolBar ).Show( bShow );
			if ( *pbLaidOut )
			{
				// Laid out once the editor has returned, as ShowControlBar's delay.
				pManager->UpdateLater();
			}
		}
	}


	bool CToolBar::IsVisible() const
	{
		return pToolBar && pManager->GetPane( pToolBar ).IsShown();
	}


	CContentPanel::CContentPanel( wxWindow *pParent )
		: wxPanel( pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxCLIP_CHILDREN )
	{
	}


	void CContentPanel::SetWxContents( wxWindow *pContents )
	{
		wxSizer *pSizer = GetSizer();
		if ( pSizer == nullptr )
		{
			pSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer( pSizer );
		}
		// The sizer does not own the windows in it; what was there stays a child.
		pSizer->Clear( false );
		pSizer->Add( pContents, wxSizerFlags( 1 ).Expand() );
		Layout();
	}


	// The panel's handle.
	void* CFrameWindow::GetNativeWidget()
	{
		return pPanel ? pPanel->GetHandle() : nullptr;
	}


	void CFrameWindow::Show( bool bShow )
	{
		if ( pPanel )
		{
			pPanel->Show( bShow );
			// The workspace's sizer leaves a hidden window out.
			if ( wxWindow *const pWorkspace = pPanel->GetParent() )
			{
				pWorkspace->Layout();
			}
		}
	}


	void CFrameWindow::Focus()
	{
		if ( pPanel )
		{
			// wxWindow::SetFocus rather than ::SetFocus on the handle: on MSW it is
			// that call, and it is the only one that also tells wx, which keeps its
			// own idea of the focused window for wxEVT_SET_FOCUS and for restoring
			// focus to a child when a top-level window is activated again.
			pPanel->SetFocus();
		}
	}


	void CFrameWindow::Destroy()
	{
		if ( pPanel )
		{
			wxWindow *const pWorkspace = pPanel->GetParent();
			pPanel->Destroy();
			if ( pWorkspace != nullptr )
			{
				pWorkspace->Layout();
			}
		}
	}


	// As CFrameWindow's.
	void* CDockPanel::GetNativeWidget()
	{
		return pPanel ? pPanel->GetHandle() : nullptr;
	}


	void CDockPanel::Show( bool bShow )
	{
		if ( pPanel )
		{
			pManager->GetPane( pPanel ).Show( bShow );
			if ( *pbLaidOut )
			{
				// Laid out once the editor has returned, as ShowControlBar's delay.
				pManager->UpdateLater();
			}
		}
	}


	void CDockPanel::ShowWithoutLayout( bool bShow )
	{
		if ( pPanel )
		{
			pPanel->Show( bShow );
		}
	}


	bool CDockPanel::IsVisible() const
	{
		return pPanel && pManager->GetPane( pPanel ).IsShown();
	}


	bool CDockPanel::IsAlive() const
	{
		return pPanel != nullptr;
	}


	void CDockPanel::Destroy()
	{
		if ( pPanel )
		{
			pManager->DetachPane( pPanel );
			pPanel->Destroy();
			pManager->Update();
		}
	}


	void CDockPanel::Redraw()
	{
		if ( pPanel )
		{
			pPanel->Refresh();
		}
	}


	bool CLogPane::Create( wxWindow *pFrame, wxAuiManager *pManager )
	{
		pPanel = CreateFilledPanel( pFrame );
		wxWindow *pWindow = nullptr;
		ILogView *const pView = NLogView::CreateWxLogViewIn( pPanel, &contents, &pWindow );
		if ( pView == nullptr )
		{
			return false;
		}
		contents.SetView( pView );
		pPanel->GetSizer()->Add( pWindow, wxSizerFlags( 1 ).Expand() );
		// MainFrame_Consts.cpp's third docking window.
		wxAuiPaneInfo info = DockedPaneInfo( "Log", std::string( LoadCaption( IDS_DW_LOG_NAME ).utf8_str() ), NMainFrameBar::DOCK_BOTTOM, 1.0f, 265 );
		pManager->AddPane( pPanel, info );
		return true;
	}


	CPropertiesPane::~CPropertiesPane()
	{
		delete pPropertyPane;
		pPropertyPane = 0;
	}


	bool CPropertiesPane::Create( wxWindow *pFrame, wxAuiManager *pManager, IWidget *pOwner )
	{
		pPanel = CreateFilledPanel( pFrame );
		wxWindow *pWindow = nullptr;
		// The label CMainFrame gives its pane, so the column widths are the same
		// file's.
		pPropertyPane = NPropertyPane::CreateWxIn( pPanel, pOwner, "CDWPropertyBrowser", &pWindow );
		if ( pPropertyPane == 0 )
		{
			return false;
		}
		pPanel->GetSizer()->Add( pWindow, wxSizerFlags( 1 ).Expand() );
		// MainFrame_Consts.cpp's second docking window, below every Game Database
		// pane, which CMainFrame docks first.
		wxAuiPaneInfo info = DockedPaneInfo( "SelectionProperties", std::string( LoadCaption( IDS_DW_PROPERTY_BROWSE_NAME ).utf8_str() ), NMainFrameBar::DOCK_LEFT, 0.5f, 265 );
		info.Position( ID_VIEW_DW_GDB_BROWSER_LAST - ID_VIEW_DW_GDB_BROWSER_FIRST + 1 );
		pManager->AddPane( pPanel, info );
		return true;
	}


	void CPropertiesPane::EnableEdit( bool bEnable )
	{
		if ( pPropertyPane != 0 )
		{
			pPropertyPane->EnableEdit( bEnable );
		}
	}


	bool CGDBBrowserPane::Create( wxWindow *pFrame, wxAuiManager *pManager, int nWindowIndex )
	{
		pPanel = CreateFilledPanel( pFrame );
		wxWindow *pBrowserWindow = nullptr;
		IObjectBrowser *const pBrowser = NObjectBrowser::CreateWxIn( pPanel, pOwner, &contents, IObjectBrowser::KIND_BROWSER,
																																	&pBrowserWindow, contents.GetID() );
		if ( pBrowser == nullptr )
		{
			return false;
		}
		// CEmptyGDBBrowser's face: nothing but the dialog colour.
		pEmpty = NWx::Child<wxPanel>( pPanel, wxID_ANY );
		pEmpty->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ) );
		pEmpty->Hide();
		pPanel->GetSizer()->Add( pBrowserWindow, wxSizerFlags( 1 ).Expand() );
		pPanel->GetSizer()->Add( pEmpty, wxSizerFlags( 1 ).Expand() );
		//
		std::string strCaption = NResources::GetString( IDS_DW_GDB_BROWSE_NAME );
		// MainFrame_Consts.cpp's first docking window.
		wxAuiPaneInfo info = DockedPaneInfo( wxString::Format( "GameDatabase%d", contents.GetID() ),
																				 fmt::sprintf( strCaption.c_str(), nWindowIndex ), NMainFrameBar::DOCK_LEFT, 0.5f, 265 );
		info.Position( nWindowIndex );
		pManager->AddPane( pPanel, info );
		contents.Start( pBrowser );
		return true;
	}


	void CGDBBrowserPane::LayoutContents()
	{
		if ( pPanel )
		{
			pPanel->Layout();
		}
	}


	void CGDBBrowserPane::ShowEmpty( bool bEmpty )
	{
		if ( pEmpty && ( pEmpty->IsShown() != bEmpty ) )
		{
			pEmpty->Show( bEmpty );
			LayoutContents();
		}
	}
}

