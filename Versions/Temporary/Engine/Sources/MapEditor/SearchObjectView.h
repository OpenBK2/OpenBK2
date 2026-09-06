#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <string>

// Find Object: type a name, get taken to it in the tree.
//
// The MFC pair was a setter, a DoModal and a getter for one string:
//
//     CSearchObjectDialog dlg;
//     dlg.SetText( szSearch );
//     if ( dlg.DoModal() == IDOK ) { szSearch = dlg.GetText(); ... }
//
// which is an in-out parameter written the long way round. pszText carries the
// text in and, on true, carries back what the user typed; on cancel it is not
// touched. That is also what the caller wanted, since the string it passes is
// the remembered search text it means to update.
namespace NSearchObject
{
	// Runs the dialog modally over pParent. Returns true if accepted, with
	// *pszText set to the entered text -- which may be empty, because the
	// dialog does not require anything and neither did the one it replaces.
	bool Run( IWidget *pParent, std::string *pszText );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, std::string *pszText );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, std::string *pszText );
#endif
}
