#pragma once

#include "Code.h"
#include "Nodes2TypeDefs.h"
#include "../System/XmlSaver.h"

namespace NLang
{
	class CComplexTypeNode;
	class CEnumNode;
}

namespace NCodeGen
{

class CNamespace;
class CTypeDefinition : public ICode
{
	OBJECT_NOCOPY_METHODS( CTypeDefinition )

	CPtr<NDb::NTypeDef::STypeDef> pType;
	CObj<CNamespace> pNamespace;
	bool bTerminal;
public:
	CTypeDefinition() : bTerminal( true ) { }
	CTypeDefinition( NLang::CComplexTypeNode *pComplexTypeNode, const CNodes2TypeDefs &nodes2TypeDefs, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );
	CTypeDefinition( NLang::CEnumNode *pEnumNode, const CNodes2TypeDefs &nodes2TypeDefs );

	virtual void GenerateCode( SCodeStreams *pCode, const string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const string &szQualifiedName );

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Type", &pType );
		saver.Add( "Namespace", &pNamespace );
		saver.Add( "Terminal", &bTerminal );
		return 0;
	}
};

}

