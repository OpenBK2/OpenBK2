#include "stdafx.h"

#include "MapObjectView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include <fmt/format.h>

#include "MapObjectMultiState.h"
#include "ObjectProperties.h"
#include "ResourceDefines.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/DialogState.h"
#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_ObjectCollector.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
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
#include <vector>

// The map object palette, in wx: the eighth palette, and the first that is
// something other than a view onto its state.
//
// The three before it read their controls when asked and wrote them when told.
// This one also **is** the editor's object storage: when the selection in its
// list changes it registers itself under CHID_OBJECT_STORAGE and answers
// ID_OS_GET_OBJECTSET with whatever is selected, which is how the rest of the
// editor finds out what the user is about to place. That is why the selection
// handling is not just a report -- see UpdateSelection.
//
// Two smaller differences from the palettes before it, both of them the MFC
// palette's and kept rather than tidied:
//
//   * **SetEditParameters does not raise the bCreateControls guard.** The other
//     palettes do, so that filling their controls does not report an edit back.
//     This one writes the direction into a read-only edit box, EN_CHANGE fires,
//     and the palette reports MIMOSEP_DIRECTION straight back at the state. The
//     loop terminates because the state's own getter does not write back, and
//     the value it reads is the one it just sent. wxTextCtrl::SetValue raises
//     wxEVT_TEXT exactly as SetDlgItemText raises EN_CHANGE, so the behaviour
//     carries over unchanged. `ChangeValue` would have been the quiet one.
//
//   * **The filter combo is remembered on disk**, by name, in the same
//     Editor/ResizeDialogStyles/CMapObjectWindow.xml the MFC palette writes --
//     that is what SDialogState and NDialogState are for, and why they are not
//     nested in CResizeDialog any more. A user switching between the two
//     implementations keeps their filter.

namespace
{
	typedef CMapObjectMultiState::SEditParameters SEditParams;


	class CMapObjectWxWindow : public CWxHostWindow, public CMapObjectCommands
	{
		// What a row in the object list stands for. The list itself carries only
		// a running number as item data, because the control is sorted and its
		// positions move.
		struct SObjectListElement
		{
			std::string szObjectTypeName;
			CDBID objectDBID;
		};
		typedef std::unordered_map<unsigned, SObjectListElement> CObjectListElementMap;

		enum EMenuItem
		{
			MENU_LIST = wxID_HIGHEST + 1,
			MENU_THUMBNAILS,
			MENU_PROPERTIES,
		};

		wxChoice *pPlayers = nullptr;
		wxRadioButton *pDirectionRandom = nullptr;
		wxRadioButton *pDirectionFixed = nullptr;
		wxTextCtrl *pDirection = nullptr;
		wxChoice *pFilters = nullptr;
		wxListCtrl *pObjects = nullptr;

		// True from construction until the controls are filled, as in the MFC
		// palette, and raised again around anything that fills them.
		bool bCreateControls = true;
		// LVS_ICON as against LVS_LIST, which is all nStyle ever held there.
		bool bThumbnails = true;

		SObjectListElement selectedObjectListElement;
		CObjectListElementMap objectListElementMap;

		// The remembered filter name, in the on-disk struct it is remembered in.
		SDialogState dialogState;

	public:
		CMapObjectWxWindow()
		{
			selectedObjectListElement.szObjectTypeName.clear();
			selectedObjectListElement.objectDBID.Clear();
			Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_MAPOBJECT_WINDOW, this );
			Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMOOLCM_LIST, ID_MIMOOLCM_PROPERTIES );
		}

		virtual ~CMapObjectWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_MAPOBJECT_WINDOW );
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_MAPOBJECT_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pHostRoot = Root();

			// Scrolled for the reason the other palettes are: four fixed rows
			// and one list that takes the slack. A tab too short for the fixed
			// rows scrolls rather than squashing them.
			wxScrolledWindow *const pRoot = NWx::Child<wxScrolledWindow>( pHostRoot, wxID_ANY );
			pRoot->SetScrollRate( 0, 8 );
			wxBoxSizer *pHostSizer = new wxBoxSizer( wxVERTICAL );
			pHostSizer->Add( pRoot, wxSizerFlags( 1 ).Expand() );
			pHostRoot->SetSizer( pHostSizer );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// Player: [                    ]
			wxBoxSizer *pPlayerRow = new wxBoxSizer( wxHORIZONTAL );
			pPlayerRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Player:" ),
											 wxSizerFlags().Centre() );
			// CBS_SORT, so a player's position in the control is not its
			// position in playerList and the list index has to travel as client
			// data. Same as the field palette's combo.
			pPlayers = NWx::Child<wxChoice>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																			 0, nullptr, wxCB_SORT );
			pPlayerRow->Add( pPlayers, wxSizerFlags( 1 ).Centre().Border( wxLEFT, 4 ) );
			pSizer->Add( pPlayerRow, wxSizerFlags().Expand() );

			// Direction: (o) Random
			//            ( ) Fixed [   ] Degree
			wxBoxSizer *pDirectionRow = new wxBoxSizer( wxHORIZONTAL );
			pDirectionRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Direction:" ),
													wxSizerFlags().Border( wxTOP, 4 ) );
			wxBoxSizer *pDirectionColumn = new wxBoxSizer( wxVERTICAL );
			pDirectionRandom = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Random",
																										wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
			pDirectionColumn->Add( pDirectionRandom );
			wxBoxSizer *pFixedRow = new wxBoxSizer( wxHORIZONTAL );
			pDirectionFixed = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Fixed" );
			pFixedRow->Add( pDirectionFixed, wxSizerFlags().Centre() );
			// ES_READONLY in the template: the direction is set from the scene,
			// not typed. The palette only ever echoes it.
			pDirection = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY, wxEmptyString,
																					 wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
			pFixedRow->Add( pDirection, wxSizerFlags( 1 ).Centre().Border( wxLEFT, 4 ) );
			pFixedRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Degree" ),
											wxSizerFlags().Centre().Border( wxLEFT, 4 ) );
			pDirectionColumn->Add( pFixedRow, wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pDirectionRow->Add( pDirectionColumn, wxSizerFlags( 1 ).Border( wxLEFT, 4 ) );
			pSizer->Add( pDirectionRow, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			// IDC_TMIMO_DELIMITER_0 is an SS_ETCHEDHORZ static.
			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			// Filter: [                    ]
			wxBoxSizer *pFilterRow = new wxBoxSizer( wxHORIZONTAL );
			pFilterRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Filter:" ),
											 wxSizerFlags().Centre() );
			// No sort on this one: the filter list has an order of its own and
			// the combo shows it. The filter index still travels as client data,
			// because a separator entry can sit between two filters.
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

			pFilters->Bind( wxEVT_CHOICE, &CMapObjectWxWindow::OnFilterChanged, this );
			pDirectionRandom->Bind( wxEVT_RADIOBUTTON, &CMapObjectWxWindow::OnDirectionType, this );
			pDirectionFixed->Bind( wxEVT_RADIOBUTTON, &CMapObjectWxWindow::OnDirectionType, this );
			pDirection->Bind( wxEVT_TEXT, &CMapObjectWxWindow::OnDirectionChanged, this );
			pObjects->Bind( wxEVT_LIST_ITEM_SELECTED, &CMapObjectWxWindow::OnObjectSelected, this );
			pObjects->Bind( wxEVT_CONTEXT_MENU, &CMapObjectWxWindow::OnObjectContextMenu, this );
			// UpdateObjectsListStyle(), which the MFC palette runs from OnSize.
			pObjects->Bind( wxEVT_SIZE, &CMapObjectWxWindow::OnObjectListSize, this );

			// OnInitDialog's order: the list style first, then the filters, then
			// what the chosen filter selects.
			ApplyListStyle();
			NDialogState::Load( "CMapObjectWindow", &dialogState );
			FillFilterList();
			FillObjectList();
			bCreateControls = false;
			return true;
		}

		//	CMapObjectCommands
		virtual bool GetEditParameters( SEditParams *pEditParameters )
		{
			if ( pEditParameters == 0 )
			{
				return false;
			}
			if ( pEditParameters->nFlags & ( MIMOSEP_PLAYER_COUNT | MIMOSEP_PLAYER_INDEX ) )
			{
				if ( pEditParameters->nFlags & MIMOSEP_PLAYER_COUNT )
				{
					ReadPlayerList( &( pEditParameters->playerList ) );
				}
				if ( pEditParameters->nFlags & MIMOSEP_PLAYER_INDEX )
				{
					pEditParameters->nPlayerIndex = SelectedPlayer();
				}
			}
			if ( pEditParameters->nFlags & MIMOSEP_DIRECTION_TYPE )
			{
				pEditParameters->eDirectionType = pDirectionFixed->GetValue() ? SEditParams::DT_CUSTOM
																																			: SEditParams::DT_RANDOM;
			}
			if ( ( pEditParameters->nFlags & MIMOSEP_DIRECTION ) > 0 )
			{
				// Read what is in the box, and if it is not a number in range,
				// clamp what the caller already had and put that back on screen.
				// Note that an unparseable box leaves fDirection as the caller
				// passed it, which is then clamped: that is what the MFC palette
				// does and the difference matters, because the box can hold text
				// the state never wrote.
				const std::string szText( pDirection->GetValue().utf8_str() );
				if ( ( sscanf( szText.c_str(), "%g", &( pEditParameters->fDirection ) ) < 1 ) ||
						 ( pEditParameters->fDirection < 0.0f ) ||
						 ( pEditParameters->fDirection > 360.0f ) )
				{
					bCreateControls = true;
					if ( pEditParameters->fDirection < 0.0f )
					{
						pEditParameters->fDirection = 0.0f;
					}
					else if ( pEditParameters->fDirection > 360.0f )
					{
						pEditParameters->fDirection = 360.0f;
					}
					pDirection->SetValue( fmt::format( "{:g}", pEditParameters->fDirection ).c_str() );
					bCreateControls = false;
				}
			}
			if ( ( pEditParameters->nFlags & MIMOSEP_THUMBNAILS ) > 0 )
			{
				pEditParameters->bThumbnails = bThumbnails;
			}
			return true;
		}

		virtual bool SetEditParameters( const SEditParams &rEditParameters )
		{
			// No bCreateControls here, deliberately; see the note at the top.
			if ( rEditParameters.nFlags & ( MIMOSEP_PLAYER_COUNT | MIMOSEP_PLAYER_INDEX ) )
			{
				WritePlayerList( rEditParameters.playerList, rEditParameters.nPlayerIndex,
												 ( rEditParameters.nFlags & MIMOSEP_PLAYER_COUNT ) != 0,
												 ( rEditParameters.nFlags & MIMOSEP_PLAYER_INDEX ) != 0 );
			}
			if ( rEditParameters.nFlags & MIMOSEP_DIRECTION_TYPE )
			{
				if ( rEditParameters.eDirectionType == SEditParams::DT_CUSTOM )
				{
					pDirectionFixed->SetValue( true );
				}
				else
				{
					pDirectionRandom->SetValue( true );
				}
			}
			if ( ( rEditParameters.nFlags & MIMOSEP_DIRECTION ) > 0 )
			{
				pDirection->SetValue( fmt::format( "{:g}", rEditParameters.fDirection ).c_str() );
			}
			if ( ( rEditParameters.nFlags & MIMOSEP_THUMBNAILS ) > 0 )
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
				case ID_MIMO_CLEAR_SELECTION:
					ClearSelection();
					return true;
				case ID_OS_GET_OBJECTSET:
					if ( SObjectSet *pObjectSet = reinterpret_cast<SObjectSet*>( dwData ) )
					{
						pObjectSet->szObjectTypeName = selectedObjectListElement.szObjectTypeName;
						pObjectSet->objectNameSet.clear();
						pObjectSet->objectNameSet[selectedObjectListElement.objectDBID] = 0;
					}
					return true;
				case ID_MIMOOLCM_LIST:
					SetThumbnails( false );
					return true;
				case ID_MIMOOLCM_THUMBNAILS:
					SetThumbnails( true );
					return true;
				case ID_MIMOOLCM_PROPERTIES:
					ShowSelectedObjectProperties();
					return true;
				default:
					return CMapObjectCommands::HandleCommand( nCommandID, dwData );
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			NI_ASSERT( pbEnable != 0, "CMapObjectWxWindow::UpdateCommand(), pbEnable == 0" );
			NI_ASSERT( pbCheck != 0, "CMapObjectWxWindow::UpdateCommand(), pbCheck == 0" );
			//
			switch ( nCommandID )
			{
				case ID_MIMO_CLEAR_SELECTION:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				case ID_OS_GET_OBJECTSET:
				case ID_MIMOOLCM_PROPERTIES:
					( *pbEnable ) = HasSelectedObject();
					( *pbCheck ) = false;
					return true;
				case ID_MIMOOLCM_LIST:
					( *pbEnable ) = true;
					( *pbCheck ) = !bThumbnails;
					return true;
				case ID_MIMOOLCM_THUMBNAILS:
					( *pbEnable ) = true;
					( *pbCheck ) = bThumbnails;
					return true;
				default:
					return CMapObjectCommands::UpdateCommand( nCommandID, pbEnable, pbCheck );
			}
			return false;
		}

	private:
		// ------------------------------------------------------------------
		// the player combo
		// ------------------------------------------------------------------

		int PlayerAt( unsigned nPosition ) const
		{
			return (int)reinterpret_cast<uintptr_t>( pPlayers->GetClientData( nPosition ) );
		}

		int SelectedPlayer() const
		{
			const int nPosition = pPlayers->GetSelection();
			return ( nPosition == wxNOT_FOUND ) ? -1 : PlayerAt( nPosition );
		}

		// GetComboBoxEditParameters, for a wxChoice: the control is sorted, so
		// the list it reports is rebuilt in list order from the client data
		// rather than read off the control top to bottom.
		void ReadPlayerList( SEditParams::CPlayerList *pList ) const
		{
			if ( pList == 0 )
			{
				return;
			}
			std::vector<std::string> stringList( pPlayers->GetCount(), std::string() );
			for ( unsigned nPosition = 0; nPosition < pPlayers->GetCount(); ++nPosition )
			{
				const int nListIndex = PlayerAt( nPosition );
				if ( ( nListIndex >= 0 ) && ( nListIndex < static_cast<int>( stringList.size() ) ) )
				{
					stringList[nListIndex] = std::string( pPlayers->GetString( nPosition ).utf8_str() );
				}
			}
			( *pList ) = stringList;
		}

		// SetComboBoxEditParameters, for a wxChoice. Refilling loses the
		// selection, so it is remembered as a list index and looked up again.
		void WritePlayerList( const SEditParams::CPlayerList &rList, int nIndex,
													bool bCount, bool bIndex )
		{
			if ( !bCount && !bIndex )
			{
				return;
			}
			int nSelectedIndex = SelectedPlayer();
			if ( nSelectedIndex < 0 )
			{
				nSelectedIndex = 0;
			}
			if ( bCount )
			{
				pPlayers->Clear();
				for ( size_t nListIndex = 0; nListIndex < rList.size(); ++nListIndex )
				{
					pPlayers->Append( wxString::FromUTF8( rList[nListIndex].c_str() ),
														reinterpret_cast<void*>( static_cast<uintptr_t>( nListIndex ) ) );
				}
			}
			if ( bIndex )
			{
				nSelectedIndex = nIndex;
			}
			if ( nSelectedIndex > static_cast<int>( pPlayers->GetCount() ) - 1 )
			{
				nSelectedIndex = 0;
			}
			for ( unsigned nPosition = 0; nPosition < pPlayers->GetCount(); ++nPosition )
			{
				if ( PlayerAt( nPosition ) == nSelectedIndex )
				{
					pPlayers->SetSelection( nPosition );
					break;
				}
			}
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
			Singleton<IObjectFilterCollector>()->GetFilterList( &filterList, NMapObjectView::FILTER_TYPE );
			for ( size_t nFilterIndex = 0; nFilterIndex < filterList.size(); ++nFilterIndex )
			{
				pFilters->Append( wxString::FromUTF8( filterList[nFilterIndex].c_str() ),
													reinterpret_cast<void*>( static_cast<uintptr_t>( nFilterIndex ) ) );
			}
			if ( !filterList.empty() )
			{
				// The remembered one by name, or the second entry, which is what
				// the MFC palette picks: the first is a separator or an "all".
				//
				// FindString rather than SelectString: MFC's is a case-insensitive
				// *prefix* match, and this is looking up a whole name that came
				// out of this same list, so an exact match is what was meant and
				// cannot pick a different filter whose name starts the same way.
				const int nRemembered = dialogState.GetStringParameter( 0 ).empty()
						? wxNOT_FOUND
						: pFilters->FindString( wxString::FromUTF8( dialogState.GetStringParameter( 0 ).c_str() ), false );
				if ( nRemembered != wxNOT_FOUND )
				{
					pFilters->SetSelection( nRemembered );
				}
				else if ( filterList.size() > 1 )
				{
					pFilters->SetSelection( 1 );
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
		// see the note in HeightViewV3Wx.cpp, which does the same thing for the
		// same reason. wx creates its list controls with LVS_SHAREIMAGELISTS, so
		// nothing here takes ownership.
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
			if ( Singleton<IObjectFilterCollector>()->IsSeparator( NMapObjectView::FILTER_TYPE, nFilterIndex ) )
			{
				return;
			}
			const IObjectFilter *pObjectFilter =
					Singleton<IObjectFilterCollector>()->Get( NMapObjectView::FILTER_TYPE, nFilterIndex );
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
			unsigned nObjectsCount = 0;
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
					pObjects->SetItemData( nItem, static_cast<long>( nObjectsCount ) );
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
			const unsigned nKey = static_cast<unsigned>( pObjects->GetItemData( nItem ) );
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
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<uintptr_t>( &( selectedObjectListElement.szObjectTypeName ) ) );
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );
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
			objectSet.objectNameSet[selectedObjectListElement.objectDBID] = 0;
			NObjectProperties::Show( objectSet );
		}

		// ------------------------------------------------------------------
		// events
		// ------------------------------------------------------------------

		void Report( unsigned nFlag )
		{
			if ( bCreateControls )
			{
				return;
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,
																														ID_GET_EDIT_PARAMETERS, nFlag );
		}

		void OnFilterChanged( wxCommandEvent& )
		{
			if ( bCreateControls )
			{
				return;
			}
			FillObjectList();
			// SelectedFilterIndex, called from FillObjectList, has just recorded
			// the new name; this is what puts it on disk.
			NDialogState::Save( "CMapObjectWindow", &dialogState );
		}

		void OnDirectionType( wxCommandEvent& ) { Report( MIMOSEP_DIRECTION_TYPE ); }
		void OnDirectionChanged( wxCommandEvent& ) { Report( MIMOSEP_DIRECTION ); }
		void OnObjectSelected( wxListEvent& ) { UpdateSelection(); }

		void OnObjectListSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			pObjects->Arrange();
		}

		void OnObjectContextMenu( wxContextMenuEvent &rEvent )
		{
			// IDM_MAPINFO_CONTEXT_MENU's MI_MAPOBJECT_OBJECT_LIST popup, built in
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
			menu.Bind( wxEVT_MENU, &CMapObjectWxWindow::OnMenu, this );

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


namespace NMapObjectView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CMapObjectWxWindow *pWindow = pTabWindow->AddNewTab( new CMapObjectWxWindow() );
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
