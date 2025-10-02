#pragma once

template <class Type>
struct SRingNode
{
	Type sData;
	SRingNode<Type> *pPrev;
	SRingNode<Type> *pNext;

	SRingNode( const Type &_sData ): sData( _sData ) {}
};

template <class Type, class TNode>
struct SRingIterator
{
	typedef SRingIterator<Type, TNode> TSelf;

	TNode* pNode;

	SRingIterator(): pNode( 0 ) {}
	SRingIterator( TNode* pStart ): pNode( pStart ) {} 
	SRingIterator( const TSelf &sIter ): pNode( sIter.pNode ) {}
		
	TSelf& operator++()
	{ 
    pNode = pNode->pNext;
    return *this;
  }
  TSelf operator++(int)
	{ 
    TSelf sTmp = *this;
    pNode = pNode->pNext;
    return sTmp;
  }
  TSelf& operator--()
	{ 
    pNode = pNode->pPrev;
    return *this;
  }
  TSelf operator--(int)
	{ 
    TSelf sTmp = *this;
    pNode = pNode->pPrev;
    return sTmp;
  }

  Type& operator*() const { return pNode->sData; }
	Type* operator->() const { return &(operator*()); }
		
  bool operator==( const TSelf &sIn ) const { return pNode == sIn.pNode; }
  bool operator!=( const TSelf &sIn ) const { return pNode != sIn.pNode; }
	
	TSelf GetPrev() const { return TSelf( pNode->pPrev ); }
	TSelf GetNext() const { return TSelf( pNode->pNext ); }
};

template <class Type>
class CRing
{
protected:
	int nCount;
	SRingNode<Type>* pList;	

public:
	typedef SRingNode<Type> node;
	typedef SRingIterator<Type, SRingNode<Type> > iterator;
	typedef SRingIterator<const Type, const SRingNode<Type> > const_iterator;
	
	CRing(): nCount( 0 ), pList( 0 ) {}
	~CRing()
	{
		clear();
	}

	void clear()
	{
		for ( int nTemp = 0; nTemp < nCount; nTemp++ )
		{
			node *pTmp = pList;
			pList = pList->pNext;
			delete pTmp;
		}
		pList = 0;
		nCount = 0;
	}

	iterator add( const Type &sData = Type() )
	{
		nCount++;
		node *pNew = new node( sData );

		if ( pList == 0 )
		{
			pList = pNew->pPrev = pNew->pNext = pNew;
		}
		else
		{
			pNew->pNext = pList;
			pNew->pPrev = pList->pPrev;
			pList->pPrev->pNext = pNew;
			pList->pPrev = pNew;
		}

		return iterator( pNew );
	}
	iterator insert( const iterator &iPos, const Type &sData = Type() )
	{
		nCount++;
		node *pNew = new node( sData );

		pNew->pNext = iPos.pNode->pNext;
		pNew->pPrev = iPos.pNode;
		iPos.pNode->pNext->pPrev = pNew;
		iPos.pNode->pNext = pNew;

		return iterator( pNew );
	}

	CRing<Type>& operator=( const CRing<Type> &ringList )
	{
		if ( this != &ringList)
		{
			clear();

			CRing<Type>::const_iterator iTemp = ringList.begin();
			for( int nTemp = 0; nTemp < ringList.size(); nTemp++, iTemp++ )
			{
				add( *iTemp );
			}
		}
		return *this;
	}

	int size() const { return nCount; }
	iterator begin() { return iterator( pList ); }
	const_iterator begin() const { return const_iterator( pList ); }
};


