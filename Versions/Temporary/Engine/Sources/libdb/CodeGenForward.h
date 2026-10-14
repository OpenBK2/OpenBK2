#pragma once

#include "Code.h"
#include "Nodes2TypeDefs.h"
#include "System/XmlSaver.h"

namespace NLang
{
class CTypeNode;
}

namespace NCodeGen
{

class CForwardDefinition : public ICode
{
	OBJECT_NOCOPY_METHODS( CForwardDefinition )
	CPtr<NDb::NTypeDef::STypeDef> pType;

public:
	CForwardDefinition() {}

	CForwardDefinition( NLang::CTypeNode *pTypeNode, const CNodes2TypeDefs &nodes2TypeDefs );

	void GenerateCode( SCodeStreams *pCode,
	                   const std::string &szTabs,
	                   NDb::NTypeDef::STypeDef *pParentType,
	                   const std::string &szQualifiedName ) override;

	int operator&( IXmlSaver &saver ) override
	{
		saver.Add( "Type", &pType );
		return 0;
	}
};

} // namespace NCodeGen
