#pragma once


namespace NHashMapConvertor
{
	void ConvertNumber( std::unordered_map<std::string,int> *pHashMap, const std::string &szName, int *pValue, const bool bRead );
	void ConvertVector( std::unordered_map<std::string,int> *pHashMap, const std::string &szPrefix, std::vector<int> *pValue, const bool bRead );
};


