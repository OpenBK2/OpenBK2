#pragma once

#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Interface_PCItemEditor.h"
#include "MapEditorLib/Interface_Widget.h"

#include <cstdint>
#include <string>
#include <vector>

// The buttons beside a property's value -- "...", "New", "Edit" -- behind a
// boundary that names no toolkit.
//
// In the MFC tree they belong to the editor opened over a row, one of the
// eleven classes under CPCStringMultibuttonEditor, and each of those had its
// own OnBrowse, OnNew or OnEdit. None of that is drawing: a button opens a
// dialog -- the database link picker, a file or folder picker, the colour
// picker, the bit field or text editor -- starting from the text in the value
// box, and the answer is the text the box holds afterwards. So that is what
// lives here, once, and the wx property grid calls it.
//
// Which dialog a picker is: the colour picker and the database link picker
// are wx's. The file and folder pickers are still MFC's CFileDialog and the
// shell's folder browser.
namespace NPropertyButton
{
	enum EButton
	{
		BUTTON_BROWSE,
		BUTTON_NEW,
		BUTTON_EDIT,
	};

	// What a button is pressed on.
	struct SContext
	{
		// The property's full name, "Weather.WindForce" -- the name the tree gave
		// its editor, and what New names a new object or file after.
		std::string szName;
		EPCIEType nType = PCIE_UNKNOWN;
		const SPropertyDesc *pDesc = nullptr;
		// The objects the pane shows. New puts what it makes beside the first.
		const SObjectSet *pObjectSet = nullptr;
		// The window the dialogs belong to. Borrowed for the call.
		IWidget *pOwner = nullptr;
		// False keeps a dialog's answer out of the box, as an ES_READONLY edit
		// box did, and opens the text editors read-only.
		bool bEditable = true;
	};

	// Where the folder picker is remembered in SUserData::filePathMap.
	const char *const PSZ_FOLDER_PATH_LABEL = "_FOLDER_";

	// The buttons a type's editor has, in order: "..." for all of them, then
	// "New" for the new references and the new text file, and "Edit" for both
	// text files. Empty for a type with no buttons.
	void GetButtons( EPCIEType nType, std::vector<EButton> *pButtons );

	// The caption, from the editor's string table.
	std::string GetTitle( EButton eButton );

	// Runs a button. rszText is the text in the value box now. True with the
	// box's new text in *pszNewText; false when the box is to stay as it is --
	// a cancelled dialog, a read-only context, or a button whose work is done
	// elsewhere, like a text file edited and saved to disk.
	bool Press( EButton eButton, const SContext &rContext, const std::string &rszText, std::string *pszNewText );

	// The colour picker, opened full on nStart over the frame pOwner belongs
	// to, with the user's custom colours. Colours are COLORREF's 0x00BBGGRR.
	// True and the colour in *pnResult on OK.
	bool PickColour( IWidget *pOwner, uint32_t nStart, uint32_t *pnResult );

}
