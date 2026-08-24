#pragma once

#include <cstdint>

namespace NCache
{
typedef unsigned int MRU_TYPE;
const int MRU_LAST = 0xffffffff;
const int N_START_RU = 10;

struct SStats
{
	int nFree, nUsed, nBlocks;
	MRU_TYPE nEldestEntry;
	MRU_TYPE nCurrentRU;
	bool bThrashing;

	SStats() { nEldestEntry = MRU_LAST; nUsed = 0; nFree = 0; nBlocks = 0; bThrashing = false; }
};

template<class T>
class CShortPtrAllocator
{
public:
	typedef uint32_t pointer;
	enum EPointer
	{
		NIL = 0xffffffff
	};
private:
	ZDATA
	std::vector<T> buffer;
	pointer nFree, nLast;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&buffer); f.Add(3,&nFree); f.Add(4,&nLast); return 0; }

	CShortPtrAllocator(): nFree(NIL), nLast(0) {}
	T& operator()( pointer p ) { return buffer[p]; }
	pointer alloc() 
	{ 
		if ( nFree != NIL )
		{
			pointer nRes = nFree;
			nFree = buffer[nFree].pNext;
			return nRes;
		}
		//ASSERT( nLast < 0xff00 );
		if ( nLast == buffer.size() )
			buffer.resize( buffer.size() * 2 + 16 );
		return nLast++;
	}
	void free( pointer p )
	{
		ASSERT( !IsValid( buffer[p].pUser ) );
		buffer[p].pNext = nFree;
		nFree = p;
		//delete p;
/*		Node( p ).pUser = 0;
		Node( p ).pNext = freePtr;
		freePtr = p;*/
	}
};


template<template<class T> class TAlloc, class TElement, class TUser>
class CGatheringCache;

template<template<class T> class TAlloc, class TElement, class TUser>
class CGatherElementBase : virtual public CObjectBase
{
protected:
	// no copies are allowed!
	typedef CGatheringCache<TAlloc,TElement,TUser> CTracker;
private:
	ZDATA
	typename CTracker::pointer p;
	CPtr<CTracker> pTracker;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&p); f.Add(3,&pTracker); return 0; }
protected:
	~CGatherElementBase()
	{
		if ( IsValid( pTracker ) )
		{
			pTracker->Free( p );
			pTracker = 0;
		}
	}
	CTracker* GetTracker() const { return pTracker; }
public:
	virtual bool Touch()
	{
		if ( IsValid( pTracker ) )
		{
			ASSERT( p != CTracker::Alloc::NIL );
			return pTracker->Touch( p );
		}
		return false;
	}
	MRU_TYPE GetMRU() const
	{
		if ( pTracker.IsValid() )
			return pTracker->GetMRU( p );
		return 0;
	}
	friend class CGatheringCache<TAlloc,TElement,TUser>;
};

template<template<class T> class TAlloc, class TElement, class TUser>
class CGatheringCache: public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CGatheringCache);
	struct SElement;
	typedef TAlloc<SElement> Alloc;
	typedef typename Alloc::pointer pointer;
	struct SElement
	{
		ZDATA
		TElement elem;
		pointer pNextPeer;
		pointer pNext, pPrev; // RU list
		pointer pUp, pDown;
		MRU_TYPE nRU;
		CMObj<TUser> pUser;
		ZEND int operator&( CStructureSaver &f ) { f.Add(2,&elem); f.Add(3,&pNextPeer); f.Add(4,&pNext); f.Add(5,&pPrev); f.Add(6,&pUp); f.Add(7,&pDown); f.Add(8,&nRU); f.Add(9,&pUser); return 0; }
	};
	ZDATA
	Alloc alloc;
	MRU_TYPE nCurrentRU, nLatestAllocRU;
	bool bThrashing;
	std::vector<pointer> nodes;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&alloc); f.Add(3,&nCurrentRU); f.Add(4,&nLatestAllocRU); f.Add(5,&bThrashing); f.Add(6,&nodes); return 0; }
private:

	void EraseNode( pointer pNode )
	{
		if ( pNode == Alloc::NIL )
			return;
		Unlink( pNode );
		MakeFreeLeaf( pNode );
		alloc.free( pNode );
	}
	// erase all children & user objects tied to node pNode
	void MakeFreeLeaf( pointer pNode )
	{
		SElement &node = alloc( pNode );
		pointer p1 = node.pDown;
		node.nRU = 0;
		if ( p1 != Alloc::NIL )
		{
			ASSERT( !IsValid( node.pUser ) );
			// wipe out children
			for(;;)
			{
				pointer pHold = p1;
				p1 = alloc(p1).pNextPeer;
				EraseNode( pHold );
				if ( p1 == Alloc::NIL )
					break;
			}
			alloc(pNode).pDown = Alloc::NIL;
		}
		else
		{
			if ( IsValid( node.pUser ) )
				node.pUser->pTracker = 0; // prevent destructor from wasting stuff up
			node.pUser = 0;
		}
	}
	void Link( pointer pBefore, pointer pThis )
	{
		SElement &before = alloc( pBefore );
		SElement &subj = alloc( pThis );
		ASSERT( alloc( before.pNext ).pPrev == pBefore );
		ASSERT( alloc( before.pPrev ).pNext == pBefore );
		alloc( before.pNext ).pPrev = pThis;
		subj.pNext = before.pNext;
		before.pNext = pThis;
		subj.pPrev = pBefore;
	}
	void Unlink( pointer p )
	{
		SElement &subj = alloc( p );
		alloc( subj.pNext ).pPrev = subj.pPrev;
		alloc( subj.pPrev ).pNext = subj.pNext;
		subj.pNext = subj.pPrev = p;
	}
	struct SSplitter
	{
		CGatheringCache *pThis;
		pointer pLast, pUp;
		SSplitter( CGatheringCache *_pThis, pointer _pUp ) : pLast( Alloc::NIL ), pThis(_pThis), pUp(_pUp) {}
		void operator()( const TElement &_e, typename TElement::ESplitType t )
		{
			pointer p = pThis->alloc.alloc();
			SElement &e = pThis->alloc(p);
			e.elem = _e;
			e.pNextPeer = pLast;
			e.pUp = pUp;
			e.pDown = Alloc::NIL;
			e.nRU = 0;
			pThis->Link( pThis->nodes[e.elem.GetHash()], p );
			pLast = p;
			if ( t != TElement::NONE )
			{
				SSplitter rec( pThis, p );
				TElement src = pThis->alloc(p).elem;
				src.Split( t, rec );
				pThis->alloc(p).pDown = rec.pLast;
			}
		}
	};
	void Split( pointer p, typename TElement::ESplitType t )
	{
		SSplitter split( this, p );
		TElement e = alloc(p).elem;
		e.Split( t, split );
		alloc(p).pDown = split.pLast;
	}
	// subdivides p until reach elem size, returns pointer to node of _elem size
	pointer GetAllocNode( pointer p, const TElement &_elem )
	{
		if ( alloc(p).elem.SameSize( _elem ) )
			return p;
		TElement elem(_elem);
		typename TElement::ESplitType t = elem.Up();
		ASSERT( _elem.GetHash() < nodes.size() );
		pointer pHigher = GetAllocNode( p, elem );
		Split( pHigher, t );
		for ( pointer pRes = alloc(pHigher).pDown; pRes != Alloc::NIL; pRes = alloc(pRes).pNextPeer )
		{
			if ( alloc(pRes).elem.SameSize( _elem ) )
				return pRes;
		}
		ASSERT(0); // trouble happened - Split()/Up() for TElement are not reversible
		return p;
	}
	pointer AddRootNode( const TElement &elem )
	{
		pointer pNode = alloc.alloc();
		SElement &n = alloc( pNode );
		n.elem = elem;
		n.pNextPeer = Alloc::NIL;
		n.pUp = Alloc::NIL;
		n.pDown = Alloc::NIL;
		n.nRU = 0;
		Link( nodes[ elem.GetHash() ], pNode );
		return pNode;
	}

	// to be called from CGatherElementBase::Touch, return true if touched
	bool Touch( pointer _p )
	{
		pointer p = _p;
		for ( ; p != Alloc::NIL; p = alloc(p).pUp )
		{
			SElement &node = alloc( p );
			if ( node.nRU == nCurrentRU )
				return p != _p;
			Unlink( p );
			int nHash = node.elem.GetHash();
			ASSERT( nHash >= 0 && nHash < nodes.size() );
			Link( alloc( nodes[ nHash ] ).pPrev, p );
			node.nRU = nCurrentRU;
		}
		return true;
	}

	void PlaceNodeSorted( pointer pNode )
	{
		SElement &node = alloc( pNode );
		pointer pRoot = nodes[ node.elem.GetHash() ], pTest = node.pPrev;
		while ( pTest != pRoot && alloc( pTest ).nRU > node.nRU )
			pTest = alloc( pTest ).pPrev;
		Unlink( pNode );
		Link( pTest, pNode );
		/*
		{	
		// TEST
		SListNode *pRoot = &Node(  pNode->nSize  ), *pPrev = pRoot, *pCurrent;
		int nPrevTestCp = 0;
		for (;;)
		{
		pCurrent = pPrev->pNext;
		if ( pCurrent == pRoot )
		break;
		SNode *pRes = (SNode*)pCurrent;
		ASSERT( pRes->nRU >= nPrevTestCp );
		nPrevTestCp = pRes->nRU;
		pPrev = pCurrent;
		}
		}
		*/
	}
	void RecalcRU( pointer pNode )
	{
		if ( pNode == Alloc::NIL )
			return;
		SElement &node = alloc( pNode );
		MRU_TYPE bestRU = 0;
		for ( pointer pDown = node.pDown; pDown != Alloc::NIL; pDown = alloc( pDown ).pNextPeer )
			bestRU = (std::max)( bestRU, alloc( pDown ).nRU );
		if ( bestRU == node.nRU )
			return;
		node.nRU = bestRU;
		//ASSERT( bestRU != 0 );
		PlaceNodeSorted( pNode );
		RecalcRU( node.pUp );
	}
	void SplitOnUsable( pointer pNode )
	{
		typename TElement::ESplitType t = alloc( pNode ).elem.GetUsableSplit();
		if ( t != TElement::NONE )
			Split( pNode, t );
	}
	// to be called only from ~CCacheElement
	void Free( pointer pNode )
	{
		ASSERT( pNode != Alloc::NIL );
		ASSERT( alloc( pNode ).pDown == Alloc::NIL );
		SElement &node = alloc( pNode );
		// mark node as free
		node.nRU = 0;
		node.pUser = 0;
		// check other peers, if all are empty - free parent
		if ( node.pUp != Alloc::NIL )
		{
			ASSERT( !IsValid( alloc(node.pUp).pUser ) );
			bool bHasChildren = false;
			for ( pointer p = alloc(node.pUp).pDown; p != Alloc::NIL; p = alloc(p).pNextPeer )
			{
				SElement &e = alloc(p);
				if ( e.nRU != 0 || e.pDown != Alloc::NIL )
				{
					bHasChildren = true;
					break;
				}
			}
			if ( !bHasChildren )
			{
				pointer pUp = node.pUp;
				MakeFreeLeaf( pUp );
				Free( pUp );
				return;
			}
		}
		// place in front
		Unlink( pNode );
		Link( nodes[ alloc( pNode ).elem.GetHash() ], pNode );
		RecalcRU( node.pUp );
		//
		SplitOnUsable( pNode );
	}
public:
	struct SCachePlace
	{
		pointer pBest;
		TElement requestSize, resPlace;
		MRU_TYPE nMRU;
	};

	CGatheringCache( MRU_TYPE _nCurrentRU = N_START_RU ): nCurrentRU( _nCurrentRU ), nLatestAllocRU(0), bThrashing(false) {}
	~CGatheringCache() { Clear(); }
	void Clear()
	{
		for ( int k = nodes.size() - 1; k >= 0; --k )
		{
			for ( pointer p = alloc(nodes[k]).pNext; p != nodes[k]; p = alloc(p).pNext )
				MakeFreeLeaf( p );
		}
	}
	void AddRoot( TElement root )
	{
		int nPrevSize = nodes.size(), nHash = root.GetHash();
		if ( nHash + 1 > nPrevSize )
		{
			nodes.resize( nHash + 1 );
			for ( int k = nPrevSize; k < nHash + 1; ++k )
			{
				nodes[k] = alloc.alloc();
				SElement &node = alloc( nodes[k] );
				node.pNext = node.pPrev = nodes[k];
				node.nRU = 0;
			}
		}
		SplitOnUsable( AddRootNode( root ) );
	}
	bool IsThrashing() const { return bThrashing; }
	void CalcStats( SStats *pStats )
	{
		pStats->nCurrentRU = nCurrentRU;
		pStats->bThrashing |= bThrashing;
		for ( int i = 0; i < nodes.size(); ++i )
		{
			pointer pStart = nodes[i];
			pointer pRes = alloc( pStart ).pNext;
			while ( pRes != pStart )
			{
				SElement &n = alloc( pRes );
				if ( n.pDown == Alloc::NIL )
				{
					if ( IsValid( n.pUser ) )
					{
						pStats->nEldestEntry = (std::min)( pStats->nEldestEntry, n.nRU );
						pStats->nUsed += n.elem.GetSize();
						pStats->nBlocks++;
					}
					else
						pStats->nFree += n.elem.GetSize();
				}
				pRes = alloc( pRes ).pNext;
			}
		}
	}
	void AdvanceFrameCounter() 
	{ 
		bThrashing = nLatestAllocRU == nCurrentRU;
		++nCurrentRU;
		nLatestAllocRU = 0;
	}
	bool GetPlace( const TElement &_elem, SCachePlace *pRes )
	{
		if ( nodes.empty() )
			return false;
		// search for node with minimal RU
		TElement elem(_elem);
		pointer pBest = Alloc::NIL;
		MRU_TYPE nBestMRU = MRU_LAST;
		for(;;)
		{
			int n = elem.GetHash();
			if ( n >= nodes.size() )
				break;
			pointer pRes = alloc( nodes[n] ).pNext;
			if ( pRes != nodes[n] )
			{
				if ( alloc( pRes ).nRU < nBestMRU )
				{
					nBestMRU = alloc( pRes ).nRU;
					pBest = pRes;
				}
			}
			elem.Up();
		}
		//ASSERT( pBest != Alloc::NIL );
		if ( pBest == Alloc::NIL )
			return false;
		pRes->nMRU = nBestMRU;
		pRes->pBest = pBest;
		pRes->requestSize = _elem;
		return true;
	}
	MRU_TYPE GetCurrentRU() const { return nCurrentRU; }
	void PerformAlloc( TUser *pRes, SCachePlace *pPlace )
	{
		nLatestAllocRU = (std::max)( nLatestAllocRU, pPlace->nMRU );
		MakeFreeLeaf( pPlace->pBest );
		pointer pNode = GetAllocNode( pPlace->pBest, pPlace->requestSize );
		SElement &nRes = alloc( pNode );
		nRes.pUser = pRes;
		nRes.pUser->p = pNode;
		nRes.pUser->pTracker = this;
		ASSERT( nRes.nRU == 0 );
		pPlace->resPlace = nRes.elem;
		Touch( pNode );
	}
	MRU_TYPE GetMRU( pointer p ) { return alloc(p).nRU; }
	struct SStatePlace
	{
		TElement place;
		TUser *pUser;
		MRU_TYPE nMRU;

		SStatePlace() {}
		SStatePlace( const TElement &_e, TUser *_p, MRU_TYPE _nMRU ) : place(_e), pUser(_p), nMRU(_nMRU) {}
	};
	void GetState( std::vector<SStatePlace> *pState )
	{
		for ( int k = 0; k < nodes.size(); ++k )
		{
			for ( pointer p = alloc( nodes[k] ).pNext; p != nodes[k]; p = alloc( p ).pNext )
			{
				SElement &e = alloc(p);
				if ( e.pDown == Alloc::NIL )
				pState->push_back( SStatePlace( e.elem, e.pUser, e.nRU ) );
			}
		}
	}

	friend class CGatherElementBase<TAlloc, TElement, TUser>;
};

class CQuadTreeElement
{
public:
	enum ESplitType
	{
		NONE,
		QUAD,
		YBINARY,
		XBINARY,
		XSPLIT,
		YSPLIT
	};
	int nXSize, nYSize; // log2 of dimension, nXSize >= nYSize
	int nShiftX, nShiftY;

	int GetHash() const 
	{ 
		if ( nXSize > nYSize ) 
			return nXSize * (nXSize + 1) + nYSize * 2;
		else
			return nYSize * (nYSize + 1) + nXSize * 2 + 1;
	}
	ESplitType Up()
	{
		if ( nYSize < nXSize )
		{
			++nYSize;
			return YBINARY;
		}
		if ( nYSize > nXSize )
		{
			++nXSize;
			return XBINARY;
		}
		++nXSize;
		++nYSize;
		return QUAD;
	}
	ESplitType GetUsableSplit() const 
	{ 
		if ( nXSize > nYSize )
			return XSPLIT;
		if ( nXSize < nYSize )
			return YSPLIT;
		return NONE; 
	}
	bool SameSize( const CQuadTreeElement &a ) const
	{
		return nXSize == a.nXSize && nYSize == a.nYSize;
	}
	int GetSize() const { return 1<<(nXSize+nYSize); }
	template<class TOutput>
		void Split( ESplitType t, TOutput &out )
	{
		switch ( t )
		{
			case XSPLIT:
				{
					ESplitType tt = ( ( nXSize - 1 ) == nYSize ) ? NONE : XSPLIT;
					out( CQuadTreeElement( nXSize - 1, nYSize, nShiftX, nShiftY ), tt );
					out( CQuadTreeElement( nXSize - 1, nYSize, nShiftX + (1<<(nXSize-1)), nShiftY ), tt );
				}
				break;
			case YSPLIT:
				{
					ESplitType tt = ( ( nYSize - 1 ) == nXSize ) ? NONE : YSPLIT;
					out( CQuadTreeElement( nXSize, nYSize - 1, nShiftX, nShiftY ), tt );
					out( CQuadTreeElement( nXSize, nYSize - 1, nShiftX, nShiftY + (1<<(nYSize-1)) ), tt );
				}
				break;
			case YBINARY:
				out( CQuadTreeElement( nXSize, nYSize - 1, nShiftX, nShiftY + (1<<(nYSize-1)) ), XSPLIT );
				out( CQuadTreeElement( nXSize, nYSize - 1, nShiftX, nShiftY ), NONE );
				break;
			case XBINARY:
				out( CQuadTreeElement( nXSize - 1, nYSize, nShiftX + (1<<(nXSize-1)), nShiftY ), YSPLIT );
				out( CQuadTreeElement( nXSize - 1, nYSize, nShiftX, nShiftY ), NONE );
				break;
			case QUAD:
				ASSERT( nXSize == nYSize );
				out( CQuadTreeElement( nXSize - 1, nYSize - 1, nShiftX, nShiftY ), NONE );
				out( CQuadTreeElement( nXSize - 1, nYSize - 1, nShiftX, nShiftY + (1<<(nYSize-1)) ), NONE );
				out( CQuadTreeElement( nXSize - 1, nYSize - 1, nShiftX + (1<<(nYSize-1)), nShiftY ), NONE );
				out( CQuadTreeElement( nXSize - 1, nYSize - 1, nShiftX + (1<<(nYSize-1)), nShiftY + (1<<(nYSize-1)) ), NONE );
				break;
		}
	}
	CQuadTreeElement() {}
	CQuadTreeElement( int _nXSize, int _nYSize, int _nShiftX, int _nShiftY )
		: nXSize(_nXSize), nYSize(_nYSize), nShiftX(_nShiftX), nShiftY(_nShiftY) {}
};

const int N_MAX_FIB = 64;
extern int nFibbonachiSeries[N_MAX_FIB];
inline int fib( int nDepth ) { ASSERT( nDepth < N_MAX_FIB ); return nFibbonachiSeries[nDepth]; }
inline int GetMajorFib( int _n )
{
	int k = 0;
	while ( fib( k ) <_n )
		++k;
	return k;
}

class CFibElement
{
public:
	enum ESplitType
	{
		NONE
	};
	int nSize;
	int nShift;

	int GetHash() const { return nSize; }
	ESplitType Up()
	{
		++nSize;
		return NONE;
	}
	ESplitType GetUsableSplit() const { return NONE; }
	bool SameSize( const CFibElement &a ) const
	{
		return nSize == a.nSize;
	}
	int GetSize() const { return fib( nSize ); }
	template<class TOutput>
		void Split( ESplitType t, TOutput &out )
	{
		ASSERT( nSize > 0 );
		out( CFibElement( nSize - 1, nShift ), NONE );
		out( CFibElement( (std::max)( 0, nSize - 2 ), nShift + fib( nSize - 1 ) ), NONE );
	}
	CFibElement() {}
	CFibElement( int _nSize, int _nShift )
		: nSize(_nSize), nShift(_nShift) {}
};

}

