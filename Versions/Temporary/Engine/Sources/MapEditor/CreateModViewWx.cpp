#include "stdafx.h"

#include "CreateModView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/DialogState.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

// Create MOD, in wx. Nothing new in it, which is the point: the fifth dialog
// and the first assembled entirely out of pieces that were already here --
// CWxToolDialog for the frame, NWxModal for the modality, SDialogState for the
// remembered size and position, and the anchor-to-sizer mapping for the layout.
//
// The two rules about what makes a creatable MOD are not duplicated here.
// NCreateMod::IsFolderNameFree and MakeFolderPath are shared with the MFC
// dialog, which now calls them too.
//
// The name and description never go through the ANSI code page, the same way
// Open MOD's do not. They are written to disk as UTF-16, and SNewMod carries
// them wide, so what the user typed goes to the file unconverted.

namespace
{
	// The label CCreateMODDialog::GetXMLFilePath answers with, so both
	// implementations share Editor/ResizeDialogStyles/CCreateMODDialog.xml.
	const char *const PSZ_STATE_NAME = "CCreateMODDialog";

	class CCreateModWxDialog : public CWxToolDialog
	{
		SDialogState dialogState;

		wxTextCtrl *pFolder = nullptr;
		wxTextCtrl *pName = nullptr;
		wxTextCtrl *pDescription = nullptr;

	public:
		explicit CCreateModWxDialog( wxWindow *pParent )
			: CWxToolDialog( pParent, wxID_ANY, "Create MOD",
											 wxDefaultPosition, wxSize( 430, 240 ),
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
		{
			// The anchors from CCreateMODDialog's constructor, translated:
			//   IDC_CM_FOLDER_LABEL ANCHORE_LEFT_TOP                  -> fixed
			//   IDC_CM_FOLDER_EDIT  ANCHORE_LEFT_TOP | RESIZE_HOR     -> Expand
			//   IDC_CM_NAME_LABEL   ANCHORE_LEFT_TOP                  -> fixed
			//   IDC_CM_NAME_EDIT    ANCHORE_LEFT_TOP | RESIZE_HOR     -> Expand
			//   IDC_CM_DESC_LABEL   ANCHORE_LEFT_TOP | RESIZE_HOR     -> Expand
			//   IDC_CM_DESC_EDIT    ANCHORE_LEFT_TOP | RESIZE_HOR_VER -> proportion 1
			//   IDOK, IDCANCEL      ANCHORE_RIGHT_BOTTOM              -> bottom right
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// A grid, because the template puts each label and its edit on one
			// row -- the label at y=9 against an edit at y=7, and again at 24
			// against 23 -- and only the edits stretch. AddGrowableCol on the
			// second column is what RESIZE_HOR means for a row like that.
			wxFlexGridSizer *pFields = new wxFlexGridSizer( 2, 2, 6, 8 );
			pFields->AddGrowableCol( 1 );

			pFields->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Folder:" ),
										wxSizerFlags().CentreVertical() );
			pFolder = NWx::Child<wxTextCtrl>( this, wxID_ANY );
			pFields->Add( pFolder, wxSizerFlags().Expand() );

			pFields->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Name:" ),
										wxSizerFlags().CentreVertical() );
			pName = NWx::Child<wxTextCtrl>( this, wxID_ANY );
			pFields->Add( pName, wxSizerFlags().Expand() );

			pSizer->Add( pFields, wxSizerFlags().Expand().Border( wxALL, 8 ) );

			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Description:" ),
									 wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 8 ) );
			pDescription = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(),
																						 wxDefaultPosition, wxDefaultSize,
																						 wxTE_MULTILINE );
			pSizer->Add( pDescription,
									 wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );

			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Right().Border( wxALL, 8 ) );
			SetSizer( pSizer );
			SetMinSize( wxSize( 300, 150 ) );		// GetMinimumXDimension/YDimension

			// Both fields decide whether OK is available, so both are watched.
			pFolder->Bind( wxEVT_TEXT, &CCreateModWxDialog::OnFieldChanged, this );
			pName->Bind( wxEVT_TEXT, &CCreateModWxDialog::OnFieldChanged, this );

			// Only the size and position: this dialog stores no parameters, and
			// the fields start empty because a MOD name is not worth remembering.
			NDialogState::Load( PSZ_STATE_NAME, &dialogState );
			if ( dialogState.rect.Width() > 0 && dialogState.rect.Height() > 0 )
			{
				SetSize( dialogState.rect.left, dialogState.rect.top,
								 dialogState.rect.Width(), dialogState.rect.Height() );
			}
			UpdateControls();
		}

		// Called on the way out whichever button was used, as CResizeDialog did:
		// where the dialog ended up is worth keeping even after Cancel.
		void SaveState()
		{
			// left+width, not GetRight(): wx's GetRight() is the last pixel inside
			// the rectangle and MFC's right is one past it, so GetRight() would
			// shrink the dialog by a pixel each time it was opened and closed.
			const wxRect placement = GetRect();
			dialogState.rect = CTRect<int>( placement.GetLeft(), placement.GetTop(),
																			placement.GetLeft() + placement.GetWidth(),
																			placement.GetTop() + placement.GetHeight() );
			NDialogState::Save( PSZ_STATE_NAME, &dialogState );
		}

		void ReadMod( NCreateMod::SNewMod *pMod ) const
		{
			pMod->szFolderPath = NCreateMod::MakeFolderPath( FolderName() );
			pMod->wszName = pName->GetValue().ToStdWstring();
			pMod->wszDesc = pDescription->GetValue().ToStdWstring();
		}

	private:
		std::string FolderName() const
		{
			return std::string( pFolder->GetValue().utf8_str() );
		}

		void OnFieldChanged( wxCommandEvent& )
		{
			UpdateControls();
		}

		void UpdateControls()
		{
			// The same two halves the MFC dialog tests, and in the same order: a
			// usable folder name, and a title to go with it.
			const bool bEnable = NCreateMod::IsFolderNameFree( FolderName() ) &&
													 !pName->GetValue().IsEmpty();
			if ( wxWindow *pOk = FindWindow( wxID_OK ) )
			{
				pOk->Enable( bEnable );
			}
		}
	};
}


namespace NCreateMod
{
	bool RunWx( IWidget *pParent, SNewMod *pMod )
	{
		if ( pMod == 0 )
		{
			return false;
		}
		CCreateModWxDialog dialog( nullptr );
		const bool bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );
		dialog.SaveState();
		if ( !bAccepted )
		{
			return false;
		}
		dialog.ReadMod( pMod );
		return !pMod->szFolderPath.empty() && !pMod->wszName.empty();
	}
}

#endif // OBK2_WITH_WX
