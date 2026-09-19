#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "AboutView.h"
#include "AboutDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// About as it has always been: CAboutDialog over IDD_ABOUT, filling three
// statics from IUserDataContainer in OnInitDialog.
//
// The dialog class is untouched. This is its call site moved behind
// NAbout::Run.

namespace NAbout
{
	void RunMfc( IWidget *pParent )
	{
		// The original passed AfxGetMainWnd() explicitly, and the call site still
		// does -- it just passes it as a widget now.
		CAboutDialog aboutDialog( ToCWnd( pParent ) );
		aboutDialog.DoModal();
	}


	void Run( IWidget *pParent )
	{
		// The same flag every migrated dialog follows, so a session runs either
		// the MFC set or the wx set. See SelectTablesViewMfc.cpp.
		if ( NToolkit::UseWxViews() )
		{
			RunWx( pParent );
			return;
		}
		RunMfc( pParent );
	}
}
