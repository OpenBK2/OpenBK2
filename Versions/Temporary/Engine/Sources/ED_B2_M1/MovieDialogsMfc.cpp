#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "MovieDialogs.h"
#include "KeySettingsDlg.h"
#include "MovEditorSettingsWindow.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The script movie dialogs as they have always been: CMovEditorSettingsDlg
// over IDD_DLG_MOVED_SETTINGS and CMovEditorKeySettingsDlg over
// IDD_DLG_MOVED_KEY_SETTINGS. The dialog classes are untouched; these are their
// call sites, moved behind the boundary, and the dispatcher in front of both
// implementations.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}
}


namespace NMovieSettings
{
	bool RunMfc( IWidget *pParent, float *pfLength )
	{
		// Only OnOK writes through the pointer, so Cancel leaves the caller's
		// value alone without a copy.
		CMovEditorSettingsDlg dlg( ToCWnd( pParent ), pfLength );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, float *pfLength )
	{
		if ( pfLength == 0 )
		{
			return false;
		}
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, pfLength );
		}
#endif
		return RunMfc( pParent, pfLength );
	}
}


namespace NMovieKeySettings
{
	bool RunMfc( IWidget *pParent, NDb::SScriptMovieKeyPos *pKey, const std::string &rszName )
	{
		// The dialog takes the name by pointer and only reads it; the boundary
		// says so by taking a reference.
		std::string szName = rszName;
		CMovEditorKeySettingsDlg dlg( ToCWnd( pParent ), pKey, &szName );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, NDb::SScriptMovieKeyPos *pKey, const std::string &rszName )
	{
		if ( pKey == 0 )
		{
			return false;
		}
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, pKey, rszName );
		}
#endif
		return RunMfc( pParent, pKey, rszName );
	}
}
