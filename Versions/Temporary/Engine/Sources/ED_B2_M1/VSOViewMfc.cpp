#include "stdafx.h"

#include "VSOView.h"
#include "VSOWindow.h"
#include "ED_B2_M1Dll.h"

#include <cstdlib>

// The VSO palette as it has always been: CVSOWindow, a CResizeDialog over
// IDD_TAB_MI_VSO with two radio groups, a width and an opacity box, a filter
// combo and an object list.
//
// The window class is untouched apart from taking the edit-parameter half of
// its command dispatch from CVSOCommands. This is its creation site, moved out
// of MapInfoEditor.

namespace NVSOView
{
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow )
	{
		CVSOWindow *pDialog = pTabWindow->AddNewTab( static_cast<CVSOWindow*>( 0 ) );
		if ( pDialog == 0 )
		{
			return 0;
		}
		// The dialog template lives in this module's resources, not the
		// executable's, so the resource handle is swapped for the call and put
		// back. Exactly what the creation site in MapInfoEditor did.
		AfxSetResourceHandle( theEDB2M1Instance );
		pDialog->Create( CVSOWindow::IDD, pTabWindow );
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
