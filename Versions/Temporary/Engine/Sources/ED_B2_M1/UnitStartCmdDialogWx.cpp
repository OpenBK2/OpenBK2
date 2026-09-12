#include "stdafx.h"

#include "UnitStartCmdDialog.h"

#ifdef OBK2_WITH_WX

#include <fmt/format.h>

#include "EditorMethods.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"
#include "StringResources.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <cstdlib>

// The unit start command editor in wx, and the first modeless window to move.
//
// Modeless changes three things and nothing else:
//
//   * **It has to live in MFC's message loop.** A wx dialog run with ShowModal
//     pumps its own; this one is shown and left there, so Tab, Escape and the
//     default button only work if something gives wx a look at each message
//     first. Something does: the app is a wxMFCApp, whose PreTranslateMessage
//     offers every message to wx before MFC translates it. See
//     B2_MapEditor/WxHost.h. Nothing needed here, but it is the reason this
//     works at all, and the first thing to suspect if a key stops responding.
//   * **The close box must not destroy it.** wx closes a dialog by destroying
//     it, and the state holds this one across uses, so the close box is bound
//     to the same thing Cancel does: tell the state, and hide.
//   * **It is owned by the frame without disabling it.** NWxModal::SetOwnerFrame
//     is that half of ShowModalOver on its own.
//
// The rest is the MFC dialog's behaviour, kept: the title says which of the two
// jobs it is doing, the type list is sorted, and the parameter box, the clear
// button and OK are enabled only when the type is one the editor knows.

namespace
{
	const char *const UNKNOWN_COMMAND = "<UNKNOWN>";


	class CUnitStartCmdWxDialog : public CWxToolDialog
	{
		wxChoice *pTypes = nullptr;
		wxTextCtrl *pTarget = nullptr;
		wxTextCtrl *pData = nullptr;
		wxButton *pClear = nullptr;
		wxButton *pOk = nullptr;

		NUnitStartCmdDialog::IListener *pListener = nullptr;
		std::vector<SUnitCommandTypeInfo> cmdTypes;
		NWxPlacement::CPlacement placement;

		bool bEditMode = true;
		int nCommandIndex = -1;
		bool bHadPlacement = false;

	public:
		explicit CUnitStartCmdWxDialog( NUnitStartCmdDialog::IListener *_pListener )
			: CWxToolDialog( nullptr, wxID_ANY, "Unit Start Command" ),
				pListener( _pListener ), placement( "CEdUnitStartCmd" )
		{
			LoadUnitCommandTypesFromXML( &cmdTypes );

			wxFlexGridSizer *pGrid = new wxFlexGridSizer( 3, 6, 6 );
			pGrid->AddGrowableCol( 1, 1 );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Type:" ),
									wxSizerFlags().CentreVertical() );
			// CBS_SORT in the template, so the list is in alphabetical order and
			// not the order the types were read in. It matters twice: the entry
			// that ends up first is not the first command type, and GetDialogData
			// indexes the command types by the row's position -- see there.
			pTypes = NWx::Child<wxChoice>( this, wxID_ANY, wxDefaultPosition,
																		 wxSize( ConvertDialogToPixels( wxSize( 155, 0 ) ).x, -1 ),
																		 0, nullptr, wxCB_SORT );
			pGrid->Add( pTypes, wxSizerFlags().Expand() );
			pGrid->AddSpacer( 0 );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Target:" ),
									wxSizerFlags().CentreVertical() );
			// ES_READONLY: the state writes it as units are picked on the map.
			pTarget = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(), wxDefaultPosition,
																				wxSize( ConvertDialogToPixels( wxSize( 117, 0 ) ).x, -1 ),
																				wxTE_READONLY );
			pGrid->Add( pTarget, wxSizerFlags().Expand() );
			pClear = NWx::Child<wxButton>( this, wxID_ANY, "clear", wxDefaultPosition,
																		 wxSize( ConvertDialogToPixels( wxSize( 34, 0 ) ).x, -1 ) );
			pGrid->Add( pClear, wxSizerFlags().CentreVertical() );

			pGrid->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Parameter:" ),
									wxSizerFlags().CentreVertical() );
			pData = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(), wxDefaultPosition,
																			wxSize( ConvertDialogToPixels( wxSize( 155, 0 ) ).x, -1 ) );
			pGrid->Add( pData, wxSizerFlags().Expand() );
			pGrid->AddSpacer( 0 );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pGrid, wxSizerFlags( 1 ).Expand().Border( wxALL, 8 ) );
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			pOk = wxDynamicCast( FindWindow( wxID_OK ), wxButton );

			Bind( wxEVT_BUTTON, &CUnitStartCmdWxDialog::OnOk, this, wxID_OK );
			Bind( wxEVT_BUTTON, &CUnitStartCmdWxDialog::OnCancel, this, wxID_CANCEL );
			pClear->Bind( wxEVT_BUTTON, &CUnitStartCmdWxDialog::OnClear, this );
			pTypes->Bind( wxEVT_CHOICE, &CUnitStartCmdWxDialog::OnTypeChanged, this );
			// Escape reaches this as a wxID_CANCEL button event, which is handled
			// above; the close box arrives as this instead, and would destroy the
			// window that the state still holds.
			Bind( wxEVT_CLOSE_WINDOW, &CUnitStartCmdWxDialog::OnCloseBox, this );

			// CEdUnitStartCmd::OnInitDialog fills itself in with a default SDlgData
			// before anyone asks, which is what gives the hidden window its title
			// and the enabled states it opens with.
			const NUnitStartCmdDialog::SData empty;
			SetDialogData( &empty );

			// Where CResizeDialog::OnInitDialog left it, through the file the MFC
			// dialog uses. It is a create-time restore there too, not a per-show
			// one, so a dialog moved while it is up stays where it was moved for
			// the rest of the session.
			bHadPlacement = placement.Restore( this );
		}

		// Whether the file had a position in it, so the caller knows whether to
		// centre this instead.
		bool HasSavedPlacement() const
		{
			return bHadPlacement;
		}

		// There is deliberately no Save. CResizeDialog writes the placement from
		// its OnOK and OnCancel, and CEdUnitStartCmd overrides both without
		// calling the base -- so this dialog restores from a file that nothing
		// ever writes, and it stays where the template put it for as long as
		// anyone has been running the editor. Measured, not assumed: closing the
		// editor cleanly after moving the window leaves no
		// ResizeDialogStyles\CEdUnitStartCmd.xml behind, in either toolkit.

		void SetDialogData( const NUnitStartCmdDialog::SData *pData_ )
		{
			if ( pData_ == 0 )
			{
				return;
			}
			bEditMode = pData_->bEditMode;
			nCommandIndex = pData_->nCommandIndex;
			SetTitle( wxString::FromUTF8( bEditMode ? RCSTR( "New command" ) : RCSTR( "Edit command" ) ) );

			pTypes->Clear();
			wxString selected = wxString::FromUTF8( UNKNOWN_COMMAND );
			bool bFound = false;
			for ( int i = 0; i < static_cast<int>( cmdTypes.size() ); ++i )
			{
				pTypes->Append( wxString::FromUTF8( cmdTypes[i].szName.c_str() ),
												reinterpret_cast<void*>( static_cast<intptr_t>( cmdTypes[i].nValue ) ) );
				if ( cmdTypes[i].nValue == pData_->nSelectedCmdType )
				{
					selected = wxString::FromUTF8( cmdTypes[i].szName.c_str() );
					bFound = true;
				}
			}
			// The one entry that is not a command type, and the only one whose
			// data is -1.
			pTypes->Append( wxString::FromUTF8( UNKNOWN_COMMAND ),
											reinterpret_cast<void*>( static_cast<intptr_t>( -1 ) ) );
			pTypes->SetStringSelection( selected );

			pData->Enable( bFound );
			pClear->Enable( bFound );
			if ( pOk != nullptr )
			{
				pOk->Enable( bFound );
			}

			pTarget->ChangeValue( wxString::FromUTF8( pData_->szTarget.c_str() ) );
			if ( pData_->szTarget.empty() )
			{
				// After the line above, as the MFC dialog does it: no target means
				// nothing to clear, whatever the type says.
				pClear->Enable( false );
			}
			pData->ChangeValue( wxString::FromUTF8( fmt::format( "{}", pData_->nData ).c_str() ) );
		}

		void GetDialogData( NUnitStartCmdDialog::SData *pData_ )
		{
			if ( pData_ == 0 )
			{
				return;
			}
			pData_->Clear();

			const int nSelected = pTypes->GetSelection();
			pData_->nSelectedCmdType = ( nSelected != wxNOT_FOUND ) ? SelectedValue() : -1;

			// Indexed by the row's position, in a sorted list, against a vector in
			// the order the types were read. The MFC dialog does exactly this and
			// so does this one: the two agree, because the sort is the same native
			// one. It is what it is -- the answer is the target flag of whichever
			// type happens to sit at that position -- and it belongs to the
			// dialogs, not to this port.
			if ( nSelected >= 0 && nSelected < static_cast<int>( cmdTypes.size() ) )
			{
				pData_->bSelectedCmdNeedTargetUnit = cmdTypes[nSelected].nNeedTargetUnit != 0;
			}
			else
			{
				pData_->bSelectedCmdNeedTargetUnit = false;
			}

			pData_->nData = atoi( std::string( pData->GetValue().utf8_str() ).c_str() );
			pData_->szTarget = std::string( pTarget->GetValue().utf8_str() );
			pData_->bEditMode = bEditMode;
			pData_->nCommandIndex = nCommandIndex;
		}

		void UpdateTarget( const std::string &rszTarget )
		{
			pTarget->ChangeValue( wxString::FromUTF8( rszTarget.c_str() ) );
		}

		int GetSelectedCommandType()
		{
			if ( pTypes->GetSelection() == wxNOT_FOUND )
			{
				return 0;
			}
			return SelectedValue();
		}

	private:
		int SelectedValue() const
		{
			return static_cast<int>( reinterpret_cast<intptr_t>(
				pTypes->GetClientData( pTypes->GetSelection() ) ) );
		}

		void Report( NUnitStartCmdDialog::EEvent eEvent )
		{
			if ( pListener != nullptr )
			{
				pListener->OnUnitStartCmdDialogEvent( eEvent );
			}
		}

		void OnOk( wxCommandEvent & )
		{
			Report( NUnitStartCmdDialog::EV_OK );
			Show( false );
		}

		void OnCancel( wxCommandEvent & )
		{
			Report( NUnitStartCmdDialog::EV_CANCEL );
			Show( false );
		}

		void OnClear( wxCommandEvent & )
		{
			Report( NUnitStartCmdDialog::EV_CLEAR );
		}

		void OnTypeChanged( wxCommandEvent & )
		{
			Report( NUnitStartCmdDialog::EV_TYPE_CHANGE );
			if ( pTypes->GetSelection() == wxNOT_FOUND )
			{
				return;
			}
			const bool bKnown = ( SelectedValue() != -1 );
			pData->Enable( bKnown );
			pClear->Enable( bKnown );
			if ( pOk != nullptr )
			{
				pOk->Enable( bKnown );
			}
		}

		void OnCloseBox( wxCloseEvent & )
		{
			// Hidden, never destroyed: the state owns this window and will show it
			// again. Its close box does what Cancel does, which is what the MFC
			// dialog's does through OnCancel.
			Report( NUnitStartCmdDialog::EV_CANCEL );
			Show( false );
		}
	};


	// The interface in front of it. Separate from the window because wx destroys
	// a top-level window itself when it is closed, so the handle the state holds
	// must not be the wxDialog: see WxOwnership.h.
	class CWxUnitStartCmdDialog : public NUnitStartCmdDialog::IDialog
	{
		wxWeakRef<CUnitStartCmdWxDialog> dialog;

	public:
		CWxUnitStartCmdDialog( IWidget *pParent, NUnitStartCmdDialog::IListener *pListener )
			: dialog( NWx::TopLevel<CUnitStartCmdWxDialog>( pListener ) )
		{
			// Above the frame and out of the taskbar, without disabling anything:
			// this window is modeless.
			NWxModal::SetOwnerFrame( dialog, pParent );
			// A deliberate difference, and the only one: with nothing saved yet
			// this opens on the frame's centre, where every other migrated dialog
			// opens. MFC's lands at the template's own position, which for this
			// template is the top left corner of the screen -- modal dialogs are
			// centred on their owner by _AfxPostInitDialog and a modeless one is
			// not. Once the file exists both read it and agree.
			if ( dialog && !dialog->HasSavedPlacement() )
			{
				NWxModal::CentreOver( dialog, pParent );
			}
		}

		~CWxUnitStartCmdDialog()
		{
			if ( dialog )
			{
				dialog->Destroy();
			}
		}

		bool IsAlive() const
		{
			return dialog != nullptr;
		}

		virtual void Show( bool bShow )
		{
			if ( dialog )
			{
				dialog->Show( bShow );
			}
		}

		virtual void SetDialogData( const NUnitStartCmdDialog::SData *pData )
		{
			if ( dialog )
			{
				dialog->SetDialogData( pData );
			}
		}

		virtual void GetDialogData( NUnitStartCmdDialog::SData *pData )
		{
			if ( dialog )
			{
				dialog->GetDialogData( pData );
			}
		}

		virtual void UpdateTarget( const std::string &rszTarget )
		{
			if ( dialog )
			{
				dialog->UpdateTarget( rszTarget );
			}
		}

		virtual int GetSelectedCommandType()
		{
			return dialog ? dialog->GetSelectedCommandType() : 0;
		}
	};
}


namespace NUnitStartCmdDialog
{
	IDialog* CreateWx( IWidget *pParent, IListener *pListener )
	{
		CWxUnitStartCmdDialog *pDialog = new CWxUnitStartCmdDialog( pParent, pListener );
		if ( !pDialog->IsAlive() )
		{
			delete pDialog;
			return 0;
		}
		// Owned by the frame, and not shown: the state shows it when a command is
		// added or edited.
		pDialog->Show( false );
		return pDialog;
	}
}

#endif // OBK2_WITH_WX
