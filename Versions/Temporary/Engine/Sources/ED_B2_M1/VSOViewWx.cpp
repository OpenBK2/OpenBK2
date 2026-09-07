#include "stdafx.h"

#include "VSOView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include <fmt/format.h>

#include "ObjectProperties.h"
#include "ResourceDefines.h"
#include "VSOMultiState.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/DialogState.h"
#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_ObjectCollector.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "libdb/ResourceManager.h"

#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/radiobut.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

#include <commctrl.h>

#include <cstdio>
#include <string>
#include <unordered_map>

// The VSO palette, in wx: the ninth, and very nearly the map object palette
// again -- a filter combo, an object list the collector fills, the same
// thumbnails and properties menu, and the same job of being the editor's object
// storage while it has a selection. What is different is above the filter, and
// most of what is interesting about this one is what does *not* happen.
//
// **Four of its handlers were never connected, and are not connected here.**
// CVSOWindow declares OnPointNumberRadio, OnStatsTypeRadio, OnChangeWidth and
// OnChangeOpacity, and its message map has entries for the size, the filter
// combo, the list notification and the context menu -- and for none of those
// four. So the two radio groups and the two edit boxes never tell the state
// anything: the state reads them when it needs them, through
// GetEditParameters, and a change made in the palette reaches the map the next
// time something asks. Binding them in wx would be a behaviour change dressed
// up as a port, so they are not bound, and this comment is here because a
// missing Bind() looks like an oversight and this one is not.
//
// **The height mode is dormant.** bEnableHeight is set false in the constructor
// and never set again: EnableHeight(), which ID_MIVSO_ENABLE_HEIGHT calls, has
// an empty body. So the opacity box is always an opacity box, the
// MIVSOSEP_HEIGHT branch always takes its else, and that else assigns
// DEFAULT_OPACITY to a height -- which looks like a typo for DEFAULT_HEIGHT and
// is reproduced exactly, because a value the state has been receiving since
// 2005 is not something to quietly change while moving a palette to another
// toolkit.
//
// The opacity conversion is the other thing to read twice: the box is a
// percentage and the state's field is a fraction, so the getter divides by 100
// and the setter multiplies -- except along the clamp path, where the original
// writes 100 into the box and 1.0f into the field for the too-large case and
// leaves the field alone for the unparseable-but-in-range case. All three
// branches are kept.

namespace
{
	typedef CVSOMultiState::SEditParameters SEditParams;

	const int POINT_NUMBER_COUNT = 3;
	const int STATS_TYPE_COUNT = 2;

	// The upper bound the width and height boxes are clamped to, spelled as the
	// palette spells it.
	const float MAX_EXTENT = AI_TILE_SIZE * 2.0f * 16.0f;


	class CVSOWxWindow : public CWxHostWindow, public CVSOCommands
	{
		struct SObjectListElement
		{
			std::string szObjectTypeName;
			CDBID objectDBID;
		};
		typedef std::unordered_map<int, SObjectListElement> CObjectListElementMap;

		enum EMenuItem
		{
			MENU_LIST = wxID_HIGHEST + 1,
			MENU_THUMBNAILS,
			MENU_PROPERTIES,
		};

		wxRadioButton *pPointNumber[POINT_NUMBER_COUNT] = { nullptr, nullptr, nullptr };
		wxRadioButton *pStatsType[STATS_TYPE_COUNT] = { nullptr, nullptr };
		wxTextCtrl *pWidth = nullptr;
		wxTextCtrl *pOpacity = nullptr;
		wxChoice *pFilters = nullptr;
		wxListCtrl *pObjects = nullptr;

		bool bCreateControls = true;
		// Always false; see the note at the top.
		bool bEnableHeight = false;
		// LVS_ICON as against LVS_LIST, which is all nStyle ever held there.
		bool bThumbnails = true;

		SObjectListElement selectedObjectListElement;
		CObjectListElementMap objectListElementMap;

		SDialogState dialogState;

	public:
		CVSOWxWindow()
		{
			selectedObjectListElement.szObjectTypeName.clear();
			selectedObjectListElement.objectDBID.Clear();
			Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_VSO_WINDOW, this );
			Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_VSO_WINDOW, ID_MIVSOOLCM_LIST, ID_MIVSOOLCM_PROPERTIES );
		}

		virtual ~CVSOWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_VSO_WINDOW );
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_VSO_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pHostRoot = Root();

			wxScrolledWindow *const pRoot = NWx::Child<wxScrolledWindow>( pHostRoot, wxID_ANY );
			pRoot->SetScrollRate( 0, 8 );
			wxBoxSizer *pHostSizer = new wxBoxSizer( wxVERTICAL );
			pHostSizer->Add( pRoot, wxSizerFlags( 1 ).Expand() );
			pHostRoot->SetSizer( pHostSizer );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// Two radio groups side by side, three on the left and two on the
			// right. **Creation order is what puts them in the right groups**:
			// wx starts a new group at each wxRB_GROUP among consecutive
			// siblings, and siblings are ordered by when they were made, not by
			// where the sizers put them. So all three of the left column are
			// made before the first of the right.
			static const char *const POINT_NUMBERS[POINT_NUMBER_COUNT] =
			{
				"Single Point", "Multi Points", "All Points"
			};
			static const char *const STATS_TYPES[STATS_TYPE_COUNT] =
			{
				"Use Predefined Stats", "Use Custom Stats"
			};

			wxBoxSizer *pRadioRow = new wxBoxSizer( wxHORIZONTAL );
			wxBoxSizer *pPointColumn = new wxBoxSizer( wxVERTICAL );
			for ( int nPoint = 0; nPoint < POINT_NUMBER_COUNT; ++nPoint )
			{
				pPointNumber[nPoint] = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, POINT_NUMBERS[nPoint],
																													wxDefaultPosition, wxDefaultSize,
																													( nPoint == 0 ) ? wxRB_GROUP : 0 );
				pPointColumn->Add( pPointNumber[nPoint], wxSizerFlags().Border( wxTOP, nPoint ? 2 : 0 ) );
			}
			wxBoxSizer *pStatsColumn = new wxBoxSizer( wxVERTICAL );
			for ( int nStats = 0; nStats < STATS_TYPE_COUNT; ++nStats )
			{
				pStatsType[nStats] = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, STATS_TYPES[nStats],
																												wxDefaultPosition, wxDefaultSize,
																												( nStats == 0 ) ? wxRB_GROUP : 0 );
				pStatsColumn->Add( pStatsType[nStats], wxSizerFlags().Border( wxTOP, nStats ? 2 : 0 ) );
			}
			pRadioRow->Add( pPointColumn );
			pRadioRow->AddStretchSpacer();
			pRadioRow->Add( pStatsColumn );
			pSizer->Add( pRadioRow, wxSizerFlags().Expand() );

			// IDC_TMIVSO_DELIMITER_0 and _1 are SS_ETCHEDHORZ statics.
			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			// Width: [    ] pts        Opacity: [    ] %
			wxBoxSizer *pValueRow = new wxBoxSizer( wxHORIZONTAL );
			pValueRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Width:" ),
											wxSizerFlags().Centre() );
			pWidth = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			pValueRow->Add( pWidth, wxSizerFlags( 1 ).Centre().Border( wxLEFT, 4 ) );
			pValueRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "pts" ),
											wxSizerFlags().Centre().Border( wxLEFT, 4 ) );
			pValueRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Opacity:" ),
											wxSizerFlags().Centre().Border( wxLEFT, 8 ) );
			pOpacity = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			pValueRow->Add( pOpacity, wxSizerFlags( 1 ).Centre().Border( wxLEFT, 4 ) );
			pValueRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "%" ),
											wxSizerFlags().Centre().Border( wxLEFT, 4 ) );
			pSizer->Add( pValueRow, wxSizerFlags().Expand() );

			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			// Filter: [                    ]
			wxBoxSizer *pFilterRow = new wxBoxSizer( wxHORIZONTAL );
			pFilterRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Filter:" ),
											 wxSizerFlags().Centre() );
			// No sort: the filter list has an order of its own and the combo
			// shows it. The filter index still travels as client data, because a
			// separator entry can sit between two filters.
			pFilters = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			pFilterRow->Add( pFilters, wxSizerFlags( 1 ).Centre().Border( wxLEFT, 4 ) );
			pSizer->Add( pFilterRow, wxSizerFlags().Expand() );

			// LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_SORTASCENDING |
			// LVS_SHAREIMAGELISTS from the template. wx adds the last two of
			// those to every list control it makes; the sort is asked for here.
			pObjects = NWx::Child<wxListCtrl>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																				 wxLC_ICON | wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING |
																				 wxBORDER_SUNKEN );
			pObjects->SetMinSize( wxSize( -1, 160 ) );
			pSizer->Add( pObjects, wxSizerFlags( 1 ).Expand().Border( wxTOP, 4 ) );
			AttachObjectIcons();

			pRoot->SetSizer( pSizer );
			pRoot->FitInside();

			// Only the four the message map connects. See the note at the top
			// for the four it does not.
			pFilters->Bind( wxEVT_CHOICE, &CVSOWxWindow::OnFilterChanged, this );
			pObjects->Bind( wxEVT_LIST_ITEM_SELECTED, &CVSOWxWindow::OnObjectSelected, this );
			pObjects->Bind( wxEVT_CONTEXT_MENU, &CVSOWxWindow::OnObjectContextMenu, this );
			pObjects->Bind( wxEVT_SIZE, &CVSOWxWindow::OnObjectListSize, this );

			ApplyListStyle();
			NDialogState::Load( "CVSOWindow", &dialogState );
			FillFilterList();
			FillObjectList();
			bCreateControls = false;
			return true;
		}

		//	CVSOCommands
		virtual bool GetEditParameters( SEditParams *pEditParameters )
		{
			if ( pEditParameters == 0 )
			{
				return false;
			}
			if ( pEditParameters->nFlags & MIVSOSEP_POINT_NUMBER )
			{
				pEditParameters->ePointNumber =
						static_cast<SEditParams::EPointNumber>( CheckedIn( pPointNumber, POINT_NUMBER_COUNT ) );
			}
			if ( pEditParameters->nFlags & MIVSOSEP_STATS_TYPE )
			{
				pEditParameters->eStatsType =
						static_cast<SEditParams::EStatsType>( CheckedIn( pStatsType, STATS_TYPE_COUNT ) );
			}
			if ( ( pEditParameters->nFlags & MIVSOSEP_WIDTH ) > 0 )
			{
				const std::string szText( pWidth->GetValue().utf8_str() );
				if ( ( sscanf( szText.c_str(), "%g", &( pEditParameters->fWidth ) ) < 1 ) ||
						 ( pEditParameters->fWidth < 0.0f ) ||
						 ( pEditParameters->fWidth > MAX_EXTENT ) )
				{
					bCreateControls = true;
					// Both ends go to the default rather than to the bound, which
					// is what the original does.
					if ( pEditParameters->fWidth < 0.0f )
					{
						pEditParameters->fWidth = CVSOManager::DEFAULT_WIDTH;
					}
					else if ( pEditParameters->fWidth > MAX_EXTENT )
					{
						pEditParameters->fWidth = CVSOManager::DEFAULT_WIDTH;
					}
					pWidth->SetValue( fmt::format( "{:g}", pEditParameters->fWidth ).c_str() );
					bCreateControls = false;
				}
			}
			if ( ( pEditParameters->nFlags & MIVSOSEP_OPACITY ) > 0 )
			{
				if ( !bEnableHeight )
				{
					const std::string szText( pOpacity->GetValue().utf8_str() );
					if ( ( sscanf( szText.c_str(), "%g", &( pEditParameters->fOpacity ) ) < 1 ) ||
							 ( pEditParameters->fOpacity < 0.0f ) ||
							 ( pEditParameters->fOpacity > 100.0f ) )
					{
						bCreateControls = true;
						if ( pEditParameters->fOpacity < 0.0f )
						{
							pEditParameters->fOpacity = 0.0f;
							pOpacity->SetValue( fmt::format( "{:g}", pEditParameters->fOpacity ).c_str() );
						}
						else if ( pEditParameters->fOpacity > 100.0f )
						{
							// The box shows 100 and the field becomes 1.0 -- the
							// fraction the state wants -- rather than the box's
							// value divided by a hundred. Written out in this order
							// in the original too.
							pEditParameters->fOpacity = 100.0f;
							pOpacity->SetValue( fmt::format( "{:g}", pEditParameters->fOpacity ).c_str() );
							pEditParameters->fOpacity = 1.0f;
						}
						else
						{
							// Unparseable but in range: the box is rewritten from
							// whatever the caller passed in, and the field is left
							// as a fraction already.
							pOpacity->SetValue( fmt::format( "{:g}", pEditParameters->fOpacity ).c_str() );
						}
						bCreateControls = false;
					}
					else
					{
						pEditParameters->fOpacity = pEditParameters->fOpacity / 100.f;
					}
				}
				else
				{
					pEditParameters->fOpacity = CVSOManager::DEFAULT_OPACITY;
				}
			}
			if ( ( pEditParameters->nFlags & MIVSOSEP_HEIGHT ) > 0 )
			{
				if ( bEnableHeight )
				{
					// Shares the opacity box, which is what the height mode is:
					// the same control reading a different quantity. Unreachable
					// while EnableHeight does nothing.
					const std::string szText( pOpacity->GetValue().utf8_str() );
					if ( ( sscanf( szText.c_str(), "%g", &( pEditParameters->fHeight ) ) < 1 ) ||
							 ( pEditParameters->fHeight < 0.0f ) ||
							 ( pEditParameters->fHeight > MAX_EXTENT ) )
					{
						bCreateControls = true;
						if ( pEditParameters->fHeight < 0.0f )
						{
							pEditParameters->fHeight = 0.0f;
						}
						else if ( pEditParameters->fHeight > MAX_EXTENT )
						{
							pEditParameters->fHeight = MAX_EXTENT;
						}
						pOpacity->SetValue( fmt::format( "{:g}", pEditParameters->fHeight ).c_str() );
						bCreateControls = false;
					}
				}
				else
				{
					// DEFAULT_OPACITY into a height. Looks like a typo for
					// DEFAULT_HEIGHT and is what the state has been given since
					// 2005; not something to change on the way past.
					pEditParameters->fHeight = CVSOManager::DEFAULT_OPACITY;
				}
			}
			if ( ( pEditParameters->nFlags & MIVSOSEP_THUMBNAILS ) > 0 )
			{
				pEditParameters->bThumbnails = bThumbnails;
			}
			return true;
		}

		virtual bool SetEditParameters( const SEditParams &rEditParameters )
		{
			// No bCreateControls here, as in the original -- and it costs
			// nothing, because none of the four controls it writes has a change
			// handler bound.
			if ( rEditParameters.nFlags & MIVSOSEP_POINT_NUMBER )
			{
				SelectOnly( pPointNumber, POINT_NUMBER_COUNT, rEditParameters.ePointNumber );
			}
			if ( rEditParameters.nFlags & MIVSOSEP_STATS_TYPE )
			{
				SelectOnly( pStatsType, STATS_TYPE_COUNT, rEditParameters.eStatsType );
			}
			if ( ( rEditParameters.nFlags & MIVSOSEP_WIDTH ) > 0 )
			{
				pWidth->SetValue( fmt::format( "{:.2f}", rEditParameters.fWidth ).c_str() );
			}
			if ( ( rEditParameters.nFlags & MIVSOSEP_OPACITY ) > 0 )
			{
				if ( !bEnableHeight )
				{
					pOpacity->SetValue( fmt::format( "{:.2f}", rEditParameters.fOpacity * 100.0f ).c_str() );
				}
			}
			if ( ( rEditParameters.nFlags & MIVSOSEP_HEIGHT ) > 0 )
			{
				if ( bEnableHeight )
				{
					pOpacity->SetValue( fmt::format( "{:.2f}", rEditParameters.fHeight ).c_str() );
				}
			}
			if ( ( rEditParameters.nFlags & MIVSOSEP_THUMBNAILS ) > 0 )
			{
				SetThumbnails( rEditParameters.bThumbnails );
			}
			return true;
		}

		//	ICommandHandler
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			switch ( nCommandID )
			{
				case ID_MIVSO_CLEAR_SELECTION:
					ClearSelection();
					return true;
				case ID_MIVSO_ENABLE_HEIGHT:
					// Empty in the original, and empty here. Every branch behind
					// bEnableHeight is dormant because of it.
					return true;
				case ID_OS_GET_OBJECTSET:
					if ( SObjectSet *pObjectSet = reinterpret_cast<SObjectSet*>( dwData ) )
					{
						pObjectSet->szObjectTypeName = selectedObjectListElement.szObjectTypeName;
						pObjectSet->objectNameSet.clear();
						pObjectSet->objectNameSet[selectedObjectListElement.objectDBID] = 0;
					}
					return true;
				case ID_MIVSOOLCM_LIST:
					SetThumbnails( false );
					return true;
				case ID_MIVSOOLCM_THUMBNAILS:
					SetThumbnails( true );
					return true;
				case ID_MIVSOOLCM_PROPERTIES:
					ShowSelectedObjectProperties();
					return true;
				default:
					return CVSOCommands::HandleCommand( nCommandID, dwData );
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			NI_ASSERT( pbEnable != 0, "CVSOWxWindow::UpdateCommand(), pbEnable == 0" );
			NI_ASSERT( pbCheck != 0, "CVSOWxWindow::UpdateCommand(), pbCheck == 0" );
			//
			switch ( nCommandID )
			{
				case ID_MIVSO_CLEAR_SELECTION:
				case ID_MIVSO_ENABLE_HEIGHT:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				case ID_OS_GET_OBJECTSET:
				case ID_MIVSOOLCM_PROPERTIES:
					( *pbEnable ) = HasSelectedObject();
					( *pbCheck ) = false;
					return true;
				case ID_MIVSOOLCM_LIST:
					( *pbEnable ) = true;
					( *pbCheck ) = !bThumbnails;
					return true;
				case ID_MIVSOOLCM_THUMBNAILS:
					( *pbEnable ) = true;
					( *pbCheck ) = bThumbnails;
					return true;
				default:
					return CVSOCommands::UpdateCommand( nCommandID, pbEnable, pbCheck );
			}
			return false;
		}

	private:
		// ------------------------------------------------------------------
		// the radio groups
		// ------------------------------------------------------------------

		static void SelectOnly( wxRadioButton *const *ppButtons, int nCount, int nChecked )
		{
			if ( ( nChecked >= 0 ) && ( nChecked < nCount ) )
			{
				ppButtons[nChecked]->SetValue( true );
			}
		}

		// GetCheckedRadioButton()'s answer without the arithmetic on control
		// ids. With none of the group down that arithmetic produced a large
		// negative cast to the enum; this answers the enum's zero. wx always has
		// one down, so the case does not arise here anyway.
		static int CheckedIn( wxRadioButton *const *ppButtons, int nCount )
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

		// ------------------------------------------------------------------
		// the filter combo
		// ------------------------------------------------------------------

		void FillFilterList()
		{
			const bool bWasCreating = bCreateControls;
			bCreateControls = true;
			pFilters->Clear();
			IObjectFilterCollector::CFilterList filterList;
			Singleton<IObjectFilterCollector>()->GetFilterList( &filterList, NVSOView::FILTER_TYPE );
			for ( size_t nFilterIndex = 0; nFilterIndex < filterList.size(); ++nFilterIndex )
			{
				pFilters->Append( wxString::FromUTF8( filterList[nFilterIndex].c_str() ),
													reinterpret_cast<void*>( static_cast<uintptr_t>( nFilterIndex ) ) );
			}
			if ( !filterList.empty() )
			{
				// The remembered one by name, or entry 5 if there are more than
				// six. That number is the original's and is not derived from
				// anything: the first entries of the VSO filter list are the
				// separators and the all-of-a-kind filters.
				//
				// FindString rather than SelectString: MFC's is a case-insensitive
				// *prefix* match, and this is looking up a whole name that came
				// out of this same list, so an exact match is what was meant.
				const int nRemembered = dialogState.GetStringParameter( 0 ).empty()
						? wxNOT_FOUND
						: pFilters->FindString( wxString::FromUTF8( dialogState.GetStringParameter( 0 ).c_str() ), false );
				if ( nRemembered != wxNOT_FOUND )
				{
					pFilters->SetSelection( nRemembered );
				}
				else if ( filterList.size() > 6 )
				{
					pFilters->SetSelection( 5 );
				}
			}
			bCreateControls = bWasCreating;
		}

		// GetSelectedFilterIndex, side effect and all: reading which filter is
		// chosen is also what records its name for next session.
		int SelectedFilterIndex()
		{
			const int nPosition = pFilters->GetSelection();
			if ( nPosition == wxNOT_FOUND )
			{
				return INVALID_NODE_ID;
			}
			dialogState.SetStringParameter( 0, std::string( pFilters->GetString( nPosition ).utf8_str() ) );
			return (int)reinterpret_cast<uintptr_t>( pFilters->GetClientData( nPosition ) );
		}

		// ------------------------------------------------------------------
		// the object list
		// ------------------------------------------------------------------

		// Borrowed from the MFC front-end's object collector rather than copied;
		// see the note in HeightViewV3Wx.cpp. wx creates its list controls with
		// LVS_SHAREIMAGELISTS, so nothing here takes ownership.
		void AttachObjectIcons()
		{
			const HWND hList = static_cast<HWND>( pObjects->GetHandle() );
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

		void ApplyListStyle()
		{
			if ( pObjects == 0 )
			{
				return;
			}
			pObjects->SetWindowStyleFlag( wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING | wxBORDER_SUNKEN |
																		( bThumbnails ? wxLC_ICON : wxLC_LIST ) );
			if ( bThumbnails )
			{
				// wxListCtrl exposes no icon spacing, so this is the raw call.
				const HWND hList = static_cast<HWND>( pObjects->GetHandle() );
				if ( hList != 0 )
				{
					ListView_SetIconSpacing( hList, NORMAL_IMAGE_SIZE_X + NORMAL_IMAGE_SPACE_X,
																	 NORMAL_IMAGE_SIZE_Y + NORMAL_IMAGE_SPACE_Y );
				}
			}
			pObjects->Arrange();
		}

		void FillObjectList()
		{
			wxBusyCursor waitCursor;
			const int nFilterIndex = SelectedFilterIndex();
			if ( nFilterIndex == INVALID_NODE_ID )
			{
				return;
			}
			if ( Singleton<IObjectFilterCollector>()->IsSeparator( NVSOView::FILTER_TYPE, nFilterIndex ) )
			{
				return;
			}
			const IObjectFilter *pObjectFilter =
					Singleton<IObjectFilterCollector>()->Get( NVSOView::FILTER_TYPE, nFilterIndex );
			if ( pObjectFilter == 0 )
			{
				return;
			}
			const bool bWasCreating = bCreateControls;
			bCreateControls = true;
			pObjects->DeleteAllItems();
			objectListElementMap.clear();
			IObjectCollector::CObjectCollection objectCollection;
			Singleton<IObjectCollector>()->ApplyFilter( &objectCollection, pObjectFilter );
			int nObjectsCount = 0;
			for ( IObjectCollector::CObjectCollection::const_iterator itObjectCollection = objectCollection.begin(); itObjectCollection != objectCollection.end(); ++itObjectCollection )
			{
				for ( IObjectCollector::CObjectNameCollection::const_iterator itObjectNameCollection = itObjectCollection->second.begin(); itObjectNameCollection != itObjectCollection->second.end(); ++itObjectNameCollection )
				{
					SObjectListElement objectListElement;
					objectListElement.szObjectTypeName = itObjectCollection->first;
					objectListElement.objectDBID = itObjectNameCollection->first;
					//
					const long nItem = pObjects->InsertItem( nObjectsCount,
																									wxString::FromUTF8( itObjectNameCollection->second.szLabel.c_str() ),
																									itObjectNameCollection->second.nIconIndex );
					pObjects->SetItemData( nItem, nObjectsCount );
					objectListElementMap[nObjectsCount] = objectListElement;
					++nObjectsCount;
				}
			}
			bCreateControls = bWasCreating;
		}

		bool HasSelectedObject() const
		{
			return ( !selectedObjectListElement.szObjectTypeName.empty() ) &&
						 ( !selectedObjectListElement.objectDBID.IsEmpty() );
		}

		// What makes this palette the object storage. A new selection is not
		// reported to a state that reads it back: it is pushed, because the
		// object the user is about to place decides which input state the editor
		// is in.
		void UpdateSelection()
		{
			if ( bCreateControls )
			{
				return;
			}
			const long nItem = pObjects->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			if ( nItem < 0 )
			{
				return;
			}
			const int nKey = static_cast<int>( pObjects->GetItemData( nItem ) );
			CObjectListElementMap::const_iterator posObjectListElement = objectListElementMap.find( nKey );
			if ( posObjectListElement == objectListElementMap.end() )
			{
				return;
			}
			if ( ( posObjectListElement->second.szObjectTypeName == selectedObjectListElement.szObjectTypeName ) &&
					 ( posObjectListElement->second.objectDBID == selectedObjectListElement.objectDBID ) )
			{
				return;
			}
			selectedObjectListElement = posObjectListElement->second;
			ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
			pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, this );
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_VSO_MULTI_STATE, ID_MIVSO_SWITCH_MULTI_STATE, reinterpret_cast<uintptr_t>( &( selectedObjectListElement.szObjectTypeName ) ) );
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_VSO_STATE, ID_MIVSO_SWITCH_ADD_STATE, 0 );
		}

		void ClearSelection()
		{
			bCreateControls = true;
			const long nItem = pObjects->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
			if ( nItem >= 0 )
			{
				pObjects->SetItemState( nItem, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
			}
			selectedObjectListElement.szObjectTypeName.clear();
			selectedObjectListElement.objectDBID.Clear();
			Singleton<IMainFrameContainer>()->Get()->RestoreObjectStorage();
			bCreateControls = false;
		}

		void ShowSelectedObjectProperties()
		{
			SObjectSet objectSet;
			objectSet.szObjectTypeName = selectedObjectListElement.szObjectTypeName;
			InsertHashSetElement( &( objectSet.objectNameSet ), selectedObjectListElement.objectDBID );
			NObjectProperties::Show( objectSet );
		}

		// ------------------------------------------------------------------
		// events
		// ------------------------------------------------------------------

		void OnFilterChanged( wxCommandEvent& )
		{
			if ( bCreateControls )
			{
				return;
			}
			FillObjectList();
			// SelectedFilterIndex, called from FillObjectList, has just recorded
			// the new name; this is what puts it on disk.
			NDialogState::Save( "CVSOWindow", &dialogState );
		}

		void OnObjectSelected( wxListEvent& ) { UpdateSelection(); }

		void OnObjectListSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			pObjects->Arrange();
		}

		void OnObjectContextMenu( wxContextMenuEvent &rEvent )
		{
			// IDM_MAPINFO_CONTEXT_MENU's MI_VSO_OBJECT_LIST popup, built in
			// place; what is checked and enabled is read from the same state
			// UpdateCommand reads.
			wxMenu menu;
			menu.AppendCheckItem( MENU_LIST, "&List" );
			menu.AppendCheckItem( MENU_THUMBNAILS, "&Thumbnails" );
			menu.AppendSeparator();
			menu.Append( MENU_PROPERTIES, "P&roperties" );
			menu.Check( MENU_LIST, !bThumbnails );
			menu.Check( MENU_THUMBNAILS, bThumbnails );
			menu.Enable( MENU_PROPERTIES, HasSelectedObject() );
			menu.Bind( wxEVT_MENU, &CVSOWxWindow::OnMenu, this );

			wxPoint point = rEvent.GetPosition();
			point = ( point == wxDefaultPosition ) ? wxPoint( 0, 0 ) : pObjects->ScreenToClient( point );
			pObjects->PopupMenu( &menu, point );

			// The scene captured the mouse before the menu did; the MFC palette
			// makes the same call after TrackPopupMenu.
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
					ShowSelectedObjectProperties();
					break;
			}
		}
	};
}


namespace NVSOView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CVSOWxWindow *pWindow = pTabWindow->AddNewTab( new CVSOWxWindow() );
		if ( pWindow == 0 )
		{
			return 0;
		}
		if ( !pWindow->Build( pTabWindow ) )
		{
			// Left in the tab list deliberately: it is the list's to delete.
			return 0;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
