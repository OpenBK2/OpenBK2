#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <string>

// Asks for a name: a caption, a label, an edit box, OK and Cancel. The script
// area state opens it when a new area has been drawn on the map, and places
// the area only if a name comes back.
//
// Same shape as NSearchObject::Run and the other migrated dialogs: a call that
// blocks and answers whether OK was pressed. The edit box starts with the last
// name accepted in this session -- the MFC dialog kept that in a static of its
// own; it is kept here now, so both implementations share it.
namespace NEnterName
{
	// True on OK, with *pszName set to what was typed, which may be empty --
	// the dialog does not require a name and never did; the caller decides what
	// an empty one means. False on Cancel or the close box, *pszName untouched.
	bool Run( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName );

	// Named so the dispatcher can reach them; not for anything else to call.
	// *pszName carries the starting text in and the result out.
	bool RunMfc( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName );
#endif
}
