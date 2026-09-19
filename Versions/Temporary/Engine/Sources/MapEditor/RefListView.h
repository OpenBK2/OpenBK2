#pragma once

#include "libdb/Manipulator.h"
#include "MapEditorLib/Interface_Widget.h"

#include <list>
#include <string>
#include <vector>

// "What points at this object", behind a boundary that names no toolkit.
//
// Two dialogs, one after the other, from the browser tree's ID_OBJECT_REF_LOOKUP:
// a small one that waits while the database is scanned, and then the list of
// objects that reference the selected one, with the fields of whichever is
// picked and the two buttons that empty those fields.
//
// Nearly all of it is here rather than in either dialog, because nearly none of
// it is drawing: the parse-and-sort of what the scan returned, finding the
// fields of one object that point at the target, and clearing them -- one
// object's or every object's. A wrong copy of the last of those would quietly
// null out the wrong field in someone's database, which is the kind of thing
// that should exist once.
namespace NRefList
{
	// One referencing object, split out of the "type:name" the scan returns.
	struct SReferenceObject
	{
		std::string szTypeName;
		std::string szObjectName;
		// What the list shows, which is the two joined by TYPE_SEPARATOR_CHAR.
		std::string szDisplayName;
	};


	// The scan, shown behind a dialog that waits for it: the resource manager
	// answers a piece at a time, so this asks until it says it is complete.
	// True when it completed; false when the user closed the dialog first, in
	// which case the list is not opened. *pReferenceObjects is filled as it
	// goes either way.
	bool RunScan( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
								std::list<std::string> *pReferenceObjects );

	// The references themselves. Nothing is returned: the dialog does its work
	// through the database as the buttons are pressed, and the caller ignored
	// the modal result even before this boundary existed.
	void Run( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
						std::list<std::string> *pReferenceObjects );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunScanMfc( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
									 std::list<std::string> *pReferenceObjects );
	void RunMfc( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
							 std::list<std::string> *pReferenceObjects );
	bool RunScanWx( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
									std::list<std::string> *pReferenceObjects );
	void RunWx( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
							std::list<std::string> *pReferenceObjects );


	// ---- the part that is not drawing ----

	// "type:name", which is what the title says and what the fields are matched
	// against.
	std::string FullName( const std::string &rszTypeName, const std::string &rszName );

	// What the scan returned, parsed and sorted by full name. Entries whose
	// type could not be read are dropped, as they always were.
	void BuildObjects( std::vector<SReferenceObject> *pObjects,
										 const std::list<std::string> &rReferenceObjects );

	// The fields of one object that point at the target. Answers the object's
	// manipulator so the caller can clear them, null if the object is no longer
	// in the database -- *pszText says so in that case, and is otherwise the
	// field names one per line.
	CPtr<IManipulator> FindFields( std::list<std::string> *pFields, std::string *pszText,
																 const SReferenceObject &rObject,
																 const std::string &rszTargetTypeName,
																 const std::string &rszTargetName );

	// Clears those fields, appending what happened to *pszText as it goes --
	// the dialog shows that text while it works. Stops at the first field that
	// will not take a null, and leaves it in *pFields. True when *pFields ends
	// up empty, which is when the object no longer references the target.
	bool ClearFields( std::list<std::string> *pFields, std::string *pszText,
										IManipulator *pManipulator );

	// The same for every object in the list, without the running commentary.
	// False if any field would not take a null.
	bool ClearAll( const std::vector<SReferenceObject> &rObjects,
								 const std::string &rszTargetTypeName, const std::string &rszTargetName );
}
