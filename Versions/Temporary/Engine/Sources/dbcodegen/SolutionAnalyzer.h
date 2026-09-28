#pragma once

namespace NSlnAnalyzer
{
	void GetProjectsOfSln( const std::string &szSlnName, const std::string &szBasePath, std::vector<std::string> *pProjects );
	void GetTypesDescriptorsOfSln( const std::string &szSlnName, const std::string &szBasePath, std::vector<std::string> *pFiles );
}


