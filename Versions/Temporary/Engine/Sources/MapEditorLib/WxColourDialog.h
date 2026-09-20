#pragma once

// The editor's colour picker, for wx callers.
//
// Five places in the editor ask for a colour the same way: a CColorDialog
// opened full, started on the current colour, with its sixteen custom colour
// slots pointed straight at SUserData::colorList so that what a user saves
// there is saved for the next session and shared between every picker. The
// model palette is the first of them in wx and the property-browser colour
// editors and the database browser will follow, so the wx half of that is
// written once, here.
//
// Three things it does that a bare wxColourDialog does not:
//
//   * **It is modal over the frame, and owned by it.** The frame is the
//     dialog's wx parent, so wx hands the system dialog that window as its
//     owner and ShowModal disables it, as CDialog::DoModal did. The parent is
//     the frame and not the palette that asked, deliberately: wx refuses a
//     parent that is not shown on screen, and a palette in a hidden pane is
//     not one -- measured, the dialog came up ownerless and free to fall
//     behind the frame. The frame is always shown. That is also what
//     CColorDialog's AfxGetMainWnd() argument did.
//
//   * **Unset slots stay unset.** colorList is resized with 0xFFffFFff for a
//     slot nobody has filled. ChooseColor shows that as white, and wx both
//     shows and returns it as white, 0x00FFffFF -- so a naive round trip would
//     turn every empty slot into a saved white one the first time anyone
//     picked a colour. A slot that went in unset and came back white is left
//     unset.
//
//   * **The custom colours are the user's on OK.** One difference from MFC
//     remains and is stated rather than hidden: CColorDialog wrote into
//     colorList directly, so a custom colour edited and then cancelled was
//     kept anyway. wxColourDialog copies its custom colours back only on OK,
//     and does not expose them otherwise.
//
// Header-only and guarded, like the other Wx*.h here, because MapEditorLib
// does not link wx.


#include "Interface_UserData.h"
#include "WxWidget.h"

#include <wx/colordlg.h>

#include <cstdint>

namespace NWxColourDialog
{
	// The marker colorList is padded with for a slot nobody has filled.
	const COLORREF UNSET_CUSTOM_COLOUR = 0xFFffFFff;

	// Opens the picker full, starting on rStart, over the frame pOwner belongs
	// to, with the user's custom colours. True and the colour in *pResult on
	// OK; false, and *pResult untouched, on Cancel or with no user data to
	// keep the custom colours in.
	//
	// pOwner is the palette, tree or button that asked; the dialog is parented
	// on the top-level window that belongs to.
	inline bool Pick( IWidget *pOwner, const wxColour &rStart, wxColour *pResult )
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		if ( ( pUserData == 0 ) || ( pResult == 0 ) )
		{
			return false;
		}
		pUserData->colorList.resize( 16, UNSET_CUSTOM_COLOUR );

		wxColourData colourData;
		// CC_FULLOPEN and CC_RGBINIT, as every MFC caller asks. CC_ANYCOLOR is
		// ChooseColor's default behaviour on anything newer than Windows 3.1.
		colourData.SetChooseFull( true );
		colourData.SetColour( rStart );
		for ( int nSlot = 0; nSlot < 16; ++nSlot )
		{
			// An unset slot is left as wxNullColour, which wx hands ChooseColor
			// as white -- what ChooseColor made of 0xFFffFFff anyway.
			if ( pUserData->colorList[nSlot] != UNSET_CUSTOM_COLOUR )
			{
				// wxColour's unsigned long constructor reads 0x00BBGGRR, which
				// is COLORREF's layout.
				colourData.SetCustomColour( nSlot, wxColour( static_cast<unsigned long>( pUserData->colorList[nSlot] ) ) );
			}
		}

		wxColourDialog dialog( ToWxOwnerWindow( pOwner ), &colourData );
		if ( dialog.ShowModal() != wxID_OK )
		{
			return false;
		}

		const wxColourData &rChosen = dialog.GetColourData();
		for ( int nSlot = 0; nSlot < 16; ++nSlot )
		{
			const COLORREF nColour = static_cast<COLORREF>( rChosen.GetCustomColour( nSlot ).GetRGB() );
			const bool bStillEmpty = ( pUserData->colorList[nSlot] == UNSET_CUSTOM_COLOUR ) &&
															 ( nColour == RGB( 255, 255, 255 ) );
			if ( !bStillEmpty )
			{
				pUserData->colorList[nSlot] = nColour;
			}
		}
		( *pResult ) = rChosen.GetColour();
		return true;
	}
}

