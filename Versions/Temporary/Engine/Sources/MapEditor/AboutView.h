#pragma once

#include "MapEditorLib/Interface_Widget.h"

// The About box.
//
// No inputs and no answer -- it is shown, it is read, it is closed -- so Run
// takes only the window to sit over and returns nothing. The three strings it
// displays come from IUserDataContainer, which is where CAboutDialog already
// got them.
namespace NAbout
{
	void Run( IWidget *pParent );

	// Named so the dispatcher can reach them; not for anything else to call.
	void RunMfc( IWidget *pParent );
#ifdef OBK2_WITH_WX
	void RunWx( IWidget *pParent );
#endif
}
