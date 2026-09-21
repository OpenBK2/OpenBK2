#include "stdafx.h"

#include "DBReferenceScan.h"

#include "System/FilePath.h"

#include <cstring>

namespace NDb
{

namespace
{

//! What separates a database object reference from a plain file path.
//!
//! BindProcessorSaveLoad's SaveRefToNode writes every object reference as
//! `<relative path>#xpointer(/<class name>)` and appends that fragment
//! unconditionally, while a file path field is written with no fragment at all.
//! So the fragment is what distinguishes "this points at another database
//! object" from "this points at a .tga on disk", and only the first kind is
//! wanted here. The C# watcher this replaces drew the line in the same place.
const char szXPointer[] = "#xpointer";
const char szHRefAttribute[] = "href=\"";

} // namespace

void CollectObjectReferences( std::vector<std::string> *pRes,
	const char *pBegin, const char *pEnd, const std::string &szOwnFileName )
{
	pRes->clear();
	if ( pBegin == 0 || pEnd <= pBegin )
		return;

	// A raw scan for the attribute rather than an XML parse. Answering this
	// question means reading every object in the database, and parsing them all
	// is far too slow to do while someone waits on a rename. The cost is that a
	// stray `href="` inside a comment would be counted; these files are machine
	// written and the worst a false positive does is mark one more object as
	// changed than had to be.
	const size_t nAttrLen = sizeof(szHRefAttribute) - 1;
	for ( const char *p = pBegin; ; )
	{
		const char *pFound = std::search( p, pEnd, szHRefAttribute, szHRefAttribute + nAttrLen );
		if ( pFound == pEnd )
			break;
		const char *pValue = pFound + nAttrLen;
		const char *pClose = std::find( pValue, pEnd, '"' );
		if ( pClose == pEnd )
			break;
		p = pClose + 1;

		const std::string szHRef( pValue, pClose );
		if ( szHRef.find( szXPointer ) == std::string::npos )
			continue;

		// The same two calls, in the same order, that LoadRefFromNode uses to
		// turn this attribute back into a CDBID. Anything else here would
		// resolve references differently from the code that loads them.
		const std::string szPath = szHRef.substr( 0, szHRef.rfind( '#' ) );
		std::string szResult;
		NFile::MakeFullPath( &szResult, szPath, szOwnFileName );
		NFile::NormalizePath( &szResult );
		if ( !szResult.empty() )
			pRes->push_back( szResult );
	}
}

}
