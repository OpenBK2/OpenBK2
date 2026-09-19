#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <string>

// The system's file and folder pickers, behind a boundary that names no
// toolkit. Strings are UTF-8, as every narrow string in the tree is.
//
// These were MFC's CFileDialog and the shell's SHBrowseForFolder, called
// straight from the property buttons and from Register XDB. They are wx's
// wxFileDialog and wxDirDialog now (FileDialogsWx.cpp), which on Windows are
// the same system dialogs, owned by the frame pOwner belongs to.
namespace NFileDialog
{
	// An existing file. rszFilter is in MFC's form, "Text (*.txt)|*.txt|All
	// Files (*.*)|*.*||", which is also how the property descriptors write it.
	// rszInitialDir may be empty. True, with the full path in *pszPath, on OK.
	bool OpenFile( IWidget *pOwner, const std::string &rszTitle, const std::string &rszFilter,
								 const std::string &rszInitialDir, std::string *pszPath );

	// An existing folder. rszInitialDir may be empty. True, with the full path
	// in *pszPath and no trailing separator, on OK.
	bool ChooseFolder( IWidget *pOwner, const std::string &rszTitle, const std::string &rszInitialDir,
										 std::string *pszPath );
}
