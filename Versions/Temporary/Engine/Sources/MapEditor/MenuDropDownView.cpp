#include "stdafx.h"

#include "MenuDropDownView.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"

// What choosing an entry in the undo and redo drop-down does, which the list
// (MenuDropDownViewWx.cpp) calls.

namespace NMenuDropDown
{
	void Choose( unsigned nCommandID, int nIndex )
	{
		if ( nIndex < 0 )
		{
			return;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CONTROLLER_CONTAINER, nCommandID,
																												 static_cast<uintptr_t>( nIndex ) );
	}
}
