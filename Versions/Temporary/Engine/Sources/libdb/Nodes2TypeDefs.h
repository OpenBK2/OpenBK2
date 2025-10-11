#pragma once

namespace NLang
{
	class CLangNode;
}

namespace NDb
{
	namespace NTypeDef
	{
		struct STypeDef;
	}
}

typedef std::unordered_map< NLang::CLangNode*, CObj<NDb::NTypeDef::STypeDef> > CNodes2TypeDefs;
typedef std::unordered_map<NDb::NTypeDef::STypeDef*, CPtr<NDb::NTypeDef::STypeDef>> CClasses2Refs;
