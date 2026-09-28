#pragma once
#include "libdb/TypeDef.h"
#include "libdb/Nodes2TypeDefs.h"

namespace NDb
{
namespace NCodeGenTool
{

enum ECodeGenOpts
{
	CODE_GEN_UNKNOWN,
	CODE_GEN_NORMAL,
	CODE_GEN_NOCOPY,
	CODE_GEN_TYPES,
	CODE_GEN_SHOW_VERSION,
};

struct SCompiledTypesInfo
{
	std::vector< CObj<NDb::NTypeDef::STypeDef> > types;
	CObj<CXmlResource> pCodeStructure;
	CNodes2TypeDefs nodes2TypeDefs;
};

bool PrecompileTypes( SCompiledTypesInfo *pRes, bool bGenerateCodeStructure, const std::vector<std::string> &files, const std::string &szDescriptorsPath );
bool GenerateTypes( const std::string &szTypesFilePath, SCompiledTypesInfo *pTypesInfo );
bool GenerateCode( std::list<std::string> *pFileTitles, const std::string &szSourceCodePath, SCompiledTypesInfo *pTypesInfo );
bool CopySourceCode( const std::list<std::string> &filetitles, const std::string &szSrcPath, const std::string &szDstPath );

}
}


