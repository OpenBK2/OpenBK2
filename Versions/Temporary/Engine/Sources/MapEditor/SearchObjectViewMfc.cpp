#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "SearchObjectView.h"
#include "SearchObjectDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// Find Object as it has always been: CSearchObjectDialog over IDD_SEARCH_OBJECT,
// a label, an edit and two buttons.
//
// The dialog class is untouched. This is its call site moved behind
// NSearchObject::Run.

namespace NSearchObject
{
	bool RunMfc( IWidget *pParent, std::string *pszText )
	{
		if ( pszText == 0 )
		{
			return false;
		}
		// No parent, as the original had none: MFC then owns the dialog with the
		// main window, and passing one now would change which window is disabled.
		CSearchObjectDialog searchObjectDialog;
		searchObjectDialog.SetText( *pszText );
		if ( searchObjectDialog.DoModal() != IDOK )
		{
			return false;
		}
		( *pszText ) = searchObjectDialog.GetText();
		return true;
	}


	bool Run( IWidget *pParent, std::string *pszText )
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated dialog follows, so a session runs either
		// the MFC set or the wx set. See SelectTablesViewMfc.cpp.
		if ( NToolkit::UseWxViews() )
		{
			return RunWx( pParent, pszText );
		}
#endif
		return RunMfc( pParent, pszText );
	}
}
