#include "stdafx.h"

#include "OpenModView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

// Open MOD, in wx. The second modal dialog, and the first with behaviour in it
// rather than just a control: choosing a name updates the description and
// decides whether OK is available at all.
//
// Two things the MFC version has to do that this one does not, and they are
// worth pointing at because they are the shape of what the migration buys.
//
// **The names and descriptions never go through the ANSI code page.** SMOD
// keeps wszName and wszDesc as std::wstring, and the MFC dialog converts them
// with Unicode2MBSC( ..., ::GetACP() ) to put them in a narrow control. That is
// lossless today only because this process sets its code page to UTF-8 in the
// manifest. wxString is wide already, so the text goes straight in.
//
// **No line-ending surgery.** The MFC version strips every 0x0D from the
// description and then reinserts one before every 0x0A, because a Win32 EDIT
// needs CRLF to break a line. wxTextCtrl does not, so that loop is gone.

namespace
{
	// The same label COpenMODDialog::GetXMLFilePath answers with, so both
	// implementations read and write Editor/ResizeDialogStyles/COpenMODDialog.xml
	// and a user switching between them keeps their remembered choice and size.
	const char *const PSZ_STATE_NAME = "COpenMODDialog";

	// CWxToolDialog rather than wxDialog: IDD_OPEN_MOD is EXSTYLE
	// WS_EX_TOOLWINDOW like almost every template in this editor, and the first
	// pass at this dialog dropped it. See WxToolDialog.h.
	class COpenModWxDialog : public CWxToolDialog
	{
		const std::vector<NMOD::SMOD> &rModList;
		// Size, position and the chosen MOD, in COpenMODDialog's file. A dialog
		// opening for the first time has none, and is centred over the frame.
		NWxPlacement::CSizedPlacement placement { PSZ_STATE_NAME };
		// The path of the MOD already attached, if any. Choosing it again is what
		// the MFC version disables OK for, and that rule is kept.
		NFile::CFilePath szAttachedPath;
		bool bHasAttached = false;

		wxChoice *pNames = nullptr;
		wxTextCtrl *pDescription = nullptr;

	public:
		COpenModWxDialog( wxWindow *pParent, const std::vector<NMOD::SMOD> &_rModList )
			: CWxToolDialog( pParent, wxID_ANY, "Open MOD",
											 wxDefaultPosition, wxSize( 380, 260 ),
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				rModList( _rModList )
		{
			NMOD::SMOD attachedMOD;
			bHasAttached = NMOD::GetAttachedMOD( &attachedMOD );
			if ( bHasAttached )
			{
				szAttachedPath = attachedMOD.szFullFolderPath;
			}

			// The anchors from COpenMODDialog's constructor, translated:
			//   IDC_OM_NAME_LABEL   ANCHORE_LEFT_TOP                -> fixed row
			//   IDC_OM_NAME_COMBO   ANCHORE_LEFT_TOP | RESIZE_HOR   -> Expand, no proportion
			//   IDC_OM_DESC_LABEL   ANCHORE_LEFT_TOP | RESIZE_HOR   -> Expand, no proportion
			//   IDC_OM_DESC_EDIT    ANCHORE_LEFT_TOP | RESIZE_HOR_VER -> proportion 1, Expand
			//   IDOK, IDCANCEL      ANCHORE_RIGHT_BOTTOM            -> bottom row, right
			//
			// RESIZE_HOR is Expand without proportion; RESIZE_HOR_VER is Expand
			// with proportion 1. That is the whole of the anchor-to-sizer mapping
			// and it has now held for two dialogs.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Name:" ),
									 wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );
			// wxCB_SORT because the MFC template says CBS_SORT: the names are shown
			// in alphabetical order there, and a migrated dialog that listed them
			// in a different order would be a visible difference for no reason.
			//
			// Sorting means the displayed position is not the modList index, so
			// the index travels as client data exactly the way MFC carries it in
			// GetItemData. The stored parameter is the modList index in both.
			pNames = NWx::Child<wxChoice>( this, wxID_ANY, wxDefaultPosition,
																		 wxDefaultSize, 0, nullptr, wxCB_SORT );
			pSizer->Add( pNames, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 8 ) );

			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Description:" ),
									 wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );
			pDescription = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(),
																						 wxDefaultPosition, wxDefaultSize,
																						 wxTE_MULTILINE | wxTE_READONLY );
			pSizer->Add( pDescription, wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT, 8 ) );

			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Right().Border( wxALL, 8 ) );
			SetSizer( pSizer );
			SetMinSize( wxSize( 300, 150 ) );		// GetMinimumXDimension/YDimension

			for ( size_t nIndex = 0; nIndex < rModList.size(); ++nIndex )
			{
				// Straight in: wszName is already wide and so is wxString.
				// The client data is the index into rModList, which is what the
				// stored parameter means and what GetMod needs back.
				pNames->Append( wxString( rModList[nIndex].wszName ),
												reinterpret_cast<void*>( static_cast<uintptr_t>( nIndex ) ) );
			}

			pNames->Bind( wxEVT_CHOICE, &COpenModWxDialog::OnNameChosen, this );

			// What the dialog remembered last time: the chosen MOD in parameter 0,
			// and its own size and position. Both come from the file CResizeDialog
			// wrote, which is the point of sharing the format.
			const int nRemembered = placement.State().GetIntParameter( 0, -1 );
			if ( nRemembered >= 0 && nRemembered < (int)rModList.size() )
			{
				// Find the sorted position carrying that modList index, which is
				// the loop SetComboBoxEditParameters does over GetItemData.
				for ( unsigned nPos = 0; nPos < pNames->GetCount(); ++nPos )
				{
					if ( (int)reinterpret_cast<uintptr_t>( pNames->GetClientData( nPos ) ) == nRemembered )
					{
						pNames->SetSelection( nPos );
						break;
					}
				}
			}
			placement.Restore( this );
			UpdateControls();
		}

		// Whether it opened where it was left last time.
		bool WasPlaced() const
		{
			return placement.WasPlaced();
		}


		// Called on the way out, whichever button was used: where the dialog
		// ended up is worth remembering even when the answer was Cancel, which
		// is how CResizeDialog behaved too.
		void SaveState()
		{
			placement.State().SetIntParameter( 0, GetSelectedIndex() );
			placement.Save( this );
		}

		// The modList index, not the position on screen. They differ because the
		// list is sorted.
		int GetSelectedIndex() const
		{
			const int nSelection = pNames->GetSelection();
			if ( nSelection == wxNOT_FOUND )
			{
				return -1;
			}
			return (int)reinterpret_cast<uintptr_t>( pNames->GetClientData( nSelection ) );
		}

		bool GetMod( NMOD::SMOD *pMod ) const
		{
			const int nIndex = GetSelectedIndex();
			if ( pMod == 0 || nIndex < 0 || nIndex >= (int)rModList.size() )
			{
				return false;
			}
			( *pMod ) = rModList[nIndex];
			return true;
		}

	private:
		void OnNameChosen( wxCommandEvent& )
		{
			UpdateControls();
		}

		void UpdateControls()
		{
			NMOD::SMOD mod;
			if ( !GetMod( &mod ) )
			{
				pDescription->Clear();
				EnableOk( false );
				return;
			}
			pDescription->SetValue( wxString( mod.wszDesc ) );

			// Reopening the MOD that is already attached does nothing, so the MFC
			// version greys OK for it. Same comparison, same flags.
			bool bEnable = true;
			if ( bHasAttached )
			{
				bEnable = ( CStringManager::Compare( szAttachedPath, mod.szFullFolderPath,
																						 true, true, false ) != 0 );
			}
			EnableOk( bEnable );
		}

		void EnableOk( bool bEnable )
		{
			if ( wxWindow *pOk = FindWindow( wxID_OK ) )
			{
				pOk->Enable( bEnable );
			}
		}
	};
}


namespace NOpenMod
{
	bool RunWx( IWidget *pParent, NMOD::SMOD *pMod )
	{
		if ( pMod == 0 )
		{
			return false;
		}
		std::vector<NMOD::SMOD> modList;
		NMOD::GetAllMODs( &modList );

		COpenModWxDialog dialog( nullptr, modList );
		if ( !dialog.WasPlaced() )
		{
			NWxModal::CentreOver( &dialog, pParent );
		}
		const bool bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );
		dialog.SaveState();
		return bAccepted && dialog.GetMod( pMod );
	}
}

#endif // OBK2_WITH_WX
