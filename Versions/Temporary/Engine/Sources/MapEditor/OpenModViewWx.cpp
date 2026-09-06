#include "stdafx.h"

#include "OpenModView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/choice.h>
#include <wx/dialog.h>
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
	class COpenModWxDialog : public wxDialog
	{
		const std::vector<NMOD::SMOD> &rModList;
		// The path of the MOD already attached, if any. Choosing it again is what
		// the MFC version disables OK for, and that rule is kept.
		NFile::CFilePath szAttachedPath;
		bool bHasAttached = false;

		wxChoice *pNames = nullptr;
		wxTextCtrl *pDescription = nullptr;

	public:
		COpenModWxDialog( wxWindow *pParent, const std::vector<NMOD::SMOD> &_rModList )
			: wxDialog( pParent, wxID_ANY, "Open MOD",
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
			pNames = NWx::Child<wxChoice>( this, wxID_ANY );
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
				pNames->Append( wxString( rModList[nIndex].wszName ) );
			}

			pNames->Bind( wxEVT_CHOICE, &COpenModWxDialog::OnNameChosen, this );
			UpdateControls();
		}

		int GetSelectedIndex() const
		{
			const int nSelection = pNames->GetSelection();
			return nSelection == wxNOT_FOUND ? -1 : nSelection;
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
		if ( NWxModal::ShowModalOver( &dialog, pParent ) != wxID_OK )
		{
			return false;
		}
		return dialog.GetMod( pMod );
	}
}

#endif // OBK2_WITH_WX
