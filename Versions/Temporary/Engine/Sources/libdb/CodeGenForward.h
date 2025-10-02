#pragma once

#include "Code.h"
#include "Nodes2TypeDefs.h"
#include "../System/XmlSaver.h"

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
	CForwardDefinition() { }
	CForwardDefinition( NLang::CTypeNode *pTypeNode, const CNodes2TypeDefs &nodes2TypeDefs );

	virtual void GenerateCode( SCodeStreams *pCode, const string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const string &szQualifiedName );

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Type", &pType );
		return 0;
	}
};

}


