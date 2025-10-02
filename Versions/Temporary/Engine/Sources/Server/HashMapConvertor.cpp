#include "stdafx.h"

#include "HashMapConvertor.h"
namespace NHashMapConvertor
{

void ConvertNumber( hash_map<string,int> *pHashMap, const string &szName, int *pValue, const bool bRead )
{
	if ( bRead )
	{
		hash_map<string,int>::const_iterator it = pHashMap->find( szName );
		if ( it != pHashMap->end() )
			*pValue = it->second;
		else
			*pValue = 0;
	}
	else
	{
		(*pHashMap)[szName] = *pValue;
	}
}

void ConvertVector( hash_map<string,int> *pHashMap, const string &szPrefix, vector<int> *pVector, const bool bRead )
{
	if ( bRead )
	{
		pVector->clear();
		string szName;
		szName.reserve( szPrefix.size() + 3 );
		szName = szPrefix + "0";
		int i = 0;
		hash_map<string,int>::const_iterator it = pHashMap->find( szName );
		while ( it != pHashMap->end() )
		{
			pVector->push_back( it->second );
			++i;
			szName.erase( szPrefix.size(), szName.size() - szPrefix.size() );
			szName += StrFmt( "%d", i );
			it = pHashMap->find( szName );
		}
	}
	else
	{
		string szName;
		szName.reserve( szPrefix.size() + 3 );
		szName = szPrefix;
		for ( int i = 0; i < pVector->size(); ++i )
		{
			szName += StrFmt( "%d", i );
			(*pHashMap)[szName] = (*pVector)[i];
			szName.erase( szPrefix.size(), szName.size() - szPrefix.size() );
		}
	}
}

}
