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
	class CFileNode;
}

namespace NCodeGen
{

class CNamespace;
class CFile : public CXmlResource
{
	OBJECT_NOCOPY_METHODS( CFile );

	string szName;
	list<string> includes;
	list<string> hExternalIncludes;
	list<string> cppExternalIncludes;
	CObj<CNamespace> pNamespace;
public:
	CFile() { }
	CFile( NLang::CFileNode *pFileNode, const CNodes2TypeDefs &nodes2TypeDefs, const string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );

	const string &GetName() const { return szName; }

	void GenerateCode( const string &szRootDir );
	int operator&( IXmlSaver &saver );
};

}


