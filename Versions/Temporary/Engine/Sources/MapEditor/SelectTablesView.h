#pragma once

#include "MapEditorLib/Interface_Builder.h"		// CTableSet
#include "MapEditorLib/Interface_Widget.h"

#include <list>
#include <string>

// The Select Tables dialog, as a function rather than a class.
//
// This is the second panel-at-a-time slice of the wx migration and the first
// modal one, and the shape is different from ILogView on purpose. A panel is an
// object with a lifetime, so it gets an interface. A modal dialog is a call that
// blocks until the user answers: inputs in, one bool out, outputs written back.
// Wrapping that in an interface with Create/Show/Destroy would be inventing a
// lifetime the thing does not have.
//
// CDWGDBBrowser::SelectTables was already written this way and did not know it:
//
//     CSelectTablesDialog dlg( this );
//     dlg.tables = tables;
//     dlg.selectedTables = selectedTables;
//     if ( dlg.DoModal() == IDOK ) { selectedTables = dlg.selectedTables; ... }
//
// Nothing toolkit-shaped crosses that boundary except DoModal. So the boundary
// was already there; it just had an MFC class sitting on it.
namespace NSelectTables
{
	// Runs the dialog modally over pParent.
	//
	// rTables is every table the database offers; pSelectedTables is the set
	// that is ticked on the way in and, if this returns true, the set the user
	// ticked on the way out. Returns false if the dialog was cancelled, and then
	// pSelectedTables is untouched.
	bool Run( IWidget *pParent,
						const std::list<std::string> &rTables,
						CTableSet *pSelectedTables );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, const std::list<std::string> &rTables, CTableSet *pSelectedTables );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, const std::list<std::string> &rTables, CTableSet *pSelectedTables );
#endif
}
