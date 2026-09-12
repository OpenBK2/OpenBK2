#pragma once

#include "MapEditorLib/Interface_Builder.h"
#include "MapEditorLib/Interface_Widget.h"

#include <string>
#include <vector>

// "Create New <type> Object", behind a boundary that names no toolkit.
//
// The dialog every new database object goes through: the browser's insert,
// CDefaultBuilderBase::InsertObject and the property browser's external text
// file editor all reach it through IBuilderContainer::FillNewObjectName. It
// asks for a type and a name, and answers by filling in the caller's
// SBuildDataParams.
//
// Two of its rules are not about drawing and are shared by both
// implementations, in NewObjectViewMfc.cpp:
//
//   * the type postfix -- the "Add Type" button appends "_<type>" to the name
//     and takes it off again, and it has to come off the old type before it
//     goes on the new one when the type changes;
//   * whether OK is available at all, which is three separate conditions and
//     the one thing a wrong port would quietly get wrong.
//
// The dialog remembers its size, its position and the state of that button,
// all three in Editor/ResizeDialogStyles/CNewObjectDialog.xml, which both
// implementations read and write.
namespace NNewObject
{
	// Runs the dialog modally over pParent. True on OK, with *pBuildDataParams
	// holding the type, the name and the export flag the user settled on.
	//
	// *pBuildDataParams is written as the user types either way: the MFC dialog
	// has always done that and the caller discards the struct when this answers
	// false, so nothing was gained by making the wx one tidier.
	bool Run( IWidget *pParent, const std::vector<std::string> &rObjectTypeNameList,
						int nObjectTypeNameIndex, SBuildDataParams *pBuildDataParams );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, const std::vector<std::string> &rObjectTypeNameList,
							 int nObjectTypeNameIndex, SBuildDataParams *pBuildDataParams );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, const std::vector<std::string> &rObjectTypeNameList,
							int nObjectTypeNameIndex, SBuildDataParams *pBuildDataParams );
#endif

	// Adds "_<type>" to the object's name, or takes it off, so that the name
	// carries its type exactly when bAddType says it should. Idempotent: it
	// looks for the postfix before adding one, case-insensitively, because the
	// name is the user's and its case is not.
	void ApplyTypePostfix( SBuildDataParams *pBuildDataParams, bool bAddType );

	// Whether OK should be enabled. Three conditions, all of them the MFC
	// dialog's: something has been typed, what is left is more than the type
	// postfix on its own, and no object of this type already has the name.
	bool CanAccept( SBuildDataParams *pBuildDataParams, const std::string &rszTypedName );

	// The caption, which names the type: IDS_PC_BD_DIALOG_TITLE.
	std::string Title( const SBuildDataParams *pBuildDataParams );
}
