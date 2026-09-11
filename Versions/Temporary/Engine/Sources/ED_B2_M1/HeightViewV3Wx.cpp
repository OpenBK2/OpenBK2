#include "stdafx.h"

#include "HeightViewV3.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "HeightStateV3.h"
#include "ResourceDefines.h"
#include "ED_B2_M1Dll.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Interface_ObjectCollector.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/bitmap.h>
#include <wx/checkbox.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/tglbtn.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include <commctrl.h>

#include <string>
#include <vector>

// The terrain height palette, in wx: the seventh palette, and the first with
// anything in it beyond plain controls.
//
// Three things here are new, and each of them is the reason this one took
// longer than the six before it.
//
// **Twelve icon buttons cut out of one bitmap.** The template's brush, brush
// size and brush type rows are BS_AUTORADIOBUTTON | BS_ICON | BS_PUSHLIKE --
// radio buttons drawn as a toolbar. wx has no such control: wxRadioButton is
// always a dot and a label. wxBitmapToggleButton looks right and behaves
// wrongly, in that nothing stops all of them being off or all of them being on,
// so the group behaviour is done here, in SelectOnly, and it is four lines.
//
// **A list view the object collector fills.** The tile list is a real
// SysListView32 in both palettes, so wxListCtrl is a like-for-like replacement
// and most of this file's list code is the MFC helper templates in
// EditParameter.h rewritten against wx's spelling of the same calls. The one
// thing that is not like-for-like is where the icons come from; see
// AttachTileIcons.
//
// **A context menu.** The MFC palette loads IDM_MAPINFO_CONTEXT_MENU and lets
// MFC's update-command routing decide what is checked and enabled. Here the
// same three items are built in place, and what to check and enable is read
// from the same state that UpdateCommand reads. Fewer moving parts for an
// identical menu.
//
// Everything else follows FieldViewWx: the same edit-parameter flag discipline,
// the same bCreateControls guard, the same scrolled window.

namespace
{
	typedef CHeightStateV3::SEditParameters SEditParams;

	const int BRUSH_COUNT = 5;
	const int SIZE_COUNT = 5;
	const int TYPE_COUNT = 2;

	// GetHeightTimerInterval(), which the MFC palette spells as a member
	// function returning a literal.
	const int HEIGHT_TIMER_INTERVAL = 100;


	class CHeightWxWindowV3 : public CWxHostWindow, public CHeightCommandsV3
	{
		// wxTimer with a Notify() rather than an owner and a bound event: the
		// palette is not a wxEvtHandler, and this way the timer does not point
		// at any wx window, so it cannot outlive one.
		class CHeightTimer : public wxTimer
		{
			CHeightWxWindowV3 *pOwner;

		public:
			explicit CHeightTimer( CHeightWxWindowV3 *_pOwner ) : pOwner( _pOwner ) {}
			virtual void Notify() { pOwner->OnHeightTimer(); }
		};

		// Ids for the tile list's context menu; local to it, and never seen by
		// the editor's own command routing.
		enum EMenuItem
		{
			MENU_LIST = wxID_HIGHEST + 1,
			MENU_THUMBNAILS,
			MENU_PROPERTIES,
		};

		wxBitmapToggleButton *pBrush[BRUSH_COUNT] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		wxBitmapToggleButton *pSize[SIZE_COUNT] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		wxBitmapToggleButton *pType[TYPE_COUNT] = { nullptr, nullptr };
		wxCheckBox *pFixCliffs = nullptr;
		wxListCtrl *pTiles = nullptr;

		// The whole strip, cut up once. The size buttons wear two of these
		// sets and swap between them, which is why they are all kept.
		wxBitmap icons[TMITH_COUNT];

		CHeightTimer heightTimer;
		uint32_t dwHeightData = 0;

		// The same guard the MFC palette has, under the same name: filling the
		// controls raises the change events the user does, and those must not
		// be reported back as edits.
		bool bCreateControls = false;
		// LVS_ICON as against LVS_LIST, which is all nStyle ever held there.
		// True to start with because OnInitDialog's SetTileListStyle( LVS_ICON )
		// is: the state is asked for the real value straight afterwards, but
		// until it answers, a palette that says it is showing thumbnails has to
		// be showing them.
		bool bThumbnails = true;
		// The tile the palette would go back to when the brush returns to
		// B_TILE, as a list index.
		int nLastIndex = -1;
		// The names behind the list, which shows labels. ID_MITHV3_PROPERTIES
		// needs the name.
		std::vector<std::string> tileList;

	public:
		CHeightWxWindowV3() : heightTimer( this )
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, this );
			Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_LIST, ID_MITHV3_PROPERTIES );
		}

		virtual ~CHeightWxWindowV3()
		{
			heightTimer.Stop();
			Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			LoadIcons();

			// Scrolled for the same reason the field palette is: this is one
			// column of fixed-height rows, the shortcut bar's tabs are not
			// always tall enough for all of them, and a plain sizer answers a
			// short window by squashing controls below their minimum. Here the
			// tile list absorbs the slack when there is any -- it is the only
			// item with a proportion -- and when there is not, the rows keep
			// their size and the palette scrolls.
			wxScrolledWindow *const pRoot = CreateScrolledRoot();

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// The tooltips are the strings the resource file keeps under each
			// control's own id, which is how MFC's EnableToolTips finds them.
			// Spelled out here because wx has no equivalent lookup and because
			// the wx palette should not need this module's string table.
			static const struct { int nIcon; const char *pszTip; } BRUSHES[BRUSH_COUNT] =
			{
				{ TMITH_TILE,		"Fill Terrain" },
				{ TMITH_UP,			"Up Terrain Height" },
				{ TMITH_DOWN,		"Low Terrain Height" },
				{ TMITH_ROUND,	"Round Terrain Height" },
				{ TMITH_PLATO,	"Make Plato" },
			};
			static const char *const SIZE_TIPS[SIZE_COUNT] =
			{
				"Very Small Brush", "Small Brush", "Medium Brush", "Large Brush", "Very Large Brush"
			};
			static const struct { int nIcon; const char *pszTip; } TYPES[TYPE_COUNT] =
			{
				{ TMITH_CIRCLE,	"Circle Brush" },
				{ TMITH_SQUARE,	"Square Brush" },
			};

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Brush:" ),
									 wxSizerFlags().Expand() );
			wxBoxSizer *pBrushRow = new wxBoxSizer( wxHORIZONTAL );
			for ( int nBrush = 0; nBrush < BRUSH_COUNT; ++nBrush )
			{
				pBrush[nBrush] = AddIconButton( pRoot, pBrushRow, icons[BRUSHES[nBrush].nIcon],
																				BRUSHES[nBrush].pszTip );
				pBrush[nBrush]->Bind( wxEVT_TOGGLEBUTTON, &CHeightWxWindowV3::OnBrush, this );
			}
			pSizer->Add( pBrushRow, wxSizerFlags().Border( wxTOP, 2 ) );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Brush Size:" ),
									 wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			wxBoxSizer *pSizeRow = new wxBoxSizer( wxHORIZONTAL );
			for ( int nSize = 0; nSize < SIZE_COUNT; ++nSize )
			{
				pSize[nSize] = AddIconButton( pRoot, pSizeRow, icons[TMITH_BRUSH_SIZE_C0 + nSize],
																			SIZE_TIPS[nSize] );
				pSize[nSize]->Bind( wxEVT_TOGGLEBUTTON, &CHeightWxWindowV3::OnBrushSize, this );
			}
			pSizer->Add( pSizeRow, wxSizerFlags().Border( wxTOP, 2 ) );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Brush Type:" ),
									 wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			// "Fix Cliffs" sits beside the two brush type buttons in the
			// template rather than under them, so it shares their row.
			wxBoxSizer *pTypeRow = new wxBoxSizer( wxHORIZONTAL );
			for ( int nType = 0; nType < TYPE_COUNT; ++nType )
			{
				pType[nType] = AddIconButton( pRoot, pTypeRow, icons[TYPES[nType].nIcon],
																			TYPES[nType].pszTip );
				pType[nType]->Bind( wxEVT_TOGGLEBUTTON, &CHeightWxWindowV3::OnBrushType, this );
			}
			pFixCliffs = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Fix Cliffs" );
			pFixCliffs->Bind( wxEVT_CHECKBOX, &CHeightWxWindowV3::OnFixCliffs, this );
			pTypeRow->Add( pFixCliffs, wxSizerFlags().Centre().Border( wxLEFT, 8 ) );
			pSizer->Add( pTypeRow, wxSizerFlags().Border( wxTOP, 2 ) );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Terrain Tile:" ),
									 wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			// LVS_SINGLESEL and LVS_SHOWSELALWAYS from the template; wx adds
			// the second of those to every list control it makes, along with
			// LVS_SHAREIMAGELISTS, which is what keeps it from destroying the
			// borrowed image lists below.
			pTiles = NWx::Child<wxListCtrl>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																			 wxLC_ICON | wxLC_SINGLE_SEL | wxBORDER_SUNKEN );
			// A minimum so that the palette asks for roughly the height its
			// template does; above it the list takes whatever the tab has.
			pTiles->SetMinSize( wxSize( -1, 120 ) );
			pSizer->Add( pTiles, wxSizerFlags( 1 ).Expand().Border( wxTOP, 2 ) );
			AttachTileIcons();
			pTiles->Bind( wxEVT_LIST_ITEM_SELECTED, &CHeightWxWindowV3::OnTileSelected, this );
			pTiles->Bind( wxEVT_CONTEXT_MENU, &CHeightWxWindowV3::OnTileContextMenu, this );
			// UpdateTileListStyle(), which the MFC palette runs from OnSize:
			// the icon view has to be told to re-flow when its width changes.
			pTiles->Bind( wxEVT_SIZE, &CHeightWxWindowV3::OnTileListSize, this );

			pRoot->SetSizer( pSizer );
			pRoot->FitInside();

			// The state's default, and the same call OnInitDialog makes before
			// anything has been exchanged.
			UpdateSizeButtons( SEditParams::BT_CIRCLE );
			ApplyListStyle();
			return true;
		}

		//	CHeightCommandsV3
		virtual bool GetEditParameters( SEditParams *pEditParameters )
		{
			if ( pEditParameters == 0 )
			{
				return false;
			}
			if ( pEditParameters->nFlags & MITHV3EP_BRUSH )
			{
				pEditParameters->eBrush = CheckedBrush();
			}
			if ( pEditParameters->nFlags & MITHV3EP_BRUSH_SIZE )
			{
				pEditParameters->eBrushSize = CheckedBrushSize();
			}
			if ( pEditParameters->nFlags & MITHV3EP_BRUSH_TYPE )
			{
				pEditParameters->eBrushType = CheckedBrushType();
			}
			if ( pEditParameters->nFlags & ( MITHV3EP_TILE_COUNT | MITHV3EP_TILE_INDEX ) )
			{
				ReadTileList( &( pEditParameters->tileList ), &( pEditParameters->nTileIndex ),
											( pEditParameters->nFlags & MITHV3EP_TILE_COUNT ) != 0,
											( pEditParameters->nFlags & MITHV3EP_TILE_INDEX ) != 0 );
				nLastIndex = pEditParameters->nTileIndex;
			}
			if ( ( pEditParameters->nFlags & MITHV3EP_THUMBNAILS ) > 0 )
			{
				pEditParameters->bThumbnails = bThumbnails;
			}
			if ( pEditParameters->nFlags & MITHV3EP_UPDATE_HEIGHT )
			{
				pEditParameters->bUpdateHeight = pFixCliffs->GetValue();
			}
			return true;
		}

		virtual bool SetEditParameters( const SEditParams &rEditParameters )
		{
			bCreateControls = true;
			if ( rEditParameters.nFlags & MITHV3EP_BRUSH )
			{
				SelectOnly( pBrush, BRUSH_COUNT, rEditParameters.eBrush );
			}
			if ( rEditParameters.nFlags & MITHV3EP_BRUSH_SIZE )
			{
				SelectOnly( pSize, SIZE_COUNT, rEditParameters.eBrushSize );
			}
			if ( rEditParameters.nFlags & MITHV3EP_BRUSH_TYPE )
			{
				SelectOnly( pType, TYPE_COUNT, rEditParameters.eBrushType );
				UpdateSizeButtons( rEditParameters.eBrushType );
			}
			if ( rEditParameters.nFlags & ( MITHV3EP_TILE_COUNT | MITHV3EP_TILE_INDEX ) )
			{
				nLastIndex = rEditParameters.nTileIndex;
				WriteTileList( rEditParameters.tileList, rEditParameters.nTileIndex,
											 ( rEditParameters.nFlags & MITHV3EP_TILE_COUNT ) != 0,
											 ( rEditParameters.nFlags & MITHV3EP_TILE_INDEX ) != 0 );
			}
			if ( ( rEditParameters.nFlags & MITHV3EP_THUMBNAILS ) > 0 )
			{
				SetThumbnails( rEditParameters.bThumbnails );
			}
			if ( rEditParameters.nFlags & MITHV3EP_UPDATE_HEIGHT )
			{
				pFixCliffs->SetValue( rEditParameters.bUpdateHeight );
			}
			bCreateControls = false;
			return true;
		}

		//	ICommandHandler
		//
		// The five commands this palette answers beyond the edit-parameter
		// pair: the two the state drives its timer with, and the tile list's
		// three context menu items, which the main frame's menu can also send.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			switch ( nCommandID )
			{
				case ID_MITHV3_SET_TIMER:
					dwHeightData = static_cast<uint32_t>( dwData );
					heightTimer.Start( HEIGHT_TIMER_INTERVAL );
					return true;
				case ID_MITHV3_KILL_TIMER:
					heightTimer.Stop();
					return true;
				case ID_MITHV3_LIST:
					SetThumbnails( false );
					return true;
				case ID_MITHV3_THUMBNAILS:
					SetThumbnails( true );
					return true;
				case ID_MITHV3_PROPERTIES:
					if ( ShowSelectedTileProperties() )
					{
						return true;
					}
					// Nothing selected: falls through to the shared dispatch,
					// which answers false, exactly as the MFC palette does.
					// fall through
				default:
					return CHeightCommandsV3::HandleCommand( nCommandID, dwData );
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			NI_ASSERT( pbEnable != 0, "CHeightWxWindowV3::UpdateCommand(), pbEnable == 0" );
			NI_ASSERT( pbCheck != 0, "CHeightWxWindowV3::UpdateCommand(), pbCheck == 0" );
			//
			switch ( nCommandID )
			{
				case ID_MITHV3_SET_TIMER:
				case ID_MITHV3_KILL_TIMER:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				case ID_MITHV3_LIST:
					( *pbEnable ) = true;
					( *pbCheck ) = !bThumbnails;
					return true;
				case ID_MITHV3_THUMBNAILS:
					( *pbEnable ) = true;
					( *pbCheck ) = bThumbnails;
					return true;
				case ID_MITHV3_PROPERTIES:
					( *pbEnable ) = ( SelectedTile() >= 0 );
					( *pbCheck ) = false;
					return true;
				default:
					return CHeightCommandsV3::UpdateCommand( nCommandID, pbEnable, pbCheck );
			}
			return false;
		}

		void OnHeightTimer()
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3,
																														ID_MITHV3_ON_TIMER, dwHeightData );
		}

	protected:
		virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
		{
			if ( message == WM_DESTROY )
			{
				// Before the wx side comes down. The timer's only job is to poke
				// the state on the palette's behalf, and a palette that is going
				// away has nothing to say.
				heightTimer.Stop();
			}
			return CWxHostWindow::WindowProc( message, wParam, lParam );
		}

	private:
		// ------------------------------------------------------------------
		// icons
		// ------------------------------------------------------------------

		// IDB_TMITH_BITMAP, cut into its seventeen frames.
		//
		// ::LoadImage rather than anything of wx's, because wxBitmap's
		// wxBITMAP_TYPE_BMP_RESOURCE takes a resource *name* and looks for it in
		// wxGetInstance(), which is the executable. This bitmap has a numeric id
		// and lives in ED_B2_M1.dll.
		//
		// Every slot is filled with a blank first, so that a palette whose
		// bitmap failed to load is a palette with blank buttons rather than one
		// that trips a wx assertion on a null bitmap.
		void LoadIcons()
		{
			for ( int nFrame = 0; nFrame < TMITH_COUNT; ++nFrame )
			{
				icons[nFrame] = wxBitmap( NHeightViewV3::ICON_PIXELS, NHeightViewV3::ICON_PIXELS );
			}
			HBITMAP hStrip = static_cast<HBITMAP>( ::LoadImage( theEDB2M1Instance,
																													MAKEINTRESOURCE( IDB_TMITH_BITMAP ),
																													IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ) );
			if ( hStrip == 0 )
			{
				return;
			}
			BITMAP header = { 0 };
			if ( ::GetObject( hStrip, sizeof( header ), &header ) == 0 )
			{
				::DeleteObject( hStrip );
				return;
			}
			wxBitmap strip;
			// Takes the handle: the wxBitmap deletes it, so hStrip must not be.
			strip.InitFromHBITMAP( reinterpret_cast<WXHBITMAP>( hStrip ),
														 header.bmWidth, header.bmHeight, header.bmBitsPixel );
			wxImage image = strip.ConvertToImage();
			if ( !image.IsOk() ||
					 image.GetWidth() < TMITH_COUNT * NHeightViewV3::ICON_PIXELS ||
					 image.GetHeight() < NHeightViewV3::ICON_PIXELS )
			{
				return;
			}
			// The key colour the MFC palette hands to CImageList::Add. Set on
			// the whole strip once, and GetSubImage carries it into each frame.
			image.SetMaskColour( 255, 0, 255 );
			for ( int nFrame = 0; nFrame < TMITH_COUNT; ++nFrame )
			{
				const wxRect frame( nFrame * NHeightViewV3::ICON_PIXELS, 0,
														NHeightViewV3::ICON_PIXELS, NHeightViewV3::ICON_PIXELS );
				icons[nFrame] = wxBitmap( image.GetSubImage( frame ) );
			}
		}

		// wxBU_EXACTFIT, so the button is its icon plus wx's margins: 40x40 for
		// a 32 pixel icon.
		//
		// The MFC palette's are larger -- 48x44 on a 2x display -- because a
		// dialog template is measured in dialog units and scales with the
		// system DPI while the icon inside it does not. That is a 32 pixel
		// picture in a button with room for a 48 pixel one. Sizing to the icon
		// is the same picture with less space around it, so nothing is lost;
		// what would be worth doing one day is a larger icon strip, and that is
		// a change to the resources rather than to either palette.
		wxBitmapToggleButton* AddIconButton( wxWindow *pParent, wxSizer *pRow,
																				 const wxBitmap &rIcon, const char *pszTip )
		{
			wxBitmapToggleButton *pButton =
					NWx::Child<wxBitmapToggleButton>( pParent, wxID_ANY, rIcon, wxDefaultPosition,
																						wxDefaultSize, wxBU_EXACTFIT );
			pButton->SetToolTip( pszTip );
			pRow->Add( pButton );
			return pButton;
		}

		// The five size buttons wear circular or rectangular brushes depending
		// on the brush type, which is what UpdateSizeButtons has always done.
		void UpdateSizeButtons( SEditParams::EBrushType eBrushType )
		{
			const int nFirst = ( eBrushType == SEditParams::BT_CIRCLE ) ? TMITH_BRUSH_SIZE_C0
																																	: TMITH_BRUSH_SIZE_R0;
			for ( int nSize = 0; nSize < SIZE_COUNT; ++nSize )
			{
				pSize[nSize]->SetBitmap( icons[nFirst + nSize] );
			}
		}

		// ------------------------------------------------------------------
		// the three button groups
		// ------------------------------------------------------------------

		// What BS_AUTORADIOBUTTON does for free and wxBitmapToggleButton does
		// not: exactly one of the group is down. Called both when filling the
		// controls and when one of them is clicked -- including a click on the
		// one already down, which wx would otherwise let the user turn off.
		static void SelectOnly( wxBitmapToggleButton *const *ppButtons, int nCount, int nChecked )
		{
			for ( int nButton = 0; nButton < nCount; ++nButton )
			{
				ppButtons[nButton]->SetValue( nButton == nChecked );
			}
		}

		static void SelectClicked( wxBitmapToggleButton *const *ppButtons, int nCount,
															 const wxObject *pClicked )
		{
			for ( int nButton = 0; nButton < nCount; ++nButton )
			{
				ppButtons[nButton]->SetValue( ppButtons[nButton] == pClicked );
			}
		}

		// GetCheckedRadioButton()'s answer without the arithmetic on control
		// ids. With none of the group down that arithmetic produced a large
		// negative cast to the enum; these answer the enum's zero, which is the
		// state's own default.
		static int CheckedIn( wxBitmapToggleButton *const *ppButtons, int nCount )
		{
			for ( int nButton = 0; nButton < nCount; ++nButton )
			{
				if ( ppButtons[nButton]->GetValue() )
				{
					return nButton;
				}
			}
			return 0;
		}

		SEditParams::EBrush CheckedBrush() const
		{
			return static_cast<SEditParams::EBrush>( CheckedIn( pBrush, BRUSH_COUNT ) );
		}

		SEditParams::EBrushSize CheckedBrushSize() const
		{
			return static_cast<SEditParams::EBrushSize>( CheckedIn( pSize, SIZE_COUNT ) );
		}

		SEditParams::EBrushType CheckedBrushType() const
		{
			return static_cast<SEditParams::EBrushType>( CheckedIn( pType, TYPE_COUNT ) );
		}

		// ------------------------------------------------------------------
		// the tile list
		// ------------------------------------------------------------------

		// The tile icons, borrowed from the object collector rather than built
		// again.
		//
		// The collector is the MFC front-end's -- it lives in MapEditor and
		// keeps its cache as CImageLists -- and wxImageList cannot be handed an
		// existing HIMAGELIST. Copying instead would mean decoding a 64x64 and a
		// 16x16 icon for every terrain tile a second time, into a second cache,
		// for a list that shows the same pictures.
		//
		// So the handle is passed straight to the list control. ListView_SetImageList
		// is exactly the call wxListCtrl::SetImageList makes, and wx creates its
		// list controls with LVS_SHAREIMAGELISTS, so nothing here takes ownership
		// of anything. This goes away when the collector itself is ported.
		void AttachTileIcons()
		{
			const HWND hList = static_cast<HWND>( pTiles->GetHandle() );
			if ( hList == 0 )
			{
				return;
			}
			if ( CImageList *pNormal = ToCImageList( Singleton<IObjectCollector>()->GetImageList( LVSIL_NORMAL ) ) )
			{
				ListView_SetImageList( hList, pNormal->GetSafeHandle(), LVSIL_NORMAL );
			}
			if ( CImageList *pSmall = ToCImageList( Singleton<IObjectCollector>()->GetImageList( LVSIL_SMALL ) ) )
			{
				ListView_SetImageList( hList, pSmall->GetSafeHandle(), LVSIL_SMALL );
			}
		}

		void SetThumbnails( bool _bThumbnails )
		{
			bThumbnails = _bThumbnails;
			ApplyListStyle();
		}

		// SetTileListStyle plus UpdateTileListStyle, which were only ever
		// called one after the other.
		void ApplyListStyle()
		{
			if ( pTiles == 0 )
			{
				return;
			}
			pTiles->SetWindowStyleFlag( wxLC_SINGLE_SEL | wxBORDER_SUNKEN |
																	( bThumbnails ? wxLC_ICON : wxLC_LIST ) );
			if ( bThumbnails )
			{
				// wxListCtrl exposes no icon spacing, so this is the raw call.
				// The numbers are the collector's normal icon size plus the gaps
				// the palettes have always used.
				const HWND hList = static_cast<HWND>( pTiles->GetHandle() );
				if ( hList != 0 )
				{
					ListView_SetIconSpacing( hList, NORMAL_IMAGE_SIZE_X + NORMAL_IMAGE_SPACE_X,
																	 NORMAL_IMAGE_SIZE_Y + NORMAL_IMAGE_SPACE_Y );
				}
			}
			pTiles->Arrange();
		}

		int SelectedTile() const
		{
			const long nItem = pTiles->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			return ( nItem < 0 ) ? -1 : static_cast<int>( pTiles->GetItemData( nItem ) );
		}

		// GetListEditParameters, for a wxListCtrl.
		//
		// Note what the list it reports is made of: the control holds the
		// collector's *labels*, and this reads them back, so a caller asking for
		// MITHV3EP_TILE_COUNT is told the labels and not the object names it
		// passed in. That is what the MFC palette does, through the same helper,
		// and the two have to agree; the names are kept in tileList, which is
		// what ID_MITHV3_PROPERTIES uses. Worth fixing one day, in both at once.
		void ReadTileList( std::vector<std::string> *pList, int *pIndex,
											 bool bCount, bool bIndex ) const
		{
			if ( bCount && ( pList != 0 ) )
			{
				std::vector<std::string> stringList( pTiles->GetItemCount(), std::string() );
				for ( long nItem = 0; nItem < pTiles->GetItemCount(); ++nItem )
				{
					const int nListIndex = static_cast<int>( pTiles->GetItemData( nItem ) );
					if ( ( nListIndex >= 0 ) && ( nListIndex < static_cast<int>( stringList.size() ) ) )
					{
						stringList[nListIndex] = std::string( pTiles->GetItemText( nItem ).utf8_str() );
					}
				}
				( *pList ) = stringList;
			}
			if ( bIndex && ( pIndex != 0 ) )
			{
				( *pIndex ) = SelectedTile();
			}
		}

		// SetListEditParameters, for a wxListCtrl. The names arrive from the
		// state; what goes into the control is the collector's label and icon
		// for each one, and the list index travels as the item's data because
		// a name the collector does not know is skipped and positions shift.
		void WriteTileList( const std::vector<std::string> &rList, int nIndex,
												bool bCount, bool bIndex )
		{
			if ( !bCount && !bIndex )
			{
				return;
			}
			wxBusyCursor waitCursor;
			int nSelectedIndex = SelectedTile();
			if ( nSelectedIndex < 0 )
			{
				nSelectedIndex = 0;
			}
			if ( bCount )
			{
				pTiles->DeleteAllItems();
				IObjectCollector::CObjectCollection objectCollection;
				Singleton<IObjectCollector>()->ApplyFilter( &objectCollection, NHeightViewV3::TILE_TYPE_NAME );
				IObjectCollector::CObjectCollection::const_iterator posCollection =
						objectCollection.find( NHeightViewV3::TILE_TYPE_NAME );
				if ( posCollection != objectCollection.end() )
				{
					long nItem = 0;
					for ( size_t nListIndex = 0; nListIndex < rList.size(); ++nListIndex )
					{
						IObjectCollector::CObjectNameCollection::const_iterator posName =
								posCollection->second.find( rList[nListIndex] );
						if ( posName != posCollection->second.end() )
						{
							const long nInserted = pTiles->InsertItem( nItem,
																												wxString::FromUTF8( posName->second.szLabel.c_str() ),
																												posName->second.nIconIndex );
							pTiles->SetItemData( nInserted, static_cast<long>( nListIndex ) );
							++nItem;
						}
					}
				}
				tileList = rList;
			}
			if ( bIndex )
			{
				nSelectedIndex = nIndex;
			}
			const long nStateMask = wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED;
			for ( long nItem = 0; nItem < pTiles->GetItemCount(); ++nItem )
			{
				if ( static_cast<int>( pTiles->GetItemData( nItem ) ) == nSelectedIndex )
				{
					pTiles->SetItemState( nItem, nStateMask, nStateMask );
					pTiles->EnsureVisible( nItem );
				}
				else
				{
					pTiles->SetItemState( nItem, 0, nStateMask );
				}
			}
		}

		bool ShowSelectedTileProperties()
		{
			const int nListIndex = SelectedTile();
			if ( ( nListIndex < 0 ) || ( nListIndex >= static_cast<int>( tileList.size() ) ) )
			{
				return false;
			}
			NHeightViewV3::ShowTileProperties( tileList[nListIndex] );
			return true;
		}

		// ------------------------------------------------------------------
		// events
		// ------------------------------------------------------------------

		// One command, to one state, with the flag naming what changed. The
		// state reads that field back out of the palette and decides what it
		// means; the palette does not know.
		void Report( unsigned nFlag )
		{
			if ( bCreateControls )
			{
				return;
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3,
																														ID_GET_EDIT_PARAMETERS, nFlag );
		}

		void OnBrush( wxCommandEvent &rEvent )
		{
			SelectClicked( pBrush, BRUSH_COUNT, rEvent.GetEventObject() );
			if ( bCreateControls )
			{
				return;
			}
			// What OnBrushRadio does: leaving the tile brush drops the tile
			// selection, and coming back to it restores the last one. The list
			// itself is left alone -- only the selection moves -- which is what
			// passing an empty list with bCount false means.
			bCreateControls = true;
			WriteTileList( std::vector<std::string>(),
										 ( CheckedBrush() == SEditParams::B_TILE ) ? nLastIndex : -1,
										 false, true );
			bCreateControls = false;
			Report( MITHV3EP_BRUSH );
		}

		void OnBrushSize( wxCommandEvent &rEvent )
		{
			SelectClicked( pSize, SIZE_COUNT, rEvent.GetEventObject() );
			Report( MITHV3EP_BRUSH_SIZE );
		}

		void OnBrushType( wxCommandEvent &rEvent )
		{
			SelectClicked( pType, TYPE_COUNT, rEvent.GetEventObject() );
			Report( MITHV3EP_BRUSH_TYPE );
			// After the report, as in the MFC palette: the state has already
			// been told, and this only changes what the size buttons look like.
			UpdateSizeButtons( CheckedBrushType() );
		}

		void OnFixCliffs( wxCommandEvent& )
		{
			Report( MITHV3EP_UPDATE_HEIGHT );
		}

		void OnTileSelected( wxListEvent& )
		{
			if ( bCreateControls )
			{
				return;
			}
			// wxEVT_LIST_ITEM_SELECTED is raised only by a selection arriving,
			// which is what the MFC handler had to work out from LVN_ITEMCHANGED
			// by hand: focus changes and deselections come through that too.
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3,
																														ID_GET_EDIT_PARAMETERS, MITHV3EP_TILE_INDEX );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}

		void OnTileListSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			pTiles->Arrange();
		}

		void OnTileContextMenu( wxContextMenuEvent &rEvent )
		{
			// The same three items as IDM_MAPINFO_CONTEXT_MENU's
			// MI_TERRAIN_HEIGHT_V3_TILE_LIST popup. What is checked and enabled
			// comes from the same state UpdateCommand reads, so the menu says
			// the same thing whichever route opened it.
			wxMenu menu;
			menu.AppendCheckItem( MENU_LIST, "&List" );
			menu.AppendCheckItem( MENU_THUMBNAILS, "&Thumbnails" );
			menu.AppendSeparator();
			menu.Append( MENU_PROPERTIES, "P&roperties" );
			menu.Check( MENU_LIST, !bThumbnails );
			menu.Check( MENU_THUMBNAILS, bThumbnails );
			menu.Enable( MENU_PROPERTIES, SelectedTile() >= 0 );
			menu.Bind( wxEVT_MENU, &CHeightWxWindowV3::OnMenu, this );

			// wxDefaultPosition is the menu key rather than the mouse; wx's own
			// documented answer to that is to pop up at the origin of the window.
			wxPoint point = rEvent.GetPosition();
			point = ( point == wxDefaultPosition ) ? wxPoint( 0, 0 ) : pTiles->ScreenToClient( point );
			pTiles->PopupMenu( &menu, point );

			// The scene captured the mouse before the menu did; without this it
			// keeps a drag it never saw the end of. The MFC palette makes the
			// same call after TrackPopupMenu.
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}

		void OnMenu( wxCommandEvent &rEvent )
		{
			switch ( rEvent.GetId() )
			{
				case MENU_LIST:
					SetThumbnails( false );
					break;
				case MENU_THUMBNAILS:
					SetThumbnails( true );
					break;
				case MENU_PROPERTIES:
					ShowSelectedTileProperties();
					break;
			}
		}
	};
}


namespace NHeightViewV3
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		// AddNewTab with a pointer rather than a null one: the template only
		// allocates when handed nothing, and this needs building before it is
		// registered. Either way the tab list owns it from here.
		CHeightWxWindowV3 *pWindow = pTabWindow->AddNewTab( new CHeightWxWindowV3() );
		if ( pWindow == 0 )
		{
			return 0;
		}
		if ( !pWindow->Build( pTabWindow ) )
		{
			// Left in the tab list deliberately: it is the list's to delete, and
			// removing it here would be the only place that ever did.
			return 0;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
