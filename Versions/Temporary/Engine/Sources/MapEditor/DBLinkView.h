#pragma once

#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/Interface_Widget.h"

#include <string>

// The database link picker, CPCDBLinkDialog, behind a boundary that names no
// toolkit.
//
// It picks one object. The tables it may come from are in a list over each
// table's tree, the selected object's fields are in the property grid beside
// it -- editable when the caller allows -- and under both are the object
// selected now and the one the picker opened on. A property's Browse button
// asks it for a reference; the frame asks it for an object to open, or to
// point at.
//
// The picker remembers its size, its position and its three column widths in
// Editor/ResizeDialogStyles/CPCDBLinkDialog.xml, which both implementations
// read and write.
namespace NDBLink
{
	enum EType
	{
		// Opening an object: no previous selection shown, no Set Empty.
		TYPE_OPEN = 0,
		// Pointing a reference at one.
		TYPE_LINK = 1,
	};

	struct SRequest
	{
		EType eType = TYPE_LINK;
		// The selections are shown as "Table:Object", for a reference that may
		// point into several tables.
		bool bMultiRef = false;
		// A multiline editor for the selected field under the grid:
		// IDD_PC_DB_LINK_EX.
		bool bTextEditor = false;
		// The list's width, and with the editor the grid's height, in pixels;
		// 0 leaves the template's.
		int nFixedWidth = 0;
		int nFixedHeight = 0;
		// Whether the fields and the tables' trees may be edited.
		bool bEnableEdit = true;
		// The tables the object may come from.
		CTableSet selectedTables;
		// Where the picker opens, which it also shows as the previous selection.
		std::string szTable;
		std::string szObject;
	};

	struct SResult
	{
		// The table and object selected when the picker closed.
		std::string szTable;
		std::string szObject;
		// Closed by Set Empty rather than OK.
		bool bEmpty = false;
	};

	// Runs the picker modally over pParent. True on OK or Set Empty, with what
	// was selected in *pResult.
	bool Run( IWidget *pParent, const SRequest &rRequest, SResult *pResult );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, const SRequest &rRequest, SResult *pResult );
	bool RunWx( IWidget *pParent, const SRequest &rRequest, SResult *pResult );
}
