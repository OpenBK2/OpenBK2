#pragma once

#include "libdb_export.h"

#include <string>
#include <vector>

namespace NDb
{

//! Every database object reference in one .xdb's text, resolved to full names.
//!
//! Split out from the database so it can be tested on its own: the resolution
//! rules are the fiddly part and they have to agree exactly with what the
//! loader does, or "what references this object" answers a different question
//! from "what breaks if I rename it".
//!
//! szOwnFileName is the name of the file the text came from, because a
//! reference may be written relative to it.
LIBDB_EXPORT void CollectObjectReferences( std::vector<std::string> *pRes,
	const char *pBegin, const char *pEnd, const std::string &szOwnFileName );

}
