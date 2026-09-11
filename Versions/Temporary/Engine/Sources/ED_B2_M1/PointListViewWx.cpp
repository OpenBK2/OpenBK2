#include "stdafx.h"

#include "PointListView.h"

#ifdef OBK2_WITH_WX

#include <fmt/format.h>

#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"
#include "SeasonMnemonics.h"

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/utils.h>

#include <cstdint>

// One of the building editor's point lists, in wx: IDD_TAB_BLD_POINTS, five
// times over. Two check boxes, a season and the list of points.
//
// The same shape as the formation palette, whose MFC class was copied from this
// one: a host window in the tab, a report-mode list that fills what is left,
// and the remembered selection that survives the state refilling the list. What
// is particular to this one is that the states address it by instance ID
// through NPointListView, which both implementations register with, and that
// setting one list's data moves the others' season to match.
//
// Things kept exactly, because the states read them back:
//
//   * the row labels -- "<label> {:3d}", and for damage levels (instance 4) a
//     "Full health" row first and then "<label> {}" unpadded;
//   * the selected row as the state last set it, remembered even while the list
//     is empty, and learnt from the selection event even while the list is
//     being filled -- the MFC list set nSelectedIndex in its LVN_ITEMCHANGED
//     handler before it looked at bIsDataSetting;
//   * the season as the item data of the selected entry, and -1 with none;
//   * the season list sorted, as CBS_SORT with AddString sorts it -- wx passes
//     wxCB_SORT to the same native combo box.

namespace
{
	class CPointListWxWindow : public CWxHostWindow, public NPointListView::IPointList
	{
		const unsigned nInstanceID;
		const std::string szLabel;

		wxCheckBox *pPropMask = nullptr;
		wxCheckBox *pPassability = nullptr;
		wxChoice *pSeason = nullptr;
		wxListCtrl *pPoints = nullptr;

		int nSelectedIndex = -1;
		// Filling the list raises the same selection events the user does.
		bool bIsDataSetting = false;

		// The entry holding eSeason as its data, or wxNOT_FOUND.
		int FindSeasonByValue( NDb::ESeason eSeason ) const
		{
			for ( unsigned int nItem = 0; nItem < pSeason->GetCount(); ++nItem )
			{
				if ( static_cast<NDb::ESeason>( reinterpret_cast<intptr_t>( pSeason->GetClientData( nItem ) ) ) == eSeason )
				{
					return static_cast<int>( nItem );
				}
			}
			return wxNOT_FOUND;
		}

		void NotifyHandler()
		{
			if ( bIsDataSetting )
			{
				return;
			}
			// CWaitCursor in the MFC list: the state reloads and redraws.
			wxBusyCursor wait;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_POINTS_LIST_DLG_LISTENER,
																														ID_POINTS_LIST_DLG_CHANGE_STATE,
																														nInstanceID );
		}

		void OnPointSelected( wxListEvent &rEvent )
		{
			// Before the bIsDataSetting check in NotifyHandler, as in the MFC
			// handler: a row selected while filling is still the selected row.
			nSelectedIndex = rEvent.GetIndex();
			NotifyHandler();
		}

		void OnListResized( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			// One column across the list, as OnInitDialog gave it the client
			// width -- kept true on every resize rather than taken once before
			// the palette has a size.
			if ( pPoints->GetColumnCount() > 0 )
			{
				pPoints->SetColumnWidth( 0, pPoints->GetClientSize().x );
			}
		}

	public:
		CPointListWxWindow( unsigned _nInstanceID, const std::string &rszLabel )
			: nInstanceID( _nInstanceID ), szLabel( rszLabel )
		{
			NPointListView::Register( this );
		}

		virtual ~CPointListWxWindow()
		{
			NPointListView::Unregister( this );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();

			// The template top to bottom: the two check boxes, the season row, and
			// the list, which is the only thing the MFC constructor lets grow in
			// both directions. The season combo grows across.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			pPropMask = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Property mask" );
			pSizer->Add( pPropMask, wxSizerFlags().Border( wxBOTTOM, 4 ) );
			pPassability = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Passability" );
			pSizer->Add( pPassability, wxSizerFlags().Border( wxBOTTOM, 6 ) );

			wxBoxSizer *pSeasonRow = new wxBoxSizer( wxHORIZONTAL );
			pSeasonRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Season:" ),
											 wxSizerFlags().CentreVertical().Border( wxRIGHT, 6 ) );
			pSeason = NWx::Child<wxChoice>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																			0, nullptr, wxCB_SORT );
			for ( int nSeason = 0; nSeason < typeSeasonMnemonics.Size(); ++nSeason )
			{
				// Append answers with where the sorted list put it.
				const int nItem = pSeason->Append( wxString::FromUTF8( typeSeasonMnemonics.GetMnemonic( nSeason ).c_str() ) );
				pSeason->SetClientData( nItem, reinterpret_cast<void*>( static_cast<intptr_t>( nSeason ) ) );
			}
			pSeasonRow->Add( pSeason, wxSizerFlags( 1 ).CentreVertical() );
			pSizer->Add( pSeasonRow, wxSizerFlags().Expand().Border( wxBOTTOM, 4 ) );

			// LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, and GRIDLINES,
			// FULLROWSELECT and INFOTIP on top: wx sets SHOWSELALWAYS and
			// FULLROWSELECT itself and LABELTIP for the tips, and draws the rules.
			pPoints = NWx::Child<wxListCtrl>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																				wxLC_REPORT | wxLC_SINGLE_SEL |
																				wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN );
			pPoints->InsertColumn( 0, "Points", wxLIST_FORMAT_CENTER );
			pPoints->Bind( wxEVT_SIZE, &CPointListWxWindow::OnListResized, this );
			pSizer->Add( pPoints, wxSizerFlags( 1 ).Expand() );

			pRoot->SetSizer( pSizer );

			pPoints->Bind( wxEVT_LIST_ITEM_SELECTED, &CPointListWxWindow::OnPointSelected, this );
			pPropMask->Bind( wxEVT_CHECKBOX, [this]( wxCommandEvent & ) { NotifyHandler(); } );
			pPassability->Bind( wxEVT_CHECKBOX, [this]( wxCommandEvent & ) { NotifyHandler(); } );
			pSeason->Bind( wxEVT_CHOICE, [this]( wxCommandEvent & ) { NotifyHandler(); } );
			return true;
		}

		//	NPointListView::IPointList
		virtual unsigned GetInstanceID() const
		{
			return nInstanceID;
		}

		virtual void GetDialogData( SPointListDialogData *pData )
		{
			// A list whose Build failed stays registered in the tab list and
			// here, with no controls; it answers nothing.
			if ( pPoints == nullptr )
			{
				return;
			}
			pData->nInstanceID = nInstanceID;
			pData->nNumPoints = pPoints->GetItemCount();
			// The remembered row, not a scan of the list: the MFC list stopped
			// scanning and kept the index its selection handler last saw.
			pData->nSelectedPoint = nSelectedIndex;
			pData->bChkPassability = pPassability->GetValue();
			pData->bChkPropmask = pPropMask->GetValue();
			pData->eSeason = static_cast<NDb::ESeason>( -1 );
			const int nSeason = pSeason->GetSelection();
			if ( nSeason != wxNOT_FOUND )
			{
				pData->eSeason = static_cast<NDb::ESeason>( reinterpret_cast<intptr_t>( pSeason->GetClientData( nSeason ) ) );
			}
		}

		virtual void SetDialogData( const SPointListDialogData *pData )
		{
			// The dispatch only hands a list its own data; the MFC list asserted
			// this and returned, leaving its guard flag set for good. Returning
			// with it clear is the only sane reading of that.
			if ( ( static_cast<unsigned>( pData->nInstanceID ) != nInstanceID ) || ( pPoints == nullptr ) )
			{
				return;
			}
			bIsDataSetting = true;

			pPoints->DeleteAllItems();
			if ( nInstanceID == 4 ) // damage levels
			{
				pPoints->InsertItem( 0, "Full health" );
				for ( int nPoint = 0; nPoint < pData->nNumPoints; ++nPoint )
				{
					pPoints->InsertItem( nPoint + 1, wxString::FromUTF8( fmt::format( "{} {}", szLabel, nPoint ).c_str() ) );
				}
			}
			else	// all other lists
			{
				for ( int nPoint = 0; nPoint < pData->nNumPoints; ++nPoint )
				{
					pPoints->InsertItem( nPoint, wxString::FromUTF8( fmt::format( "{} {:3d}", szLabel, nPoint ).c_str() ) );
				}
			}
			// CListCtrl::SetItemState: -1 means every row, which a single
			// selection list refuses, and a row past the end is refused too. Both
			// are nothing, so only a real row is asked for here.
			if ( ( pData->nNumPoints > 0 ) && ( pData->nSelectedPoint >= 0 ) && ( pData->nSelectedPoint < pPoints->GetItemCount() ) )
			{
				pPoints->SetItemState( pData->nSelectedPoint, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			}
			pPassability->SetValue( pData->bChkPassability );
			pPropMask->SetValue( pData->bChkPropmask );
			// Found by value and left alone when no entry has it, as the MFC
			// loop over the item data did.
			const int nSeason = FindSeasonByValue( pData->eSeason );
			if ( nSeason != wxNOT_FOUND )
			{
				pSeason->SetSelection( nSeason );
			}
			NPointListView::FollowSeason( this, pData->eSeason );

			bIsDataSetting = false;
		}

		// By name, as the MFC loop set the other lists: an unknown season names
		// the default mnemonic, and whichever entry has that name is selected.
		virtual void FollowSeason( NDb::ESeason eSeason )
		{
			if ( pSeason == nullptr )
			{
				return;
			}
			const int nSeason = pSeason->FindString( wxString::FromUTF8( typeSeasonMnemonics.GetMnemonic( eSeason ).c_str() ) );
			if ( nSeason != wxNOT_FOUND )
			{
				pSeason->SetSelection( nSeason );
			}
		}
	};
}


namespace NPointListView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow, unsigned nInstanceID, const std::string &rszLabel )
	{
		// Handed over as a CWnd: AddNewTab<T> also compiles a `new T()` for a null
		// argument, and this class has no default constructor -- it is nothing
		// without its instance ID.
		CPointListWxWindow *pWindow = new CPointListWxWindow( nInstanceID, rszLabel );
		pTabWindow->AddNewTab<CWnd>( pWindow );
		if ( !pWindow->Build( pTabWindow ) )
		{
			// Left in the tab list deliberately: it is the list's to delete.
			return 0;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
