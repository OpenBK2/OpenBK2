#pragma once


template<class TContainer, class TElement> 
void InsertHashSetElement( TContainer *pSet, const TElement &rElement )
{
	if ( pSet )
	{
		( *pSet )[rElement] = 0;
	}
}



template<class TContainer> 
void InsertIndexToIndicesList( TContainer *pList, int nIndex )
{
	for ( TContainer::iterator itElement = pList->begin(); itElement != pList->end(); ++itElement )
	{
		if ( ( *itElement ) >= nIndex )
		{
			++( *itElement );
		}
	}
	pList->push_back( nIndex );
}


template<class TContainer, class TElement> 
void InsertIndexToIndicesList( TContainer *pList, int nIndex )
{
	for ( TContainer::iterator itElement = pList->begin(); itElement != pList->end(); ++itElement )
	{
		if ( itElement->nIndex >= nIndex )
		{
			++( itElement->nIndex );
		}
	}
	pList->push_back( TElement( nIndex ) );
}


template<class TContainer, class TElement> 
void RemoveIndexToIndicesList( TContainer *pList, int nIndex )
{
	for ( TContainer::iterator itElement = pList->begin(); itElement != pList->end(); ++itElement )
	{
		if ( itElement->nIndex > nIndex )
		{
			--( itElement->nIndex );
			++itElement;
		}
		else if ( itElement->nIndex == nIndex )
		{
			itElement = pList->erase( itElement );
		}
		else
		{
			++itElement;
		}
	}
}


template<class TContainer, class TContainerIterator> 
TContainerIterator FindIndex( TContainer &rList, int nIndex )
{
	for ( TContainerIterator itElement = rList.begin(); itElement != rList.end(); ++itElement )
	{
		if ( itElement->nIndex == nIndex )
		{
			return itElement;
		}
	}
	return rList.end();
}
/**

template<class TContainer, class TElement> 
void AddListElement( TContainer *pList, const TElement &rElement )
{
	pList->push_back( rElement );
}


template<class TContainer, class TElement> 
void AddUniqueElementToList( TContainer *pList, const TElement &rElement )
{
	TContainer::iterator posExistingElement = find( pList->begin(), pList->end(), rElement );
	{
		if ( posExistingElement == pList->end() )
		{
			pList->push_back( rElement );
		}
	}
}


template<class TContainer, class TElement> 
bool IsElementExistsInList( const TContainer &rList, const TElement &rElement )
{
	return ( find( rList.begin(), rList.end(), rElement ) != rList.end() );
}

/**/

