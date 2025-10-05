#pragma once

#include "Code.h"
#include "Nodes2TypeDefs.h"
#include "System/XmlSaver.h"

namespace NLang
{
	class CNamespace;
}

namespace NCodeGen
{

class CNamespace : public ICode
{
	OBJECT_NOCOPY_METHODS( CNamespace )

	std::list< CObj<ICode> > definitions;
	std::list<std::string> badIncludes;
public:
	CNamespace() { }
	CNamespace( NLang::CNamespace *pNM, const CNodes2TypeDefs &nodes2TypeDefs, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );

	virtual void GenerateCode( SCodeStreams *pCode, const std::string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const std::string &szQualifiedName );

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Definitions", &definitions );
		saver.Add( "BadIncludes", &badIncludes );
		return 0;
	}
};

}


