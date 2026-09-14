#include "stdafx.h"

#include "BuildDataView.h"

#ifdef OBK2_WITH_WX

#include "NewObjectView.h"
#include "PC_BaseDialog.h"
#include "PropertyPaneView.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxMfcOwnerDialog.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <memory>

// The build data dialog in wx: IDD_PC_BD's controls around the wx property
// grid the Selection Properties pane uses, NPropertyPane::CreateGridWx.
//
// Three things it has to do that the MFC dialog got from being MFC:
//
//   * **Be an owner.** A button in the grid can open an MFC dialog -- the
//     database link picker -- and that wants a CWnd to belong to. Given the
//     frame, CDialog::DoModal would enable the frame again when it closed,
//     under this dialog. So the dialog's own handle is attached to a CWnd:
//     attaching only puts it in MFC's handle map, it does not subclass, and
//     MFC's owner search stops at a window that is not a child. That is
//     CWxMfcOwnerDialog, which also keeps the frame from disabling the dialog
//     in turn.
//   * **Hear about changes.** CPCMainTreeControl sent WM_PC_MANIPULATOR_CHANGE
//     to its dialog after every undo and redo, and the dialog checked OK again.
//     The grid calls back instead. A builder's IsValidBuildData may itself write
//     a field, which comes back here as another change; the second check is
//     skipped while the first runs.
//   * **Keep its columns in its own file.** The widths are parameters 0 to 2 of
//     the file that holds the placement, so they are put into the placement's
//     state before it is saved, or the save would put back the ones read when
//     the dialog opened.

namespace
{
	// CPCBuildDataDialog::GetXMLFilePath's label, shared with the MFC dialog.
	const char *const PSZ_STATE_NAME = "CPCBuildDataDialog";
	const int N_COLUMN_COUNT = 3;


	class CBuildDataWxDialog : public CWxMfcOwnerDialog, public CPCBaseDialog
	{
		NWxPlacement::CSizedPlacement placement { PSZ_STATE_NAME };
		// A member, so it goes before the base's owner: the grid's window is taken
		// down while the dialog and its handle still stand.
		std::unique_ptr<NPropertyPane::IGrid> pGrid;

		wxTextCtrl *pName = nullptr;
		wxStaticText *pStatus = nullptr;
		wxCheckBox *pExport = nullptr;

		SBuildDataParams *pBuildDataParams = nullptr;
		IBuildDataCallback *pBuildDataCallback = nullptr;
		ICommandHandler *pPreviousCommandHandler = nullptr;
		bool bRegistered = false;
		bool bCheckingOk = false;

	public:
		CBuildDataWxDialog( SBuildDataParams *_pBuildDataParams, IBuildDataCallback *_pBuildDataCallback )
			: CWxMfcOwnerDialog( nullptr, wxID_ANY, "Create Game Data Base Object",
											 wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				pBuildDataParams( _pBuildDataParams ), pBuildDataCallback( _pBuildDataCallback )
		{
			// IDD_PC_BD, top to bottom: the name, "Properties:" over the grid, the
			// status line, then the export check with OK and Cancel on its line.
			wxBoxSizer *const pNameRow = new wxBoxSizer( wxHORIZONTAL );
			pNameRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Name:" ),
										 wxSizerFlags().CentreVertical().Border( wxRIGHT, FromDIP( 6 ) ) );
			pName = NWx::Child<wxTextCtrl>( this, wxID_ANY );
			pNameRow->Add( pName, wxSizerFlags( 1 ).CentreVertical() );

			pGrid.reset( NPropertyPane::CreateGridWx( this, nullptr, GetMfcOwner(), PSZ_STATE_NAME ) );
			// The tree's status window was never connected; the line under it
			// shows what is wrong with the fields.
			pStatus = NWx::Child<wxStaticText>( this, wxID_ANY, wxString(), wxDefaultPosition,
																					wxSize( -1, FromDIP( 18 ) ),
																					wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END | wxBORDER_SUNKEN );

			wxBoxSizer *const pBottomRow = new wxBoxSizer( wxHORIZONTAL );
			pExport = NWx::Child<wxCheckBox>( this, wxID_ANY, "Export object after creation" );
			pBottomRow->Add( pExport, wxSizerFlags( 1 ).CentreVertical() );
			pBottomRow->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ), wxSizerFlags().CentreVertical() );
			// IDD_PC_BD's buttons are 50 by 14 dialog units, larger than wx's
			// standard ones.
			const wxSize buttonSize = ConvertDialogToPixels( wxSize( 50, 14 ) );
			for ( wxWindow *pButton : { FindWindow( wxID_OK ), FindWindow( wxID_CANCEL ) } )
			{
				if ( pButton != nullptr )
				{
					pButton->SetMinSize( buttonSize );
				}
			}

			const int nBorder = FromDIP( 8 );
			wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pNameRow, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, nBorder ) );
			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Properties:" ),
									 wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, nBorder ) );
			// The grid takes whatever the dialog grows by, as IDC_PC_BD_TREE did.
			pSizer->Add( pGrid->GetWindow(), wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT | wxTOP, FromDIP( 4 ) ) );
			pSizer->Add( pStatus, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, FromDIP( 4 ) ) );
			pSizer->Add( pBottomRow, wxSizerFlags().Expand().Border( wxALL, nBorder ) );
			SetSizerAndFit( pSizer );
			// CPCBuildDataDialog's GetMinimumXDimension and GetMinimumYDimension.
			SetMinSize( wxSize( 300, 200 ) );
			// The template's size for a first open, then wherever it was left.
			SetSize( ConvertDialogToPixels( wxSize( 310, 230 ) ) + ( GetSize() - GetClientSize() ) );
			placement.Restore( this );

			// OnInitDialog's order: the name and the check, then the tree, OK and
			// the title -- the tree and OK once the grid has an object, in Open.
			pName->ChangeValue( wxString::FromUTF8( pBuildDataParams->szObjectName.c_str() ) );
			pExport->SetValue( pBuildDataParams->bNeedExport );
			// IDS_PC_BD_DIALOG_TITLE, which the new object dialog names its type with too.
			SetTitle( wxString::FromUTF8( NNewObject::Title( pBuildDataParams ).c_str() ) );

			pName->Bind( wxEVT_TEXT, &CBuildDataWxDialog::OnNameChanged, this );
			pExport->Bind( wxEVT_CHECKBOX, &CBuildDataWxDialog::OnExportChanged, this );
			pGrid->SetChangeCallback( [this]() { UpdateOkButton(); } );
		}

		virtual ~CBuildDataWxDialog()
		{
			Unregister();
		}

		bool WasPlaced() const
		{
			return placement.WasPlaced();
		}

		// The grid shows the object, and the dialog is CHID_PC_BUILD_DATA_DIALOG
		// while it does, as CPCBuildDataDialog::OnInitDialog made itself.
		void Open( IManipulator *pManipulator, const SObjectSet &rObjectSet, const std::string &rszTemporaryLabel )
		{
			pGrid->GetView()->SetViewManipulator( pManipulator, rObjectSet, rszTemporaryLabel );
			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pPreviousCommandHandler = pContainer->Get( CHID_PC_BUILD_DATA_DIALOG );
			pContainer->Set( CHID_PC_BUILD_DATA_DIALOG, this );
			bRegistered = true;
			pGrid->BuildTree();
			UpdateOkButton();
		}

		// OnDestroy's order: the widths and the placement, the previous handler
		// back, then the caller takes the object away.
		void Close()
		{
			int nWidths[N_COLUMN_COUNT] = { 0, 0, 0 };
			if ( pGrid->GetColumnWidths( nWidths ) )
			{
				for ( int nColumn = 0; nColumn < N_COLUMN_COUNT; ++nColumn )
				{
					placement.State().SetIntParameter( nColumn, nWidths[nColumn] );
				}
			}
			placement.Save( this );
			Unregister();
			pGrid->GetView()->RemoveViewManipulator();
		}

		// CPCBaseDialog
		virtual IView* GetView()
		{
			return pGrid->GetView();
		}

		virtual ICommandHandler* GetCommandHandler()
		{
			return pGrid->GetCommandHandler();
		}

		virtual void CreateTree()
		{
			pGrid->BuildTree();
		}

		virtual void UpdateValues()
		{
			pGrid->UpdateValues();
		}

	private:
		void Unregister()
		{
			if ( bRegistered )
			{
				Singleton<ICommandHandlerContainer>()->Set( CHID_PC_BUILD_DATA_DIALOG, pPreviousCommandHandler );
				bRegistered = false;
			}
		}

		void UpdateOkButton()
		{
			if ( bCheckingOk )
			{
				return;
			}
			bCheckingOk = true;
			std::string szErrorMessage;
			const bool bAccept = NBuildData::CanAccept( pBuildDataParams, pBuildDataCallback, pGrid->GetView(), &szErrorMessage );
			pStatus->SetLabelText( wxString::FromUTF8( szErrorMessage.c_str() ) );
			if ( wxWindow *const pOk = FindWindow( wxID_OK ) )
			{
				pOk->Enable( bAccept );
			}
			bCheckingOk = false;
		}

		void OnNameChanged( wxCommandEvent & )
		{
			pBuildDataParams->szObjectName = std::string( pName->GetValue().utf8_str() );
			UpdateOkButton();
		}

		void OnExportChanged( wxCommandEvent & )
		{
			pBuildDataParams->bNeedExport = pExport->GetValue();
		}
	};
}


namespace NBuildData
{
	bool RunWx( IWidget *pParent, IManipulator *pManipulator, const SObjectSet &rObjectSet,
							const std::string &rszTemporaryLabel, SBuildDataParams *pBuildDataParams,
							IBuildDataCallback *pBuildDataCallback )
	{
		if ( pBuildDataParams == 0 )
		{
			return false;
		}
		CBuildDataWxDialog dialog( pBuildDataParams, pBuildDataCallback );
		if ( !dialog.WasPlaced() )
		{
			NWxModal::CentreOver( &dialog, pParent );
		}
		dialog.Open( pManipulator, rObjectSet, rszTemporaryLabel );
		const bool bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );
		dialog.Close();
		return bAccepted;
	}
}

#endif // OBK2_WITH_WX
