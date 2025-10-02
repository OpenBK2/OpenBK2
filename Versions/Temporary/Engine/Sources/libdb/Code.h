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
interface ICode : public CXmlResource
{
	struct SCodeStreams
	{
		CStrStream h;
		CStrStream cpp;
		CStrStream hEOF;
		CStrStream cppEOF;

		SCodeStreams( string *pszHFile, string *pszCPPFile, string *pszHEOFFile, string *pszCPPEOFFile )
			: h( pszHFile ), cpp( pszCPPFile ), hEOF( pszHEOFFile ), cppEOF( pszCPPEOFFile ) { }
	};
	
	virtual void GenerateCode( SCodeStreams *pCode, const string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const string &szQualifiedName ) = 0;
};

}

