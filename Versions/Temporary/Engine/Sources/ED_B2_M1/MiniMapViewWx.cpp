#include "stdafx.h"

#include "MiniMapView.h"


#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "Input/GameMessage.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/menu.h>
#include <wx/sizer.h>

// The minimap in wx: a window the picture is stretched into, the view's
// outline drawn over it, the camera moved from it, and its one-command menu.
//
// Everything it computes is NMiniMapView's, shared with CMiniMapWindow; what is
// here is the drawing and the mouse, each as the MFC window did it:
//
//   * the picture is stretched over the whole window, the outline drawn twice
//     -- two pixels wide in dark green, then one pixel wide in bright green a
//     pixel left and two up;
//   * with no map, the window is filled and captioned "No map loaded" in the
//     colours the MFC window asked for, which are COLOR_BTNFACE and
//     COLOR_BTNTEXT's index numbers taken as colours -- all but black;
//   * a press, and a move with the button down, put the camera over that point
//     of the map, and a move repaints at once;
//   * the menu's command goes to the minimap's command handler, which hands it
//     to the map editor.

namespace
{
	class CMiniMapWxWindow : public CWxHostWindow, public ICommandHandler
	{
		wxWindow *pPanel = nullptr;
		NMiniMapView::SImage image;
		wxBitmap mapBitmap;
		wxSize editorSize;
		bool bRegistered = false;

	public:
		virtual ~CMiniMapWxWindow()
		{
			Unregister();
		}

		// CMiniMapWindow::Create: the handler first, then the window.
		bool Build( IWidget *pPane )
		{
			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pContainer->Set( CHID_MAPINFO_MINIMAP_WINDOW, this );
			pContainer->Register( CHID_MAPINFO_MINIMAP_WINDOW, ID_MIMCO_GENERATE_MINIMAP_IMAGE, ID_MIMCO_GENERATE_MINIMAP_IMAGE );
			bRegistered = true;
			if ( !CreateHost( pPane ) )
			{
				return false;
			}
			pPanel = NWx::Child<wxWindow>( Root(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE );
			pPanel->SetBackgroundStyle( wxBG_STYLE_PAINT );
			wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pPanel, wxSizerFlags( 1 ).Expand() );
			Root()->SetSizer( pSizer );
			pPanel->Bind( wxEVT_PAINT, &CMiniMapWxWindow::OnPaint, this );
			pPanel->Bind( wxEVT_LEFT_DOWN, &CMiniMapWxWindow::OnLeftDown, this );
			pPanel->Bind( wxEVT_MOTION, &CMiniMapWxWindow::OnMotion, this );
			pPanel->Bind( wxEVT_CONTEXT_MENU, &CMiniMapWxWindow::OnContextMenu, this );
			pPanel->Bind( wxEVT_DESTROY, [this]( wxWindowDestroyEvent &rEvent )
			{
				rEvent.Skip();
				if ( rEvent.GetEventObject() == pPanel )
				{
					pPanel = nullptr;
				}
			} );
			return true;
		}

		void Unregister()
		{
			if ( bRegistered )
			{
				Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_MINIMAP_WINDOW );
				Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_MINIMAP_WINDOW );
				bRegistered = false;
			}
		}

		// CMiniMapWindow::LoadMap, which did not repaint.
		void LoadMap( const NDb::STerrain *pTerrainDesc )
		{
			if ( pPanel == nullptr )
			{
				return;
			}
			mapBitmap = wxNullBitmap;
			if ( !NMiniMapView::BuildImage( pTerrainDesc, &image ) )
			{
				return;
			}
			wxImage picture( image.nSide, image.nSide, false );
			unsigned char *const pRGB = picture.GetData();
			for ( size_t nPixel = 0; nPixel < image.pixels.size(); ++nPixel )
			{
				const uint32_t nValue = image.pixels[nPixel];
				pRGB[nPixel * 3 + 0] = static_cast<unsigned char>( ( nValue >> 16 ) & 0xFF );
				pRGB[nPixel * 3 + 1] = static_cast<unsigned char>( ( nValue >> 8 ) & 0xFF );
				pRGB[nPixel * 3 + 2] = static_cast<unsigned char>( nValue & 0xFF );
			}
			mapBitmap = wxBitmap( picture );
		}

		void SetMapInfoEditorSize( int nSizeX, int nSizeY )
		{
			editorSize = wxSize( nSizeX, nSizeY );
			if ( pPanel != nullptr )
			{
				pPanel->Refresh();
			}
		}

		void Redraw()
		{
			if ( pPanel != nullptr )
			{
				pPanel->Refresh();
				pPanel->Update();
			}
		}

		void Update()
		{
			if ( pPanel != nullptr )
			{
				pPanel->Update();
			}
		}

		// ICommandHandler: CMiniMapWindow's, which passes the one command on.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			if ( nCommandID == ID_MIMCO_GENERATE_MINIMAP_IMAGE )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAP_INFO_EDITOR, ID_MIMCO_GENERATE_MINIMAP_IMAGE, 0 );
				return true;
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			if ( ( nCommandID == ID_MIMCO_GENERATE_MINIMAP_IMAGE ) && ( pbEnable != 0 ) && ( pbCheck != 0 ) )
			{
				( *pbEnable ) = true;
				( *pbCheck ) = false;
				return true;
			}
			return false;
		}

	private:
		void OnPaint( wxPaintEvent & )
		{
			wxAutoBufferedPaintDC dc( pPanel );
			const wxSize size = pPanel->GetClientSize();
			dc.SetPen( *wxTRANSPARENT_PEN );
			dc.SetBrush( *wxBLACK_BRUSH );
			dc.DrawRectangle( 0, 0, size.x, size.y );
			if ( mapBitmap.IsOk() && !image.IsEmpty() )
			{
				wxMemoryDC source( mapBitmap );
#ifdef __WXMSW__
				// BLACKONWHITE, the stretch mode a new DC starts in and so the one
				// the MFC window shrank the picture with: it keeps the darker of the
				// pixels it drops, and a map a thousand pixels across shrinks a lot.
				// wxDC::StretchBlit sets COLORONCOLOR, which keeps whichever pixel
				// it lands on, and small dark marks came out another colour.
				const HDC hTarget = static_cast<HDC>( dc.GetHDC() );
				const int nOldMode = ::SetStretchBltMode( hTarget, BLACKONWHITE );
				::StretchBlt( hTarget, 0, 0, size.x, size.y, static_cast<HDC>( source.GetHDC() ), 0, 0, image.nSide, image.nSide, SRCCOPY );
				::SetStretchBltMode( hTarget, nOldMode );
#else
				dc.StretchBlit( 0, 0, size.x, size.y, &source, 0, 0, image.nSide, image.nSide );
#endif
				// The view's corners, through the camera, as a closed outline.
				const CVec2 corners[4] = { CVec2( 0, 0 ), CVec2( editorSize.x, 0 ), CVec2( editorSize.x, editorSize.y ), CVec2( 0, editorSize.y ) };
				wxPoint outline[5];
				for ( int nCorner = 0; nCorner < 4; ++nCorner )
				{
					const CVec2 vPoint = NMiniMapView::EditorToMiniMap( image, size.x, size.y, corners[nCorner] );
					outline[nCorner] = wxPoint( static_cast<int>( vPoint.x ), static_cast<int>( vPoint.y ) );
				}
				outline[4] = outline[0];
				dc.SetPen( wxPen( wxColour( 64, 128, 0 ), 2 ) );
				dc.DrawLines( 5, outline );
				for ( int nPoint = 0; nPoint < 5; ++nPoint )
				{
					outline[nPoint].x -= 1;
					outline[nPoint].y -= 2;
				}
				dc.SetPen( wxPen( wxColour( 0, 255, 0 ), 1 ) );
				dc.DrawLines( 5, outline );
				return;
			}
			dc.SetBrush( wxBrush( wxColour( 15, 0, 0 ) ) );
			dc.DrawRectangle( 0, 0, size.x, size.y );
			dc.SetFont( wxFont( wxFontInfo( wxSize( 0, 20 ) ).Bold() ) );
			dc.SetTextForeground( wxColour( 18, 0, 0 ) );
			const wxString caption( "No map loaded" );
			// TA_CENTER: centred across, its top at the middle.
			dc.DrawText( caption, ( size.x - dc.GetTextExtent( caption ).x ) / 2, size.y / 2 );
		}

		void MoveCamera( const wxPoint &rAt )
		{
			if ( image.IsEmpty() )
			{
				return;
			}
			const wxSize size = pPanel->GetClientSize();
			const CVec2 vPosition = NMiniMapView::MiniMapToEditor( image, size.x, size.y, CVec2( rAt.x, rAt.y ) );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_CAMERA_POSITION, PackCoords( vPosition ) );
		}

		void OnLeftDown( wxMouseEvent &rEvent )
		{
			rEvent.Skip();
			MoveCamera( rEvent.GetPosition() );
		}

		void OnMotion( wxMouseEvent &rEvent )
		{
			rEvent.Skip();
			if ( rEvent.LeftIsDown() )
			{
				MoveCamera( rEvent.GetPosition() );
				Redraw();
			}
		}

		// IDM_MAPINFO_CONTEXT_MENU's MI_MINIMAP popup. The MFC window tracked the
		// resource menu with the frame as its owner, which routed the command
		// back here through the container; this asks the container directly.
		void OnContextMenu( wxContextMenuEvent &rEvent )
		{
			wxMenu menu;
			bool bEnable = false;
			bool bCheck = false;
			UpdateCommand( ID_MIMCO_GENERATE_MINIMAP_IMAGE, &bEnable, &bCheck );
			menu.Append( ID_MIMCO_GENERATE_MINIMAP_IMAGE, "&Generate Minimap Image ..." )->Enable( bEnable );
			menu.Bind( wxEVT_MENU, []( wxCommandEvent & )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MINIMAP_WINDOW, ID_MIMCO_GENERATE_MINIMAP_IMAGE, 0 );
			}, ID_MIMCO_GENERATE_MINIMAP_IMAGE );
			const wxPoint at = rEvent.GetPosition();
			pPanel->PopupMenu( &menu, ( at == wxDefaultPosition ) ? wxDefaultPosition : pPanel->ScreenToClient( at ) );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
	};


	class CWxMiniMapView : public NMiniMapView::IView
	{
		CMiniMapWxWindow window;
		bool bCreated = false;

	public:
		virtual ~CWxMiniMapView()
		{
			Destroy();
		}

		virtual bool Create( IWidget *pPane )
		{
			bCreated = window.Build( pPane );
			return bCreated;
		}

		virtual void Destroy()
		{
			window.Unregister();
			if ( bCreated )
			{
				window.DestroyHost();
				bCreated = false;
			}
		}

		virtual void Show( bool bShow )
		{
			if ( bCreated )
			{
				window.ShowHost( bShow );
			}
		}

		virtual IWidget* GetWidget()
		{
			return bCreated ? &window : 0;
		}

		virtual void LoadMap( const NDb::STerrain *pTerrainDesc )
		{
			window.LoadMap( pTerrainDesc );
		}

		virtual void SetMapInfoEditorSize( int nSizeX, int nSizeY )
		{
			window.SetMapInfoEditorSize( nSizeX, nSizeY );
		}

		virtual void Redraw()
		{
			window.Redraw();
		}

		virtual void Update()
		{
			window.Update();
		}
	};
}


namespace NMiniMapView
{
	IView* CreateWx()
	{
		return new CWxMiniMapView;
	}
}

