#pragma once

#include "Nodes2TypeDefs.h"

namespace NLang
{
	class CFileNode;
}

namespace NCodeGen
{
	struct SCodeStructure;
	CXmlResource* GenerateCodeStructure( NLang::CFileNode *pRootFile, const CNodes2TypeDefs &nodes2TypeDefs,
																			const string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );
}


