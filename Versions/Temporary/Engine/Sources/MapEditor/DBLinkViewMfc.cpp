#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "DBLinkView.h"
#include "PC_DBLinkDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The link picker as it has always been, CPCDBLinkDialog, behind the boundary,
// and the dispatcher.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}
}


namespace NDBLink
{
	bool RunMfc( IWidget *pParent, const SRequest &rRequest, SResult *pResult )
	{
		CPCDBLinkDialog dialog( ( rRequest.eType == TYPE_OPEN ) ? CPCDBLinkDialog::TYPE_OPEN : CPCDBLinkDialog::TYPE_LINK,
														rRequest.bMultiRef, rRequest.bTextEditor, rRequest.nFixedWidth, rRequest.nFixedHeight,
														ToCWnd( pParent ) );
		dialog.SetSelectedTables( rRequest.selectedTables );
		dialog.SetCurrentTable( rRequest.szTable );
		dialog.SetCurrentObject( rRequest.szObject );
		dialog.EnableEdit( rRequest.bEnableEdit );
		if ( dialog.DoModal() != IDOK )
		{
			return false;
		}
		if ( pResult != 0 )
		{
			dialog.GetCurrentTable( &( pResult->szTable ) );
			dialog.GetCurrentObject( &( pResult->szObject ) );
			pResult->bEmpty = dialog.IsEmpty();
		}
		return true;
	}


	bool Run( IWidget *pParent, const SRequest &rRequest, SResult *pResult )
	{
#ifdef OBK2_WITH_WX
		// The multiline editor under the grid is the MFC property tree's own, and
		// has no wx counterpart yet: a picker that asks for it stays MFC.
		if ( UseWx() && !rRequest.bTextEditor )
		{
			return RunWx( pParent, rRequest, pResult );
		}
#endif
		return RunMfc( pParent, rRequest, pResult );
	}
}
