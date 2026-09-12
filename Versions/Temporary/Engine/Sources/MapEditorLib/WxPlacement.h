#pragma once

// Where a migrated dialog opens, when the MFC one it replaces remembers that.
//
// A dialog deriving from CResizeDialog *and calling its OnInitDialog* restores
// its placement from Editor/ResizeDialogStyles/<class>.xml and saves it again
// on destroy, so it reopens where it was left. A wx dialog put in front of the
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
// A resizable dialog is a different case and does not use this: it restores
// the size as well, and CreateModViewWx, OpenModViewWx and SelectTablesViewWx
// each do that themselves.
//
// Check which of the three shapes an MFC dialog has before porting it: plain
// CDialog (no placement at all), CResizeDialog with its OnInitDialog called
// (placement), or CResizeDialog whose message map chains to CDialog, which
// runs neither half -- CEnterNameDialog is the last of those.

#ifdef OBK2_WITH_WX

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

		// On the way out, whichever button was used, as CResizeDialog's OnDestroy
		// does. The size already in the file is kept; see the note above.
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
}

#endif // OBK2_WITH_WX
