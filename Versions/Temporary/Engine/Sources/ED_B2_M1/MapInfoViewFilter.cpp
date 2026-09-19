#include "stdafx.h"

#include "MapInfoViewFilter.h"
#include "CommandHandlerDefines.h"

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResourceDefines.h"

// View -> Filter's Apply, which the dialog (MapInfoViewFilterWx.cpp) calls.

namespace NMapInfoViewFilter
{
	// Moved out of CMapInfoViewFilterDlg::Apply unchanged.
	void Apply()
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_VIEW_APPLY_MI_FILTER, 0 );
	}
}
