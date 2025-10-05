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

struct SNodesHash
{
	int operator()( NLang::CLangNode *pNode ) const;
	int operator()( NDb::NTypeDef::STypeDef *pNode ) const;
};

typedef std::unordered_map< NLang::CLangNode*, CObj<NDb::NTypeDef::STypeDef>, SNodesHash > CNodes2TypeDefs;
typedef std::unordered_map<NDb::NTypeDef::STypeDef*, CPtr<NDb::NTypeDef::STypeDef>, SNodesHash> CClasses2Refs;


