#include "stdafx.h"

#include "FieldView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "FieldState.h"
#include "ResourceDefines.h"

#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <string>
#include <vector>

// The field palette, in wx: the sixth palette, and the first of the five that
// speak edit parameters rather than a dialog struct.
//
// The difference that makes is worth stating, because it is not visible in the
// controls. A dialog-data palette is asked for all of its state at once. This
// one is asked about a subset: every exchange carries nFlags saying which
// fields it is about, and the palette must touch those and leave the rest of
// the struct exactly as the caller left it. Every `if ( nFlags & ... )` below
// is that, not defensiveness.
//
// The three clamps in GetEditParameters are the odd ones out. MITFEP_MIN_LENGTH,
// MITFEP_WIDTH and MITFEP_DISTURBANCE have no controls in this palette -- the
// template never had them -- so the palette does not read them from anywhere,
// it only bounds what the state already holds. That is where the bounds have
// always lived, and moving them would be a change to the state's behaviour
// rather than to the palette's, so they stay.

namespace
{
	class CFieldWxWindow : public CWxHostWindow, public CFieldCommands
	{
		wxRadioButton *pMoveSingle = nullptr;
		wxRadioButton *pMoveMultiple = nullptr;
		wxRadioButton *pMoveAll = nullptr;
		wxChoice *pFields = nullptr;
		wxCheckBox *pRandomize = nullptr;
		wxCheckBox *pFillTerrain = nullptr;
		wxCheckBox *pFillObjects = nullptr;
		wxCheckBox *pFillHeights = nullptr;
		// The same guard the MFC palette has, under the same name: filling the
		// controls raises the change events the user does, and those must not be
		// reported back as edits.
		bool bCreateControls = false;

	public:
		CFieldWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_FIELD_WINDOW, this );
		}

		virtual ~CFieldWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pHostRoot = Root();

			// **Why this palette scrolls and the earlier ones do not.** Every
			// control in CFieldWindow's constructor carries the same anchor --
			// ANCHORE_LEFT_TOP | RESIZE_HOR -- and there are eleven of them, so
			// the whole palette is one column of full-width rows that grows
			// sideways and never vertically. Nothing in it can absorb a short
			// window the way the reinforcement points list does.
			//
			// It needs 178 pixels and the shortcut bar here gives its tabs 173.
			// The MFC palette answers that by clipping: the dialog is its
			// template's size and "Modify Terrain Heights" is simply outside the
			// tab, gone. A sizer answers it by shrinking an item below its
			// minimum, which put that check box on screen 10 pixels tall with its
			// text cut through the middle -- measurably worse than the palette it
			// replaces, and the reason this was not left alone.
			//
			// A scrolled window is what makes neither happen: every control keeps
			// the size it asked for, and a bar too short to hold them all scrolls
			// to the rest instead of hiding it. Where the bar is tall enough --
			// which is most of the time -- there is no scrollbar and nothing about
			// this is visible.
			wxScrolledWindow *const pRoot =
					NWx::Child<wxScrolledWindow>( pHostRoot, wxID_ANY );
			pRoot->SetScrollRate( 0, 8 );
			wxBoxSizer *pHostSizer = new wxBoxSizer( wxVERTICAL );
			pHostSizer->Add( pRoot, wxSizerFlags( 1 ).Expand() );
			pHostRoot->SetSizer( pHostSizer );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// wxRB_GROUP on the first, which is what WS_GROUP says in the
			// template: the three are one group and exactly one is set.
			pMoveSingle = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Move Single Point",
																							 wxDefaultPosition, wxDefaultSize,
																							 wxRB_GROUP );
			pSizer->Add( pMoveSingle, wxSizerFlags().Expand() );
			pMoveMultiple = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Move Multiple Points" );
			pSizer->Add( pMoveMultiple, wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pMoveAll = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Move All Points" );
			pSizer->Add( pMoveAll, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			// IDC_TMITF_DELIMITER_0 and _1 are SS_ETCHEDHORZ statics, which is
			// what a wxStaticLine draws.
			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Available fields:" ),
									 wxSizerFlags().Expand() );
			// CBS_SORT is wxCB_SORT, and it is the reason the list index has to
			// travel as client data: sorted, a field's position in the control is
			// not its position in fieldList, and the state knows it only by the
			// latter.
			pFields = NWx::Child<wxChoice>( pRoot, wxID_ANY, wxDefaultPosition,
																			wxDefaultSize, 0, nullptr, wxCB_SORT );
			pSizer->Add( pFields, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			pRandomize = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Randomize Polygon" );
			pSizer->Add( pRandomize, wxSizerFlags().Expand().Border( wxTOP, 4 ) );

			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pFillTerrain = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Fill Terrain" );
			pSizer->Add( pFillTerrain, wxSizerFlags().Expand() );
			pFillObjects = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Place Objects" );
			pSizer->Add( pFillObjects, wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pFillHeights = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Modify Terrain Heights" );
			pSizer->Add( pFillHeights, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			// SetSizer rather than SetSizerAndFit, and FitInside so the scrolled
			// window learns the height its contents want: that is what the
			// scrollbar's range is computed from.
			pRoot->SetSizer( pSizer );
			pRoot->FitInside();

			// The MFC palette maps all three radios to one handler and each check
			// box to its own, because what travels is the flag saying which field
			// changed. Same here.
			pMoveSingle->Bind( wxEVT_RADIOBUTTON, &CFieldWxWindow::OnMoveType, this );
			pMoveMultiple->Bind( wxEVT_RADIOBUTTON, &CFieldWxWindow::OnMoveType, this );
			pMoveAll->Bind( wxEVT_RADIOBUTTON, &CFieldWxWindow::OnMoveType, this );
			pFields->Bind( wxEVT_CHOICE, &CFieldWxWindow::OnFieldChanged, this );
			pRandomize->Bind( wxEVT_CHECKBOX, &CFieldWxWindow::OnRandomize, this );
			pFillTerrain->Bind( wxEVT_CHECKBOX, &CFieldWxWindow::OnFillTerrain, this );
			pFillObjects->Bind( wxEVT_CHECKBOX, &CFieldWxWindow::OnFillObjects, this );
			pFillHeights->Bind( wxEVT_CHECKBOX, &CFieldWxWindow::OnFillHeights, this );
			return true;
		}

		//	CFieldCommands
		virtual bool GetEditParameters( CFieldState::SEditParameters *pEditParameters )
		{
			if ( pEditParameters == 0 )
			{
				return false;
			}
			if ( pEditParameters->nFlags & MITFEP_MOVE_TYPE )
			{
				pEditParameters->eMoveType = CheckedMoveType();
			}
			if ( pEditParameters->nFlags & ( MITFEP_FIELD_COUNT | MITFEP_FIELD_INDEX ) )
			{
				if ( pEditParameters->nFlags & MITFEP_FIELD_COUNT )
				{
					ReadFieldList( &( pEditParameters->fieldList ) );
				}
				if ( pEditParameters->nFlags & MITFEP_FIELD_INDEX )
				{
					pEditParameters->nFieldIndex = SelectedField();
				}
			}
			if ( pEditParameters->nFlags & MITFEP_RANDOMIZE )
			{
				pEditParameters->bRandomize = pRandomize->GetValue();
			}
			// No controls behind these three; see the note at the top.
			if ( pEditParameters->nFlags & MITFEP_MIN_LENGTH )
			{
				pEditParameters->fMinLength = Clamp( pEditParameters->fMinLength, 3.0f, 32.0f * 16.0f );
			}
			if ( pEditParameters->nFlags & MITFEP_WIDTH )
			{
				pEditParameters->fWidth = Clamp( pEditParameters->fWidth, 0.0f, 0.5f );
			}
			if ( pEditParameters->nFlags & MITFEP_DISTURBANCE )
			{
				pEditParameters->fDisturbance = Clamp( pEditParameters->fDisturbance, 0.0f, 1.0f );
			}
			if ( pEditParameters->nFlags & MITFEP_FILL_TERRAIN )
			{
				pEditParameters->bFillTerrain = pFillTerrain->GetValue();
			}
			if ( pEditParameters->nFlags & MITFEP_FILL_OBJECTS )
			{
				pEditParameters->bFillObjects = pFillObjects->GetValue();
			}
			if ( pEditParameters->nFlags & MITFEP_FILL_HEIGHTS )
			{
				pEditParameters->bFillHeights = pFillHeights->GetValue();
			}
			if ( pEditParameters->nFlags & MITFEP_UPDATE_MAP )
			{
				pEditParameters->bUpdateMap = true;
			}
			return true;
		}

		virtual bool SetEditParameters( const CFieldState::SEditParameters &rEditParameters )
		{
			bCreateControls = true;
			if ( rEditParameters.nFlags & MITFEP_MOVE_TYPE )
			{
				CheckMoveType( rEditParameters.eMoveType );
			}
			if ( rEditParameters.nFlags & ( MITFEP_FIELD_COUNT | MITFEP_FIELD_INDEX ) )
			{
				WriteFieldList( rEditParameters.fieldList, rEditParameters.nFieldIndex,
												( rEditParameters.nFlags & MITFEP_FIELD_COUNT ) != 0,
												( rEditParameters.nFlags & MITFEP_FIELD_INDEX ) != 0 );
			}
			if ( rEditParameters.nFlags & MITFEP_RANDOMIZE )
			{
				pRandomize->SetValue( rEditParameters.bRandomize );
			}
			if ( rEditParameters.nFlags & MITFEP_FILL_TERRAIN )
			{
				pFillTerrain->SetValue( rEditParameters.bFillTerrain );
			}
			if ( rEditParameters.nFlags & MITFEP_FILL_OBJECTS )
			{
				pFillObjects->SetValue( rEditParameters.bFillObjects );
			}
			if ( rEditParameters.nFlags & MITFEP_FILL_HEIGHTS )
			{
				pFillHeights->SetValue( rEditParameters.bFillHeights );
			}
			bCreateControls = false;
			return true;
		}

	private:
		static float Clamp( const float fValue, const float fMin, const float fMax )
		{
			if ( fValue < fMin )
			{
				return fMin;
			}
			if ( fValue > fMax )
			{
				return fMax;
			}
			return fValue;
		}

		// GetCheckedRadioButton()'s answer, without the arithmetic on control
		// ids that turned it into an EMoveType. With none of the three set that
		// arithmetic gives 0 - IDC_TMITF_MOVE_SINGLE_RADIO, a large negative
		// cast to the enum; this answers MT_SINGLE, which is what the enum's
		// zero is and what the state defaults to anyway.
		CPolygonState::EMoveType CheckedMoveType() const
		{
			if ( pMoveMultiple->GetValue() )
			{
				return CPolygonState::MT_MULTI;
			}
			if ( pMoveAll->GetValue() )
			{
				return CPolygonState::MT_ALL;
			}
			return CPolygonState::MT_SINGLE;
		}

		void CheckMoveType( const CPolygonState::EMoveType eMoveType )
		{
			switch ( eMoveType )
			{
				case CPolygonState::MT_MULTI:
					pMoveMultiple->SetValue( true );
					break;
				case CPolygonState::MT_ALL:
					pMoveAll->SetValue( true );
					break;
				default:
					pMoveSingle->SetValue( true );
					break;
			}
		}

		// The list index a control position carries, which is what the state
		// names a field by.
		int FieldAt( unsigned nPosition ) const
		{
			return (int)reinterpret_cast<uintptr_t>( pFields->GetClientData( nPosition ) );
		}

		int SelectedField() const
		{
			const int nPosition = pFields->GetSelection();
			return ( nPosition == wxNOT_FOUND ) ? -1 : FieldAt( nPosition );
		}

		// GetComboBoxEditParameters, for a wxChoice: the control is sorted, so
		// the list it reports is rebuilt in list order from the client data
		// rather than read off the control top to bottom.
		void ReadFieldList( CFieldState::SEditParameters::CFieldList *pList ) const
		{
			std::vector<std::string> stringList( pFields->GetCount(), std::string() );
			for ( unsigned nPosition = 0; nPosition < pFields->GetCount(); ++nPosition )
			{
				const int nListIndex = FieldAt( nPosition );
				if ( ( nListIndex >= 0 ) && ( nListIndex < static_cast<int>( stringList.size() ) ) )
				{
					stringList[nListIndex] = std::string( pFields->GetString( nPosition ).utf8_str() );
				}
			}
			( *pList ) = stringList;
		}

		// SetComboBoxEditParameters, for a wxChoice. Refilling the control loses
		// the selection, so the selected field is remembered as a list index --
		// the one thing that survives a re-sort -- and looked up again after.
		void WriteFieldList( const CFieldState::SEditParameters::CFieldList &rList,
												 const int nIndex, const bool bCount, const bool bIndex )
		{
			if ( !bCount && !bIndex )
			{
				return;
			}
			int nSelectedIndex = SelectedField();
			if ( nSelectedIndex < 0 )
			{
				nSelectedIndex = 0;
			}
			if ( bCount )
			{
				pFields->Clear();
				for ( size_t nListIndex = 0; nListIndex < rList.size(); ++nListIndex )
				{
					pFields->Append( wxString::FromUTF8( rList[nListIndex].c_str() ),
													 reinterpret_cast<void*>( static_cast<uintptr_t>( nListIndex ) ) );
				}
			}
			if ( bIndex )
			{
				nSelectedIndex = nIndex;
			}
			if ( nSelectedIndex > static_cast<int>( pFields->GetCount() ) - 1 )
			{
				nSelectedIndex = 0;
			}
			for ( unsigned nPosition = 0; nPosition < pFields->GetCount(); ++nPosition )
			{
				if ( FieldAt( nPosition ) == nSelectedIndex )
				{
					pFields->SetSelection( nPosition );
					break;
				}
			}
		}

		// One command, to one state, with the flag naming what changed. The
		// state reads that field back out of the palette and decides what it
		// means; the palette does not know.
		void Report( const unsigned nFlag )
		{
			if ( bCreateControls )
			{
				return;
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE,
																														ID_GET_EDIT_PARAMETERS, nFlag );
		}

		void OnMoveType( wxCommandEvent& ) { Report( MITFEP_MOVE_TYPE ); }
		void OnFieldChanged( wxCommandEvent& ) { Report( MITFEP_FIELD_INDEX ); }
		void OnRandomize( wxCommandEvent& ) { Report( MITFEP_RANDOMIZE ); }
		void OnFillTerrain( wxCommandEvent& ) { Report( MITFEP_FILL_TERRAIN ); }
		void OnFillObjects( wxCommandEvent& ) { Report( MITFEP_FILL_OBJECTS ); }
		void OnFillHeights( wxCommandEvent& ) { Report( MITFEP_FILL_HEIGHTS ); }
	};
}


namespace NFieldView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		// AddNewTab with a pointer rather than a null one: the template only
		// allocates when handed nothing, and this needs building before it is
		// registered. Either way the tab list owns it from here.
		CFieldWxWindow *pWindow = pTabWindow->AddNewTab( new CFieldWxWindow() );
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
