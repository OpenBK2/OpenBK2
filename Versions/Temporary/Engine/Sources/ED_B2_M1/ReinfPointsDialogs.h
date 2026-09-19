#pragma once

#include "MapEditorLib/Interface_Widget.h"
#include "ReinfPointsState.h"

#include <string>

// The reinforcement points palette's two dialogs, behind a boundary that names
// no toolkit. A point's Typed templates button opens the first, from
// CReinfPointsState::EditPointTypedTemplate; that dialog's Add button opens the
// second on a database node it has just created.
//
// These are the first migrated dialogs that do not fill in a struct and hand it
// back. They write into the map through the view manipulator and the undo
// controller as their buttons are pressed, and that half is in
// ReinfPointsDialogs.cpp, which names no toolkit either. Both implementations
// call it rather than carrying a copy each: an insert that has to be rolled
// back when the dialog on top of it is cancelled is not a thing to write twice
// and hope the two stay the same.
//
// What Cancel means here is worth saying out loud, because it is not what the
// button suggests. Add and Remove change the map as they are pressed. Cancel on
// the templates dialog only drops the caller's copy of the list -- the database
// operations stay, on the editor's own undo stack where the user can undo them.
// That is how this dialog has always behaved.
namespace NReinfPointsTemplates
{
	// The typed templates of one reinforcement point: a list of type and
	// template, with Add and Remove. True on OK, with *pTemplates holding what
	// the list showed; on Cancel the caller's copy is left as it was.
	bool Run( IWidget *pParent, CReinfPointsState::CTypedTemplateType *pTemplates,
						CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint );


	// The Add button: makes a node, asks the add dialog to fill it in, and takes
	// the node out again if that dialog is cancelled. True when a template was
	// added, which is when *pTemplates has grown by one and the list wants
	// rebuilding.
	bool Add( IWidget *pParent, CReinfPointsState::CTypedTemplateType *pTemplates,
						CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint );

	// The Remove button, after the user has confirmed it: takes the node out of
	// the map and the template out of *pTemplates. Asking the question stays
	// with the dialogs, because a message box is a toolkit's.
	void Remove( CReinfPointsState::CTypedTemplateType *pTemplates,
							 CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint, int nSelected );
}


namespace NReinfPointsAddTemplate
{
	// Fills in one typed template node, named by its path in the map:
	// its type, from the reinforcement type mnemonics, and its template, typed
	// in or picked through the database link browser. Both are written through
	// the manipulator on OK, and nothing is written on Cancel.
	bool Run( IWidget *pParent, const std::string &rszNode, CMapInfoEditor *pMapInfoEditor );
}
