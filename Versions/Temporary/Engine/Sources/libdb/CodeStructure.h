#pragma once

#include "Nodes2TypeDefs.h"
#include "../System/XmlSaver.h"

namespace NDb
{
namespace NTypeDef
{
	class CTerminalTypesDescriptor;
} 
}

namespace NLang
{
	class CFileNode;
}

namespace NCodeGen
{
class CFile;
	
class CCodeStructure : public CXmlResource
{
	OBJECT_NOCOPY_METHODS( CCodeStructure )
	list< CObj<CFile> > files;
public:
	CCodeStructure() { }
	CCodeStructure( NLang::CFileNode *pRootFile, const CNodes2TypeDefs &nodes2TypeDefs, const string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );

	const list< CObj<CFile> > &GetFiles() const { return files; }

	void GenerateCode( const string &szRootDir );

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Files", &files );
		return 0;
	}
};
}


