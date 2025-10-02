#pragma once
#include "../libdb/TypeDef.h"
#include "../libdb/Nodes2TypeDefs.h"

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
	vector< CObj<NDb::NTypeDef::STypeDef> > types;
	CObj<CXmlResource> pCodeStructure;
	CNodes2TypeDefs nodes2TypeDefs;
};

bool PrecompileTypes( SCompiledTypesInfo *pRes, bool bGenerateCodeStructure, const vector<string> &files, const string &szDescriptorsPath );
bool GenerateTypes( const string &szTypesFilePath, SCompiledTypesInfo *pTypesInfo );
bool GenerateCode( list<string> *pFileTitles, const string &szSourceCodePath, SCompiledTypesInfo *pTypesInfo );
bool CopySourceCode( const list<string> &filetitles, const string &szSrcPath, const string &szDstPath );

}
}

