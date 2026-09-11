#include "stdafx.h"

#include "AIGeneralDialogs.h"
#include "AIGenMobileIDDlg.h"
#include "AIGenParcelDlg.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The AI general palette's dialogs as they have always been: CAIGenMobileDlg
// over IDD_DLG_AIGEN_MOBILE_ID and CAIGenParcelDlg over IDD_DLG_AIGEN_PARCEL.
// The dialog classes are untouched; these are their call sites in
// CAIGeneralPointsState, moved behind the boundary.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		return ( pszUseWx != 0 ) && ( pszUseWx[0] != '0' ) && ( pszUseWx[0] != '\0' );
	}
}


namespace NAIGenMobileDialog
{
	bool RunMfc( IWidget *pParent, int *pMobileID )
	{
		if ( pMobileID == 0 )
		{
			return false;
		}
		// OnOK is the only thing that writes through the pointer, so Cancel
		// leaves the caller's value alone without a copy.
		CAIGenMobileDlg dlg( ToCWnd( pParent ), pMobileID );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, int *pMobileID )
	{
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, pMobileID );
		}
#endif
		return RunMfc( pParent, pMobileID );
	}
}


namespace NAIGenParcelDialog
{
	bool RunMfc( IWidget *pParent, NDb::EParcelType *pType, float *pImportance )
	{
		if ( ( pType == 0 ) || ( pImportance == 0 ) )
		{
			return false;
		}
		CAIGenParcelDlg dlg( ToCWnd( pParent ), pType, pImportance );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, NDb::EParcelType *pType, float *pImportance )
	{
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, pType, pImportance );
		}
#endif
		return RunMfc( pParent, pType, pImportance );
	}
}
