#include "stdafx.h"

#include "ModelView.h"
#include "ModelWindow.h"
#include "ED_B2_M1Dll.h"

#include <cstdlib>

// The model palette as it has always been: CModelWindow, a CResizeDialog over
// IDD_TAB_MODEL_TOOL with six combo boxes, four edit boxes with their buttons,
// five check boxes and two radio groups.
//
// The window class is untouched apart from taking its whole command dispatch
// from CModelCommands. This is its creation, moved out of
// CModelEditor::CreateControls.

namespace NModelView
{
	std::unique_ptr<CWnd> CreateMfc( CWnd *pParent )
	{
		std::unique_ptr<CModelWindow> pWindow( new CModelWindow() );
		// The dialog template lives in this module's resources, not the
		// executable's, so the resource handle is swapped for the call and put
		// back. Exactly what CreateControls did.
		AfxSetResourceHandle( theEDB2M1Instance );
		const BOOL bCreated = pWindow->Create( CModelWindow::IDD, pParent );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		// CreateControls did not look at the result and handed the pane a
		// window with no handle if this failed. Null is the better answer, and
		// the caller shows the pane either way.
		if ( !bCreated )
		{
			return nullptr;
		}
		return pWindow;
	}


	std::unique_ptr<CWnd> Create( CWnd *pParent )
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated piece follows, so a session runs either
		// the MFC set or the wx set.
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		if ( pszUseWx != 0 && pszUseWx[0] != '0' && pszUseWx[0] != '\0' )
		{
			return CreateWx( pParent );
		}
#endif
		return CreateMfc( pParent );
	}
}
