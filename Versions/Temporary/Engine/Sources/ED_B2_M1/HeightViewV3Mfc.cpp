#include "stdafx.h"

#include "HeightViewV3.h"
#include "HeightWindowV3.h"
#include "ObjectProperties.h"
#include "ED_B2_M1Dll.h"

#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Tools_HashSet.h"

#include <cstdlib>

// The terrain height palette as it has always been: CHeightWindowV3, a
// CResizeDialog over IDD_TAB_MI_TERRAIN_HEIGHT_V3 with twelve icon buttons, a
// check box and a tile list.
//
// The window class is untouched apart from taking the edit-parameter half of
// its command dispatch from CHeightCommandsV3. This is its creation site, moved
// out of MapInfoEditor.
//
// It also holds the part of the palette that belongs to neither toolkit, for
// the plain reason that this is the translation unit that is always compiled:
// the wx half is behind OBK2_WITH_WX and the MFC palette cannot call into it.

namespace NHeightViewV3
{
	void ShowTileProperties( const std::string &rszTileName )
	{
		// A set of one. The showing itself is NObjectProperties::Show, because
		// the map object and VSO palettes want the same thing and the block it
		// replaces was already written out three times.
		SObjectSet objectSet;
		objectSet.szObjectTypeName = TILE_TYPE_NAME;
		InsertHashSetElement( &( objectSet.objectNameSet ), rszTileName );
		NObjectProperties::Show( objectSet );
	}


	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow )
	{
		CHeightWindowV3 *pDialog = pTabWindow->AddNewTab( static_cast<CHeightWindowV3*>( 0 ) );
		if ( pDialog == 0 )
		{
			return 0;
		}
		// The dialog template lives in this module's resources, not the
		// executable's, so the resource handle is swapped for the call and put
		// back. Exactly what the creation site in MapInfoEditor did.
		AfxSetResourceHandle( theEDB2M1Instance );
		pDialog->Create( CHeightWindowV3::IDD, pTabWindow );
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
