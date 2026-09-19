#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "SelectTablesView.h"
#include "SelectTablesDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// Select Tables as it has always been: CSelectTablesDialog, a CResizeDialog over
// the IDD_CHOOSE_TABLES template with a CCheckListBox on it.
//
// Nothing here is new behaviour. The dialog class is untouched; this is the
// six lines of its call site, moved behind NSelectTables::Run so that the
// browser stops naming an MFC dialog and something else can answer instead.

namespace NSelectTables
{
	bool RunMfc( IWidget *pParent, const std::list<std::string> &rTables, CTableSet *pSelectedTables )
	{
		if ( pSelectedTables == 0 )
		{
			return false;
		}
		CSelectTablesDialog selectTablesDialog( ToCWnd( pParent ) );
		selectTablesDialog.tables = rTables;
		selectTablesDialog.selectedTables = ( *pSelectedTables );
		if ( selectTablesDialog.DoModal() != IDOK )
		{
			return false;
		}
		( *pSelectedTables ) = selectTablesDialog.selectedTables;
		return true;
	}


	bool Run( IWidget *pParent, const std::list<std::string> &rTables, CTableSet *pSelectedTables )
	{
		// OBK2_WX_DIALOGS rather than a per-dialog switch: as more modal dialogs
		// move across they should all follow one flag, so that a session runs
		// either the MFC set or the wx set and the comparison is of the editor
		// rather than of one window.
		if ( NToolkit::UseWxViews() )
		{
			return RunWx( pParent, rTables, pSelectedTables );
		}
		return RunMfc( pParent, rTables, pSelectedTables );
	}
}
