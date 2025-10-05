#pragma once

#include "StrStream.h"

namespace NDb
{
namespace NTypeDef
{
	struct STypeDef;
	class CTerminalTypesDescriptor;
}
}


namespace NCodeGen
{

class CStrStream;
struct ICode : public CXmlResource
{
	struct SCodeStreams
	{
		CStrStream h;
		CStrStream cpp;
		CStrStream hEOF;
		CStrStream cppEOF;

		SCodeStreams( std::string *pszHFile, std::string *pszCPPFile, std::string *pszHEOFFile, std::string *pszCPPEOFFile )
			: h( pszHFile ), cpp( pszCPPFile ), hEOF( pszHEOFFile ), cppEOF( pszCPPEOFFile ) { }
	};
	
	virtual void GenerateCode( SCodeStreams *pCode, const std::string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const std::string &szQualifiedName ) = 0;
};

}


