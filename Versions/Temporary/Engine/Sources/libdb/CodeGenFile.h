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

	std::string szName;
	std::list<std::string> includes;
	std::list<std::string> hExternalIncludes;
	std::list<std::string> cppExternalIncludes;
	CObj<CNamespace> pNamespace;
public:
	CFile() { }
	CFile( NLang::CFileNode *pFileNode, const CNodes2TypeDefs &nodes2TypeDefs, const std::string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );

	const std::string &GetName() const { return szName; }

	void GenerateCode( const std::string &szRootDir );
	int operator&( IXmlSaver &saver );
};

}


