#pragma once

#include "libdb_export.h"

namespace NCodeGen
{
	class CCodeStructure;

	// exported for dbcodegen, which writes the generated sources with it
	LIBDB_EXPORT void GenerateCode( CCodeStructure *pCodeStructure, const std::string &szRootDir );
}


