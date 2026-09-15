#include "stdafx.h"

#include "MainFrameWxPanes.h"

#ifdef OBK2_WITH_WX

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
			CString strCaption;
			strCaption.LoadString( nID );
			return wxString::FromUTF8( strCaption.GetString() );
		}


		// A panel whose one child fills it.
		wxPanel* CreateFilledPanel( wxWindow *pParent )
		{
			wxPanel *const pPanel = NWx::Child<wxPanel>( pParent, wxID_ANY );
			pPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
			return pPanel;
		}
	}


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


	wxAuiPaneInfo DockedPaneInfo( const wxString &rName, const std::string &rszTitle, unsigned nPlace, float fRate, int nWidth )
	{
		wxAuiPaneInfo info;
		info.Name( rName ).Caption( wxString::FromUTF8( rszTitle.c_str() ) ).CloseButton( true ).MaximizeButton( false );
		const bool bAcross = ( nPlace == AFX_IDW_DOCKBAR_TOP ) || ( nPlace == AFX_IDW_DOCKBAR_BOTTOM );
		switch ( nPlace )
		{
			case AFX_IDW_DOCKBAR_RIGHT:
				info.Right();
				break;
			case AFX_IDW_DOCKBAR_TOP:
				info.Top();
				break;
			case AFX_IDW_DOCKBAR_BOTTOM:
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
		const HINSTANCE hInstance = AfxFindResourceHandle( MAKEINTRESOURCE( nResourceID ), RT_TOOLBAR );
		// The A form by name: wx's Windows headers make FindResource the wide one.
		const HRSRC hResource = ::FindResourceA( hInstance, MAKEINTRESOURCEA( nResourceID ), RT_TOOLBAR );
		if ( hResource == 0 )
		{
			return false;
		}
		const HGLOBAL hData = ::LoadResource( hInstance, hResource );
		const WORD *const pData = ( hData != 0 ) ? static_cast<const WORD*>( ::LockResource( hData ) ) : nullptr;
		const DWORD nSize = ::SizeofResource( hInstance, hResource );
		if ( ( pData == nullptr ) || ( nSize < 4 * sizeof( WORD ) ) )
		{
			return false;
		}
		// CToolBarData: version, width, height, item count, then the items, a
		// command id each, 0 for a separator.
		const int nWidth = pData[1];
		const int nHeight = pData[2];
		const int nCount = pData[3];
		if ( ( nWidth <= 0 ) || ( nHeight <= 0 ) || ( nSize < ( 4 + nCount ) * sizeof( WORD ) ) )
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
			const unsigned nCommandID = pData[4 + nItem];
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
		const HINSTANCE hInstance = AfxFindResourceHandle( MAKEINTRESOURCE( nIconID ), RT_GROUP_ICON );
		const HICON hIcon = static_cast<HICON>( ::LoadImage( hInstance, MAKEINTRESOURCE( nIconID ), IMAGE_ICON, 16, 16, 0 ) );
		if ( hIcon == 0 )
		{
			return;
		}
		wxIcon icon;
		// The icon takes the handle and destroys it.
		if ( icon.CreateFromHICON( hIcon ) )
		{
			bitmaps[nCommandID] = wxBitmap( icon );
		}
		else
		{
			::DestroyIcon( hIcon );
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
				pManager->Update();
			}
		}
	}


	bool CToolBar::IsVisible() const
	{
		return pToolBar && pManager->GetPane( pToolBar ).IsShown();
	}


	CMfcPanel::CMfcPanel( wxWindow *pParent )
		: wxPanel( pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxCLIP_CHILDREN ),
			hwndContents( 0 )
	{
		mfcWindow.SubclassWindow( GetHWND() );
		Bind( wxEVT_SIZE, &CMfcPanel::OnSize, this );
	}


	CMfcPanel::~CMfcPanel()
	{
		// While the handle stands, and before CWnd's destructor, which would
		// destroy it: wx's window procedure goes back on it.
		if ( mfcWindow.GetSafeHwnd() != 0 )
		{
			mfcWindow.UnsubclassWindow();
		}
	}


	void CMfcPanel::SetContents( HWND _hwndContents )
	{
		hwndContents = _hwndContents;
		FitContents();
	}


	void CMfcPanel::FitContents()
	{
		if ( ( hwndContents != 0 ) && ::IsWindow( hwndContents ) )
		{
			const wxSize size = GetClientSize();
			::SetWindowPos( hwndContents, 0, 0, 0, size.x, size.y, SWP_NOZORDER | SWP_NOACTIVATE );
		}
	}


	void CMfcPanel::OnSize( wxSizeEvent &rEvent )
	{
		rEvent.Skip();
		FitContents();
	}


	void* CFrameWindow::GetNativeWidget()
	{
		return pPanel ? static_cast<CWnd*>( pPanel->GetMfcWindow() ) : nullptr;
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
			::SetFocus( pPanel->GetHWND() );
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


	void* CDockPanel::GetNativeWidget()
	{
		return pPanel ? static_cast<CWnd*>( pPanel->GetMfcWindow() ) : nullptr;
	}


	void CDockPanel::Show( bool bShow )
	{
		if ( pPanel )
		{
			pManager->GetPane( pPanel ).Show( bShow );
			if ( *pbLaidOut )
			{
				pManager->Update();
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
		wxAuiPaneInfo info = DockedPaneInfo( "Log", std::string( LoadCaption( IDS_DW_LOG_NAME ).utf8_str() ), AFX_IDW_DOCKBAR_BOTTOM, 1.0f, 265 );
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
		wxAuiPaneInfo info = DockedPaneInfo( "SelectionProperties", std::string( LoadCaption( IDS_DW_PROPERTY_BROWSE_NAME ).utf8_str() ), AFX_IDW_DOCKBAR_LEFT, 0.5f, 265 );
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
		CString strCaption;
		strCaption.LoadString( IDS_DW_GDB_BROWSE_NAME );
		// MainFrame_Consts.cpp's first docking window.
		wxAuiPaneInfo info = DockedPaneInfo( wxString::Format( "GameDatabase%d", contents.GetID() ),
																				 fmt::sprintf( strCaption.GetString(), nWindowIndex ), AFX_IDW_DOCKBAR_LEFT, 0.5f, 265 );
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

#endif // OBK2_WITH_WX
