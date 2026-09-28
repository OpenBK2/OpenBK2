#pragma once

#include "Nodes2TypeDefs.h"
#include "libdb_export.h"

namespace NDb
{
	namespace NTypeDef
	{
		struct STypeDef;
		class CTerminalTypesDescriptor;
	}
}

namespace NLang
{
	class CNamespace;
}

namespace NCompileCLike
{
	// exported for dbcodegen, which compiles the parsed .cll files with it
	LIBDB_EXPORT bool Compile( std::vector< CObj<NDb::NTypeDef::STypeDef> > *pTypes, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc,
								CNodes2TypeDefs *pNodes2TypeDefs, NLang::CNamespace *pRootNN );
}


