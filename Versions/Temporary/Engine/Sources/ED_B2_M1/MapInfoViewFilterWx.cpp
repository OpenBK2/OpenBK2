#include "stdafx.h"

#include "MapInfoViewFilter.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <string>

// View -> Filter, in wx: IDD_DLG_MAPINFO_VIEW_FILTER, "MapInfo View Filter".
//
// A fixed-size dialog like the others -- the template has no WS_THICKFRAME --
// fitted to its contents and centred, keeping no placement. What makes it
// different from the ones before it is that it is live: every click writes the
// whole filter back and sends ID_VIEW_APPLY_MI_FILTER, so the map redraws under
// the dialog, and Cancel undoes it all. That is kept exactly, including the
// order: the dialog writes the filter, then asks for it to be applied.
//
// What the MFC dialog did that this one does on purpose:
//
//   * **every change reads everything.** GetDialogData rebuilt the whole filter
//     from the controls, whichever one moved; ReadFilter does the same, so a
//     change can never leave the filter half-updated.
//   * **Default Settings** resets the filter, shows it, and applies it.
//   * **OK** reads and applies once more before closing, as OnOK did; the
//     caller applies again after that, as ConfigureViewFilter always has.
//   * **Cancel**, Escape and the close box all restore the filter as it was on
//     entry and apply that. MFC routed all three to OnCancel; here all three
//     come back from ShowModal as something other than wxID_OK.
//   * **where the focus starts.** The template lists OK first, so the MFC
//     dialog opens with OK focused; that is set explicitly here.
//
// And what it does differently:
//
//   * **"Show statistics" is not built.** The template has it NOT WS_VISIBLE,
//     so nobody has ever seen or clicked it, and all the MFC dialog did with it
//     was copy bShowStats out and back in again. Leaving the field alone is the
//     same thing; Default Settings still resets it, because SetDefault does.
//   * **The grid size applies when the choice is made**, not while the list is
//     open. The MFC combo box answered CBN_SELCHANGE, which fires for each item
//     the mouse passes over in an open drop-down; wxChoice raises its event on
//     CBN_SELENDOK. With two items, one of which is already selected, that is
//     the difference between previewing a size and choosing it.
//   * **Selecting a type in the list applies nothing.** LVN_ITEMCHANGED fired
//     for selection changes as well as for checks and the MFC dialog applied
//     for both; only a check changes the filter, so only a check applies it.

namespace
{
	class CMapInfoViewFilterWxDialog : public CWxToolDialog
	{
		CMapInfoEditorSettings::SViewFilterData *pFilter = nullptr;
		// Set while the controls are being filled from the filter: CheckItem
		// raises the same event a click does, and a half-filled list must not be
		// read back. The MFC dialog's bIsDataSetting, for the same reason.
		bool bFilling = false;

		wxListCtrl *pTypes = nullptr;
		wxCheckBox *pGrid = nullptr;
		wxChoice *pGridSize = nullptr;
		wxCheckBox *pBoundingBoxes = nullptr;
		wxCheckBox *pWireFrame = nullptr;
		wxCheckBox *pMipmap = nullptr;
		wxCheckBox *pOverdraw = nullptr;
		wxCheckBox *pShadows = nullptr;
		wxCheckBox *pWarfog = nullptr;
		wxCheckBox *pTerrain = nullptr;

		// CB_SELECTSTRING, which both the MFC OnInitDialog and SetDialogData
		// used: the first item that starts with the text, ignoring case, and the
		// selection left alone when nothing does. The stored value is always a
		// whole item name in practice, since this dialog is what writes it.
		void SelectGridSize( const std::string &rszGridSize )
		{
			const wxString szPrefix = wxString::FromUTF8( rszGridSize.c_str() ).Lower();
			for ( unsigned int nItem = 0; nItem < pGridSize->GetCount(); ++nItem )
			{
				if ( pGridSize->GetString( nItem ).Lower().StartsWith( szPrefix ) )
				{
					pGridSize->SetSelection( nItem );
					return;
				}
			}
		}

		// SetDialogData: the controls from the filter.
		void Fill()
		{
			bFilling = true;
			pTypes->DeleteAllItems();
			for ( int nType = 0; nType < static_cast<int>( pFilter->objTypeFilter.size() ); ++nType )
			{
				const long nItem = pTypes->InsertItem( nType, wxString::FromUTF8( pFilter->objTypeFilter[nType].szObjTypeName.c_str() ) );
				pTypes->CheckItem( nItem, pFilter->objTypeFilter[nType].bShow );
			}
			pGrid->SetValue( pFilter->bShowGrid );
			pBoundingBoxes->SetValue( pFilter->bShowBBoxes );
			pWireFrame->SetValue( pFilter->bWireFrame );
			pTerrain->SetValue( pFilter->bShowTerrain );
			pShadows->SetValue( pFilter->bShowShadows );
			pWarfog->SetValue( pFilter->bShowWarfog );
			pMipmap->SetValue( pFilter->bMipmap );
			pOverdraw->SetValue( pFilter->bOverdraw );
			SelectGridSize( pFilter->szGridSize );
			bFilling = false;
		}

		// A control changed: GetDialogData's read and its Apply.
		void OnChanged()
		{
			if ( bFilling )
			{
				return;
			}
			ReadFilter();
			NMapInfoViewFilter::Apply();
		}

		void OnDefault()
		{
			pFilter->SetDefault();
			Fill();
			OnChanged();
		}

		wxCheckBox* AddCheck( const char *pszLabel )
		{
			wxCheckBox *pCheck = NWx::Child<wxCheckBox>( this, wxID_ANY, pszLabel );
			pCheck->Bind( wxEVT_CHECKBOX, [this]( wxCommandEvent & ) { OnChanged(); } );
			return pCheck;
		}

	public:
		CMapInfoViewFilterWxDialog( CMapInfoEditorSettings::SViewFilterData *_pFilter )
			: CWxToolDialog( nullptr, wxID_ANY, "MapInfo View Filter" ),
			pFilter( _pFilter )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Object types:" ),
									 wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );
			// LVS_REPORT with LVS_NOCOLUMNHEADER, and LVS_EX_GRIDLINES and
			// LVS_EX_CHECKBOXES on top; FULLROWSELECT is wx's default on MSW. The
			// template's size, since the dialog is fitted around it.
			pTypes = NWx::Child<wxListCtrl>( this, wxID_ANY, wxDefaultPosition,
																			 ConvertDialogToPixels( wxSize( 167, 114 ) ),
																			 wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL |
																			 wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN );
			pTypes->EnableCheckBoxes();
			// One column, 200 pixels, as OnInitDialog gave it: four fifths of the
			// list, with the gridline showing where it ends. FromDIP is the same
			// 200 in this DPI-unaware process and keeps the proportion if that
			// ever changes.
			pTypes->InsertColumn( 0, wxString(), wxLIST_FORMAT_LEFT, pTypes->FromDIP( 200 ) );
			pTypes->Bind( wxEVT_LIST_ITEM_CHECKED, [this]( wxListEvent & ) { OnChanged(); } );
			pTypes->Bind( wxEVT_LIST_ITEM_UNCHECKED, [this]( wxListEvent & ) { OnChanged(); } );
			// A sizer item has one border width for all its sides, so the gaps
			// that differ from the side margins are spacers.
			pSizer->AddSpacer( 2 );
			pSizer->Add( pTypes, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 8 ) );

			// Grid on the left, its size on the right under the list's edge.
			wxBoxSizer *pGridRow = new wxBoxSizer( wxHORIZONTAL );
			pGrid = AddCheck( "Grid" );
			pGridRow->Add( pGrid, wxSizerFlags().CentreVertical() );
			pGridRow->AddStretchSpacer();
			pGridRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Size:" ),
										 wxSizerFlags().CentreVertical().Border( wxRIGHT, 6 ) );
			// Not sorted. The template says CBS_SORT, but OnInitDialog filled it
			// with InsertString, which puts an item where it is told and never
			// sorts, so the running dialog lists "Visual tile" first -- measured,
			// not read off the template.
			pGridSize = NWx::Child<wxChoice>( this, wxID_ANY, wxDefaultPosition,
																				wxSize( ConvertDialogToPixels( wxSize( 80, 0 ) ).x, -1 ) );
			pGridSize->Append( wxString::FromUTF8( RCSTR( "Visual tile" ) ) );
			pGridSize->Append( wxString::FromUTF8( RCSTR( "AI tile" ) ) );
			// What OnInitDialog selected before SetDialogData looked at the
			// filter, and so what stays selected if the filter names neither.
			SelectGridSize( RCSTR( "Visual tile" ) );
			pGridSize->Bind( wxEVT_CHOICE, [this]( wxCommandEvent & ) { OnChanged(); } );
			pGridRow->Add( pGridSize, wxSizerFlags().CentreVertical() );
			pSizer->Add( pGridRow, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );

			// The two columns of render toggles, in the template's order.
			wxFlexGridSizer *pToggles = new wxFlexGridSizer( 2, wxSize( 6, 4 ) );
			pToggles->AddGrowableCol( 0 );
			pToggles->AddGrowableCol( 1 );
			pBoundingBoxes = AddCheck( "Bounding boxes" );
			pShadows = AddCheck( "Draw shadows" );
			pWireFrame = AddCheck( "Wire frame" );
			pWarfog = AddCheck( "Draw warfog" );
			pMipmap = AddCheck( "Mipmap levels" );
			pTerrain = AddCheck( "Draw terrain" );
			pOverdraw = AddCheck( "Overdraw" );
			pToggles->Add( pBoundingBoxes );
			pToggles->Add( pShadows );
			pToggles->Add( pWireFrame );
			pToggles->Add( pWarfog );
			pToggles->Add( pMipmap );
			pToggles->Add( pTerrain );
			pToggles->Add( pOverdraw );
			// The template leaves a wider gap between the grid row and these.
			pSizer->AddSpacer( 12 );
			pSizer->Add( pToggles, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 8 ) );

			// Default Settings on the left, OK and Cancel on the right.
			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			wxButton *pDefault = NWx::Child<wxButton>( this, wxID_ANY, "Default Settings" );
			pDefault->Bind( wxEVT_BUTTON, [this]( wxCommandEvent & ) { OnDefault(); } );
			pButtons->Add( pDefault );
			pButtons->AddStretchSpacer();
			wxButton *pOk = NWx::Child<wxButton>( this, wxID_OK, "OK" );
			pOk->SetDefault();
			pButtons->Add( pOk, wxSizerFlags().Border( wxRIGHT, 4 ) );
			pButtons->Add( NWx::Child<wxButton>( this, wxID_CANCEL, "Cancel" ) );
			pSizer->Add( pButtons, wxSizerFlags().Expand().Border( wxALL, 8 ) );

			Fill();
			SetSizerAndFit( pSizer );
			Centre();
			pOk->SetFocus();
		}

		// GetDialogData: the filter from the controls, all of it. The type list
		// is rebuilt from the list's rows, as it was, rather than patched.
		void ReadFilter()
		{
			pFilter->objTypeFilter.clear();
			for ( int nItem = 0; nItem < pTypes->GetItemCount(); ++nItem )
			{
				CMapInfoEditorSettings::SViewFilterData::SObjTypeFilter typeFilter;
				typeFilter.szObjTypeName = std::string( pTypes->GetItemText( nItem ).utf8_str() );
				typeFilter.bShow = pTypes->IsItemChecked( nItem );
				pFilter->objTypeFilter.push_back( typeFilter );
			}
			pFilter->bShowGrid = pGrid->GetValue();
			pFilter->bShowBBoxes = pBoundingBoxes->GetValue();
			pFilter->bWireFrame = pWireFrame->GetValue();
			pFilter->bShowTerrain = pTerrain->GetValue();
			pFilter->bShowShadows = pShadows->GetValue();
			pFilter->bShowWarfog = pWarfog->GetValue();
			pFilter->bMipmap = pMipmap->GetValue();
			pFilter->bOverdraw = pOverdraw->GetValue();
			// GetWindowText on a drop-down list is the selected item's text.
			pFilter->szGridSize = std::string( pGridSize->GetStringSelection().utf8_str() );
		}
	};
}


namespace NMapInfoViewFilter
{
	bool RunWx( IWidget *pParent, CMapInfoEditorSettings::SViewFilterData *pFilter )
	{
		if ( pFilter == 0 )
		{
			return false;
		}
		// Taken before the dialog touches anything: Cancel puts this back.
		const CMapInfoEditorSettings::SViewFilterData original = ( *pFilter );
		CMapInfoViewFilterWxDialog dialog( pFilter );
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			( *pFilter ) = original;
			Apply();
			return false;
		}
		// OnOK's last read. The dialog is hidden, not destroyed, until it goes
		// out of scope, so its controls still answer.
		dialog.ReadFilter();
		Apply();
		return true;
	}
}

#endif // OBK2_WITH_WX
