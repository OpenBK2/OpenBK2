#if !defined(__COMMON_TOOLS__UNIQUE_LIST__)
#define __COMMON_TOOLS__UNIQUE_LIST__
#pragma once

#define UNIQUE_LIST_INSERT_FRONT true
#define UNIQUE_LIST_INSERT_BACK false

template<class TList, class TElement>
class CUniqueList
{
	typedef hash_map<TElement, UINT> CElementMap;
	//
	CElementMap elementMap;
	TList elementList;

public:
	bool Insert( const TElement &rElement, bool bFront )
	{
		if ( elementMap.find( rElement ) == elementMap.end() )
		{
			elementMap[rElement] = 0;
			if ( bFront )
			{
				elementList.insert( elementList.begin(), rElement );
			}
			else
			{
				elementList.insert( elementList.end(), rElement );
			}
			return true;
		}
		return false;
	}
	//
	void Remove( const TElement &rElement )
	{
		{
			CElementMap::iterator posElement = elementMap.find( rElement );
			if ( posElement != elementMap.end() )
			{
				elementMap.erase( posElement );
			}
		}
		{
			TList::iterator posElement = elementList.find( rElement );
			if ( posElement != elementList.end() )
			{
				elementList.erase( posElement );
			}
		}
	}
	//
	void Clear()
	{
		elementMap.clear();
		elementList.clear();
	}
	//
	bool IsExists( const TElement &rElement ) const
	{
		return ( elementMap.find( rElement ) != elementMap.end() );
	}
	//
	const CElementMap& GetMap() const { return elementMap; }
	const TList& GetList() const { return elementList; }
};

#endif // #if !defined(__COMMON_TOOLS__UNIQUE_LIST__)
