#include "stdafx.h"

#include "MapInfoViewFilter.h"
#include "MapInfoViewFilterDlg.h"
#include "CommandHandlerDefines.h"

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"

#include <cstdlib>

// View -> Filter as it has always been: CMapInfoViewFilterDlg over
// IDD_DLG_MAPINFO_VIEW_FILTER. The dialog class now takes the filter rather than
// the whole editor settings, and otherwise is what it was; this is its call
// site in CMapInfoEditor::ConfigureViewFilter, moved behind the boundary, and
// the dispatcher in front of both implementations.

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


namespace NMapInfoViewFilter
{
	// Moved out of CMapInfoViewFilterDlg::Apply unchanged.
	void Apply()
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_VIEW_APPLY_MI_FILTER, 0 );
	}


	bool RunMfc( IWidget *pParent, CMapInfoEditorSettings::SViewFilterData *pFilter )
	{
		if ( pFilter == 0 )
		{
			return false;
		}
		CMapInfoViewFilterDlg dlg( ToCWnd( pParent ), pFilter );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, CMapInfoEditorSettings::SViewFilterData *pFilter )
	{
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, pFilter );
		}
#endif
		return RunMfc( pParent, pFilter );
	}
}
