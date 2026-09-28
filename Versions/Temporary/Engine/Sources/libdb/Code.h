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
		// The module the file being generated belongs to: the directory it goes
		// in, which is the module's CMake target name, such as Stats_B2_M1.
		std::string szModule;

		SCodeStreams( std::string *pszHFile, std::string *pszCPPFile, std::string *pszHEOFFile, std::string *pszCPPEOFFile )
			: h( pszHFile ), cpp( pszCPPFile ), hEOF( pszHEOFFile ), cppEOF( pszCPPEOFFile ) { }

		// The module as its export macros spell it, STATS_B2_M1 in
		// STATS_B2_M1_EXPORT: in capitals, with a leading underscore when it starts
		// with a digit, as generate_export_header makes _3DMOTOR of 3Dmotor. The
		// DB registration macros take it too.
		std::string GetModuleMacroName() const
		{
			std::string szResult;
			if ( !szModule.empty() && szModule[0] >= '0' && szModule[0] <= '9' )
			{
				szResult += '_';
			}
			for ( const char c : szModule )
			{
				szResult += ( c >= 'a' && c <= 'z' ) ? char( c - 'a' + 'A' ) : c;
			}
			return szResult;
		}
	};
	
	virtual void GenerateCode( SCodeStreams *pCode, const std::string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const std::string &szQualifiedName ) = 0;
};

}


