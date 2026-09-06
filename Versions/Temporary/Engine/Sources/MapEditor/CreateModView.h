#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <string>

// Create MOD: name a folder, a title and a description, and get a new MOD.
//
// The MFC call site read three getters off the dialog and then re-checked two
// of them -- "is the folder non-empty, is the name non-empty" -- which the
// dialog had already decided by greying OK. Same collapse as NOpenMod::Run:
// true means pMod is filled and worth acting on.
namespace NCreateMod
{
	struct SNewMod
	{
		// The full path the MOD is created at, ending in a separator, as
		// MakeFolderPath builds it. Not the folder name the user typed.
		std::string szFolderPath;
		// Wide, because that is what these end up as. The call site writes both
		// to disk as UTF-16 with a BOM, and NMOD::SMOD reads them back into
		// std::wstring; carrying them narrow in between only meant a conversion
		// out through the ANSI code page and another one back.
		std::wstring wszName;
		std::wstring wszDesc;
	};

	// Runs the dialog modally over pParent. True if the user accepted and named
	// a MOD that can be created, with *pMod filled; false on cancel.
	bool Run( IWidget *pParent, SNewMod *pMod );

	// Where a MOD folder of this name would go: the parent of the data storage
	// folder, then Mods, then the name. One definition, because both dialogs
	// need it and two copies of path arithmetic drift.
	std::string MakeFolderPath( const std::string &rszFolderName );

	// Whether a MOD may be created in a folder of this name. False for an empty
	// name, for one carrying a path separator -- it is a single component
	// appended to Mods, not a path -- and for one an existing MOD already
	// occupies. The other half of the rule, that the MOD needs a title too, is
	// left to the caller because it is one test on a different field.
	bool IsFolderNameFree( const std::string &rszFolderName );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, SNewMod *pMod );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, SNewMod *pMod );
#endif
}
