#pragma once

#include "Nodes2TypeDefs.h"
#include "libdb_export.h"

namespace NLang
{
	class CFileNode;
}

namespace NCodeGen
{
	struct SCodeStructure;
	// exported for dbcodegen, which builds the file layout of the output with it
	LIBDB_EXPORT CXmlResource* GenerateCodeStructure( NLang::CFileNode *pRootFile, const CNodes2TypeDefs &nodes2TypeDefs,
																			const std::string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );
}


