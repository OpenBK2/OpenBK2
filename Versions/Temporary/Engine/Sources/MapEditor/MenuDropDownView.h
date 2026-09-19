#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <list>
#include <string>

// The drop-down list under the toolbar's Undo and Redo arrows, behind a
// boundary that names no toolkit.
//
// It is modeless and borderless and lives for the rest of the session once it
// is first shown: CControllerContainer fills it with the operations that can be
// undone or redone, most recent first, and puts it under the button. Choosing
// an entry hides it and tells the controller container to undo or redo that
// many more steps. Losing the focus or Escape hides it without choosing.
//
// Under MFC it is CMDDLDialog over IDD_MENU_DROP_DOWN_LIST.
namespace NMenuDropDown
{
	class IView
	{
	public:
		virtual ~IView() {}

		// Fills the list, selects the first entry and shows the window with its
		// top-left at nX, nY on the screen, taking the focus. nCommandID is what
		// a choice sends: ID_CC_UNDO or ID_CC_REDO.
		virtual void Show( int nX, int nY, unsigned nCommandID, const std::list<std::string> &rEntries ) = 0;
	};

	// Hidden until Show, owned by pParent's frame. pParent is used only during
	// this call. The caller owns the result and deletes it.
	IView* Create( IWidget *pParent );


	// What choosing entry nIndex does, in both: the controller container is
	// sent nCommandID with the index as its data. A negative index, which is
	// what a list with nothing selected answers, sends nothing -- the MFC
	// dialog sent it and Undo and Redo ignored it.
	void Choose( unsigned nCommandID, int nIndex );
}
