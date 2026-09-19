#pragma once

#include <string>

// The XML half of Register XDB's check on a file before its header goes into
// index.bin, behind a boundary that names no toolkit, so that the caller, which
// also deals in the database, need not include wx. Strings are UTF-8, as every
// narrow string in the tree is.
//
// This was MSXML 6 through COM, held with ATL's smart pointers: Windows only,
// and the last thing that needed ATL in the build. It is wxXmlDocument now
// (XdbValidationWx.cpp), whose parser is the expat wx carries.
//
// It checks that the document is well-formed, not that it is valid: expat is a
// non-validating parser, and MSXML was run with validation off as well. Whether
// the root element names a known resource type is the caller's to check.
namespace NXdbValidation
{
	// Whether the nSize bytes at pData, the whole file as the engine will read
	// it, are a well-formed XML document in UTF-8 with no DOCTYPE. True, with the
	// root element's name in *pszRootName, if so; false, with a message for the
	// user in *pszError, if not.
	bool CheckDocument( const char *pData, int nSize, std::string *pszRootName, std::string *pszError );
}
