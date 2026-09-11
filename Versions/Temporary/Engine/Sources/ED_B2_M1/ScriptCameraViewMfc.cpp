#include "stdafx.h"

#include "ScriptCameraView.h"
#include "ScriptCameraWindow.h"
#include "ED_B2_M1Dll.h"

#include <cstdlib>

// The script camera palette as it has always been: CScriptCameraWindow, a
// CResizeDialog over IDD_TAB_MI_SCRIPT_MOVIES with a Save button, a five column
// camera list and three hidden manual-control rows.
//
// The window class is untouched apart from taking the dialog-data half of its
// command dispatch from CScriptCameraCommands. This is its creation site, moved
// out of MapInfoEditor.

namespace NScriptCameraView
{
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow )
	{
		CScriptCameraWindow *pDialog = pTabWindow->AddNewTab( static_cast<CScriptCameraWindow*>( 0 ) );
		if ( pDialog == 0 )
		{
			return 0;
		}
		// The dialog template lives in this module's resources, not the
		// executable's, so the resource handle is swapped for the call and put
		// back. Exactly what the creation site in MapInfoEditor did.
		AfxSetResourceHandle( theEDB2M1Instance );
		pDialog->Create( CScriptCameraWindow::IDD, pTabWindow );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		return pDialog;
	}


	CWnd* Create( CDefault3DTabWindow *pTabWindow )
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated piece follows, so a session runs either
		// the MFC set or the wx set.
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		if ( pszUseWx != 0 && pszUseWx[0] != '0' && pszUseWx[0] != '\0' )
		{
			return CreateWx( pTabWindow );
		}
#endif
		return CreateMfc( pTabWindow );
	}
}
