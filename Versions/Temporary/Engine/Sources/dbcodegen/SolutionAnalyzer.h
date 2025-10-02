#pragma once

namespace NSlnAnalyzer
{
	void GetProjectsOfSln( const string &szSlnName, const string &szBasePath, vector<string> *pProjects );
	void GetTypesDescriptorsOfSln( const string &szSlnName, const string &szBasePath, vector<string> *pFiles );
}


