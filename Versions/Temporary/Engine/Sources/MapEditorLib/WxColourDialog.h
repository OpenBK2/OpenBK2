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
//   * **It is modal over the MFC frame, and owned by it.** wx disables only
//     wx windows while a dialog is up, so the frame would stay live under it;
//     NWxModal::ShowModalOver disables the frame, as CDialog::DoModal did.
//     And wxColourDialog's owner is whatever wx picks as its parent, which is
//     nothing at all when the palette is not on screen -- measured: the
//     dialog came up ownerless, free to fall behind the frame. So the frame is
//     made its owner from MSWOnInitDone, the hook wx calls on WM_INITDIALOG,
//     before the dialog is shown. That is what CColorDialog's AfxGetMainWnd()
//     argument did.
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

#ifdef OBK2_WITH_WX

#include "Interface_UserData.h"
#include "WxModal.h"

#include <wx/colordlg.h>

#include <cstdint>

namespace NWxColourDialog
{
	// The marker colorList is padded with for a slot nobody has filled.
	const COLORREF UNSET_CUSTOM_COLOUR = 0xFFffFFff;

	namespace NDetail
	{
		// wxColourDialog, owned by a window wx knows nothing about. The owner
		// is set on WM_INITDIALOG, which is the first moment the dialog has a
		// window and comes before it is shown -- the same "set it before it is
		// visible" rule NWxModal::ShowModalOver follows for ordinary dialogs.
		class COwnedColourDialog : public wxColourDialog
		{
			HWND hwndOwner;

		public:
			COwnedColourDialog( wxWindow *pParent, wxColourData *pData, HWND _hwndOwner )
				: wxColourDialog( pParent, pData ), hwndOwner( _hwndOwner ) {}

			virtual void MSWOnInitDone( WXHWND hDlg ) override
			{
				if ( hwndOwner != 0 )
				{
					::SetWindowLongPtr( (HWND)hDlg, GWLP_HWNDPARENT, (LONG_PTR)hwndOwner );
				}
				wxColourDialog::MSWOnInitDone( hDlg );
			}
		};
	}

	// Opens the picker full, starting on rStart, over the frame pOwner belongs
	// to, with the user's custom colours. True and the colour in *pResult on
	// OK; false, and *pResult untouched, on Cancel or with no user data to
	// keep the custom colours in.
	//
	// pParent is the wx window the dialog belongs to; pOwner is the same
	// palette as the MFC frame sees it, which is what the frame is found from.
	inline bool Pick( wxWindow *pParent, IWidget *pOwner, const wxColour &rStart, wxColour *pResult )
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

		NDetail::COwnedColourDialog dialog( pParent, &colourData, NWxModal::FindOwnerFrame( pOwner ) );
		if ( NWxModal::ShowModalOver( &dialog, pOwner ) != wxID_OK )
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

#endif // OBK2_WITH_WX
