#include "stdafx.h"

#include "OpenModView.h"
#include "OpenMODDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// Open MOD as it has always been: COpenMODDialog, a CResizeDialog over
// IDD_OPEN_MOD with a combo and a description edit on it.
//
// The dialog class is untouched. This is its call site moved behind
// NOpenMod::Run, with the accept and the "did you actually pick one" question
// answered together instead of one after the other.

namespace NOpenMod
{
	bool RunMfc( IWidget *pParent, NMOD::SMOD *pMod )
	{
		if ( pMod == 0 )
		{
			return false;
		}
		// The MFC dialog took no parent here and neither does this: the original
		// constructed it with the default, which makes MFC use the main window.
		// Passing one now would change which window is disabled.
		COpenMODDialog openMODDialog;
		if ( openMODDialog.DoModal() != IDOK )
		{
			return false;
		}
		return openMODDialog.GetMOD( pMod );
	}


	bool Run( IWidget *pParent, NMOD::SMOD *pMod )
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated dialog follows, so a session runs either
		// the MFC set or the wx set. See SelectTablesViewMfc.cpp.
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		if ( pszUseWx != 0 && pszUseWx[0] != '0' && pszUseWx[0] != '\0' )
		{
			return RunWx( pParent, pMod );
		}
#endif
		return RunMfc( pParent, pMod );
	}
}
