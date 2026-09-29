#pragma once

#include "TestDB_export.h"

#include <string>

namespace NTestDB
{
	// The directory of records the tests open, with a trailing '/'. The tests work
	// on a copy, since the database writes an index and new objects beside them.
	//
	// Exported, and called by every test that uses these types, which is also what
	// makes the test load this library: the types register themselves with the
	// database when it loads, and nothing else in a test names a symbol from it.
	TESTDB_EXPORT std::string GetDataDir();
}
