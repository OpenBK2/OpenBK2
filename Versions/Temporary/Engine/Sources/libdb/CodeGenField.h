#pragma once

#include "Code.h"
#include "Nodes2TypeDefs.h"
#include "../System/XmlSaver.h"

namespace NLang
{
	class CVariable;
}

namespace NCodeGen
{

class CFieldDefinition : public ICode
{
	OBJECT_NOCOPY_METHODS( CFieldDefinition )
	string szFieldName;
	CPtr<NDb::NTypeDef::STypeDef> pType;
public:
	CFieldDefinition() { }
	CFieldDefinition( NLang::CVariable *pVarNode, const CNodes2TypeDefs &nodes2TypeDefs );

	virtual void GenerateCode( SCodeStreams *pCode, const string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const string &szQualifiedName );

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Name", &szFieldName );
		saver.Add( "Type", &pType );
		return 0;
	}
};

}

