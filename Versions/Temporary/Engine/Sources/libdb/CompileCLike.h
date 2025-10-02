#pragma once

#include "Nodes2TypeDefs.h"

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
	bool Compile( vector< CObj<NDb::NTypeDef::STypeDef> > *pTypes, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc,
								CNodes2TypeDefs *pNodes2TypeDefs, NLang::CNamespace *pRootNN );
}

