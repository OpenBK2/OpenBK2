#pragma once

// Where a migrated dialog opens, when the MFC one it replaces remembers that.
//
// A dialog deriving from CResizeDialog *and calling its OnInitDialog* restores
// its placement from Editor/ResizeDialogStyles/<class>.xml, and writes it again
// from CResizeDialog::OnOK and OnCancel -- so it reopens where it was left, as
// long as it does not override both of those without calling the base. A
// modeless one that does (CEdUnitStartCmd) restores from a file nothing ever
// writes. A wx dialog put in front of the
// same user should do the same, through the same file -- which is what
// SDialogState and NDialogState are for.
//
// Only the position is taken. The size in the file is the MFC template's: a wx
// dialog fitted to its contents would be left with empty space, and writing a
// fitted size back would shrink the MFC dialog, whose controls do not move
// because nothing in these templates is anchored. So the size found there is
// left as it was, and a dialog that has never been opened is centred over the
// frame instead (NWxModal::CentreOver).
//
// A resizable dialog is a different case: it restores the size as well, which
// is CSizedPlacement below.
//
// Check which shape an MFC dialog has before porting it, and check for the
// macro as well as the base class:
//
//   * plain CDialog: no placement at all;
//   * CResizeDialog with its OnInitDialog called, *and*
//     DECLARE_RESIZE_DLG_WND_COMMON_METHODS declared: placement, under the
//     class name the macro was given;
//   * CResizeDialog with its OnInitDialog called and no macro: the shell font
//     but no placement, because GetXMLFilePath answers with an empty label and
//     NDialogState refuses one -- the reinforcement point dialogs are these;
//   * CResizeDialog whose message map chains to CDialog: neither half runs --
//     CEnterNameDialog is the last of those.


#include "DialogState.h"

#include <wx/dialog.h>

namespace NWxPlacement
{
	class CPlacement
	{
		const char *pszName;
		SDialogState state;

	public:
		// pszStateName is the MFC dialog's class name, which is what
		// CResizeDialog::GetXMLFilePath answers and so what the file is called.
		explicit CPlacement( const char *pszStateName ) : pszName( pszStateName )
		{
			NDialogState::Load( pszName, &state );
		}

		// Moves the dialog to where it was left; false if nothing was saved yet,
		// in which case the caller centres it.
		bool Restore( wxDialog *pDialog ) const
		{
			if ( pDialog == 0 || state.rect.Width() <= 0 || state.rect.Height() <= 0 )
			{
				return false;
			}
			pDialog->Move( state.rect.left, state.rect.top );
			return true;
		}

		// On the way out, whichever button was used, which is where
		// CResizeDialog writes it too: its OnOK and its OnCancel, both. The size
		// already in the file is kept; see the note above.
		void Save( const wxDialog *pDialog )
		{
			if ( pDialog == 0 )
			{
				return;
			}
			const wxPoint at = pDialog->GetPosition();
			const int nWidth = state.rect.Width() > 0 ? state.rect.Width() : pDialog->GetSize().x;
			const int nHeight = state.rect.Height() > 0 ? state.rect.Height() : pDialog->GetSize().y;
			state.rect = CTRect<int>( at.x, at.y, at.x + nWidth, at.y + nHeight );
			NDialogState::Save( pszName, &state );
		}
	};


	// The resizable case: size and position both, restored and saved as
	// CResizeDialog does for a THICKFRAME template. Same file, same format.
	class CSizedPlacement
	{
		std::string szName;
		SDialogState state;
		bool bPlaced = false;

	public:
		explicit CSizedPlacement( const std::string &rszStateName ) : szName( rszStateName )
		{
			NDialogState::Load( szName, &state );
		}

		// Sizes and moves the dialog to where it was left; false if nothing was
		// saved yet, in which case the caller centres it.
		bool Restore( wxDialog *pDialog )
		{
			if ( pDialog == 0 || state.rect.Width() <= 0 || state.rect.Height() <= 0 )
			{
				return false;
			}
			pDialog->SetSize( state.rect.left, state.rect.top, state.rect.Width(), state.rect.Height() );
			bPlaced = true;
			return true;
		}

		bool WasPlaced() const
		{
			return bPlaced;
		}

		// The rest of the file, for a dialog that keeps parameters of its own in
		// it -- New Object's checkbox, Open MOD's last choice. Loaded when this
		// is constructed, and written with the placement by Save.
		SDialogState& State()
		{
			return state;
		}

		const SDialogState& State() const
		{
			return state;
		}

		// On the way out, whichever button was used. left + width rather than
		// GetRight(), which is the last pixel inside where MFC's right is one past.
		void Save( const wxDialog *pDialog )
		{
			if ( pDialog == 0 )
			{
				return;
			}
			const wxRect placement = pDialog->GetRect();
			state.rect = CTRect<int>( placement.GetLeft(), placement.GetTop(),
																placement.GetLeft() + placement.GetWidth(),
																placement.GetTop() + placement.GetHeight() );
			NDialogState::Save( szName, &state );
		}
	};
}

