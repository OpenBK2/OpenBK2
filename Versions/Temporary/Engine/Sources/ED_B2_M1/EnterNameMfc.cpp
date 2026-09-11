#include "stdafx.h"

#include "EnterName.h"
#include "EnterNameDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The name prompt as it has always been: CEnterNameDialog over
// IDD_DLG_AREA_NAME. The class now takes its starting text instead of keeping
// the last name itself; this is its call site in CScriptAreaState, moved behind
// the boundary, and the dispatcher that keeps the last name for both
// implementations.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		return ( pszUseWx != 0 ) && ( pszUseWx[0] != '0' ) && ( pszUseWx[0] != '\0' );
	}

	// The last name accepted, which the next prompt starts with -- including an
	// empty one, as CEnterNameDialog::OnOK always stored it. Per process, as the
	// static it replaces was.
	std::string szLastName;
}


namespace NEnterName
{
	bool RunMfc( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName )
	{
		CEnterNameDialog dialog( ToCWnd( pParent ), rszCaption, rszLabel, *pszName );
		if ( dialog.DoModal() != IDOK )
		{
			return false;
		}
		dialog.GetName( pszName );
		return true;
	}


	bool Run( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName )
	{
		if ( pszName == 0 )
		{
			return false;
		}
		std::string szName = szLastName;
#ifdef OBK2_WITH_WX
		const bool bAccepted = UseWx() ? RunWx( pParent, rszCaption, rszLabel, &szName )
																	 : RunMfc( pParent, rszCaption, rszLabel, &szName );
#else
		const bool bAccepted = RunMfc( pParent, rszCaption, rszLabel, &szName );
#endif
		if ( !bAccepted )
		{
			return false;
		}
		szLastName = szName;
		( *pszName ) = szName;
		return true;
	}
}
