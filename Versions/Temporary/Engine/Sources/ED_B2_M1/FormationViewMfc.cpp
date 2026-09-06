#include "stdafx.h"

#include "FormationView.h"
#include "FormationWindow.h"
#include "ED_B2_M1Dll.h"

#include <cstdlib>

// The squad formations palette as it has always been: CFormationWindow, a
// CResizeDialog over IDD_TAB_SQD_FORMATION with a report-mode list and a
// checkbox.
//
// The window class is untouched apart from taking its command dispatch from
// CPaletteCommands. This is its creation site, moved out of SquadEditor so that
// which toolkit draws the palette is decided in one place.

namespace NFormationView
{
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow )
	{
		CFormationWindow *pDialog = pTabWindow->AddNewTab( new CFormationWindow() );
		if ( pDialog == 0 )
		{
			return 0;
		}
		// The dialog template lives in this module's resources, not the
		// executable's, so the resource handle is swapped for the call and put
		// back. Exactly what the creation site in SquadEditor did.
		AfxSetResourceHandle( theEDB2M1Instance );
		const BOOL bResult = pDialog->Create( CFormationWindow::IDD, pTabWindow );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		NI_ASSERT( bResult, "Creation of CFormationWindow dialog failed" );
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
