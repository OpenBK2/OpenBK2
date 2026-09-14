#pragma once

#include "MapEditorLib/Interface_Builder.h"
#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Interface_PCItemEditor.h"
#include "MapEditorLib/Interface_Widget.h"

#include <string>

struct IView;

// "Create <type>", the build data dialog, behind a boundary that names no
// toolkit.
//
// A builder that needs more than a name before it can make an object --
// a map's size and players, a unit's type -- keeps those in a build data
// object, and IBuilderContainer::FillBuildData shows it here: a name box, the
// build data object's fields in the property grid, a line saying what is still
// wrong with them, the export check and OK. The fields are edited in place,
// through the undo list, and the builder reads them after OK.
//
// What decides whether OK is available is not drawing, and is shared by both
// implementations in BuildDataViewMfc.cpp: CanAccept below.
//
// The dialog remembers its size, its position and its three column widths, in
// Editor/ResizeDialogStyles/CPCBuildDataDialog.xml, which both implementations
// read and write.
namespace NBuildData
{
	// Runs the dialog modally over pParent on the build data object
	// pManipulator stands for. True on OK. The name and the export flag are
	// written into *pBuildDataParams as the user changes them, either way, as
	// CPCBuildDataDialog has always done.
	bool Run( IWidget *pParent, IManipulator *pManipulator, const SObjectSet &rObjectSet,
						const std::string &rszTemporaryLabel, SBuildDataParams *pBuildDataParams,
						IBuildDataCallback *pBuildDataCallback );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, IManipulator *pManipulator, const SObjectSet &rObjectSet,
							 const std::string &rszTemporaryLabel, SBuildDataParams *pBuildDataParams,
							 IBuildDataCallback *pBuildDataCallback );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, IManipulator *pManipulator, const SObjectSet &rObjectSet,
							const std::string &rszTemporaryLabel, SBuildDataParams *pBuildDataParams,
							IBuildDataCallback *pBuildDataCallback );
#endif

	// CPCBuildDataDialog::UpdateOKButton: whether OK should be enabled, and the
	// reason in *pszErrorMessage when it should not -- empty otherwise. With
	// BDF_CHECK_FILE_NAME the name must be given and not taken; with
	// BDF_CHECK_PROPERTIES the builder must accept the fields, which it may
	// fill in itself through pView as it looks at them.
	bool CanAccept( SBuildDataParams *pBuildDataParams, IBuildDataCallback *pBuildDataCallback,
									IView *pView, std::string *pszErrorMessage );
}
