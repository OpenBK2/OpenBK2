#include "stdafx.h"

#include "ReinfPointsDialogs.h"
#include "ReinfPointsTypedDlg.h"
#include "ReinfPointsTypedTemplateAddDlg.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The reinforcement point dialogs as they have always been: CReinfPointsTypedDlg
// over IDD_DLG_REINFPTS_TEMPLATES and CReinfPointsTypedTemplateAddDlg over
// IDD_DLG_REINFPTS_ADD_TEMPLATE. Their call sites, moved behind the boundary,
// and the dispatcher in front of both implementations.

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


namespace NReinfPointsTemplates
{
	bool RunMfc( IWidget *pParent, CReinfPointsState::CTypedTemplateType *pTemplates,
							 CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint )
	{
		CReinfPointsTypedDlg dlg( ToCWnd( pParent ), pTemplates, pMapInfoEditor, nPlayer, nReinfPoint );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, CReinfPointsState::CTypedTemplateType *pTemplates,
						CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint )
	{
		if ( pTemplates == 0 || pMapInfoEditor == 0 )
		{
			return false;
		}
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, pTemplates, pMapInfoEditor, nPlayer, nReinfPoint );
		}
#endif
		return RunMfc( pParent, pTemplates, pMapInfoEditor, nPlayer, nReinfPoint );
	}
}


namespace NReinfPointsAddTemplate
{
	bool RunMfc( IWidget *pParent, const std::string &rszNode, CMapInfoEditor *pMapInfoEditor )
	{
		// The dialog takes the node path by pointer and only reads it; the
		// boundary says so by taking a reference.
		std::string szNode = rszNode;
		CReinfPointsTypedTemplateAddDlg dlg( ToCWnd( pParent ), &szNode, pMapInfoEditor );
		return ( dlg.DoModal() == IDOK );
	}


	bool Run( IWidget *pParent, const std::string &rszNode, CMapInfoEditor *pMapInfoEditor )
	{
		if ( pMapInfoEditor == 0 )
		{
			return false;
		}
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunWx( pParent, rszNode, pMapInfoEditor );
		}
#endif
		return RunMfc( pParent, rszNode, pMapInfoEditor );
	}
}
