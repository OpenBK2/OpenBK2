#pragma once


namespace NHashMapConvertor
{
	void ConvertNumber( hash_map<string,int> *pHashMap, const string &szName, int *pValue, const bool bRead );
	void ConvertVector( hash_map<string,int> *pHashMap, const string &szPrefix, vector<int> *pValue, const bool bRead );
};


