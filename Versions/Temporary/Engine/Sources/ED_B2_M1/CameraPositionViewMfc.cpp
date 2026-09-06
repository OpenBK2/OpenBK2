#include "stdafx.h"

#include "CameraPositionView.h"
#include "CameraPositionWindow.h"
#include "ED_B2_M1Dll.h"
#include "ResourceDefines.h"

#include <cstdlib>

// The start-camera-positions palette as it has always been: CCameraPositionWindow,
// a CResizeDialog over IDD_TAB_MI_START_CAMERA, created as a child of the tab
// control.
//
// The window class is untouched apart from taking its two dispatch methods from
// CCameraPositionCommands instead of writing them out. This is its creation
// site, moved out of MapInfoEditor so that which toolkit draws the palette is
// decided in one place.
//
// The command dispatch is here rather than in a third file because it is shared
// by both implementations and this one is always compiled.

bool CCameraPositionCommands::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	SCameraPositionWindowData *pData = reinterpret_cast<SCameraPositionWindowData*>( dwData );
	switch ( nCommandID )
	{
		case ID_WINDOW_GET_DIALOG_DATA:
			GetDialogData( pData );
			return true;
		//
		case ID_WINDOW_SET_DIALOG_DATA:
			SetDialogData( pData );
			return true;
	}
	return false;
}


bool CCameraPositionCommands::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CCameraPositionCommands::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CCameraPositionCommands::UpdateCommand(), pbCheck == 0" );
	//
	switch ( nCommandID )
	{
	case ID_WINDOW_SET_DIALOG_DATA:
	case ID_WINDOW_GET_DIALOG_DATA:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}
}


namespace NCameraPositionView
{
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow )
	{
		CCameraPositionWindow *pDialog =
				pTabWindow->AddNewTab( static_cast<CCameraPositionWindow*>( 0 ) );
		if ( pDialog == 0 )
		{
			return 0;
		}
		// The dialog template lives in this module's resources, not the
		// executable's, so the resource handle is swapped for the call and put
		// back. Exactly what the creation site in MapInfoEditor did.
		AfxSetResourceHandle( theEDB2M1Instance );
		pDialog->Create( CCameraPositionWindow::IDD, pTabWindow );
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
