#include "stdafx.h"

#include "HashMapConvertor.h"
namespace NHashMapConvertor
{

void ConvertNumber( std::unordered_map<std::string,int> *pHashMap, const std::string &szName, int *pValue, const bool bRead )
{
	if ( bRead )
	{
		std::unordered_map<std::string,int>::const_iterator it = pHashMap->find( szName );
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

void ConvertVector( std::unordered_map<std::string,int> *pHashMap, const std::string &szPrefix, std::vector<int> *pVector, const bool bRead )
{
	if ( bRead )
	{
		pVector->clear();
		std::string szName;
		szName.reserve( szPrefix.size() + 3 );
		szName = szPrefix + "0";
		int i = 0;
		std::unordered_map<std::string,int>::const_iterator it = pHashMap->find( szName );
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
		std::string szName;
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
