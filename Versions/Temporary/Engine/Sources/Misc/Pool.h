#pragma once

#include <boost/config.hpp>

template<class T, int N_BLOCK_SIZE = 4096>
class CPool
{
	struct SBlock
	{
		T data[N_BLOCK_SIZE];
	};
	std::list<SBlock> data;
	T *pCurrent, *pLast;

	CPool( const CPool &a ) { ASSERT(0); }
	void operator=( const CPool &a ) { ASSERT(0); }
	int operator&( IBinSaver &f ) { ASSERT(0); return 0; }
	BOOST_NOINLINE void AllocNewBlock()
	{
		SBlock *pBlock = &data.emplace_back();
		pCurrent = &pBlock->data[-1];
		pLast = &pBlock->data[N_BLOCK_SIZE - 1];
	}
public:
	struct SIterator
	{
		T *p, *pBlockStart;
		typename std::list<SBlock>::iterator i;
		CPool *pPool;

		SIterator(): pPool(0), p(0), pBlockStart(0) {}
		SIterator( CPool *_pPool ) { Assign( _pPool ); }
		void Assign( CPool *_pPool )
		{
			pPool = _pPool;
			i = pPool->data.end();
			if ( !pPool->data.empty() )
			{
				--i;
				pBlockStart = &i->data[-1];
			}
			else
				pBlockStart = 0;
			p = pPool->pCurrent;
		}
		void operator--() 
		{
			ASSERT(pPool);
			--p; 
			if ( p == pBlockStart ) 
			{
				if ( i == pPool->data.begin() )
				{
					p = 0;
					pBlockStart = 0;
					return;
				}
				--i;
				p = &i->data[N_BLOCK_SIZE - 1];
				pBlockStart = &i->data[-1];
			}
		}
		bool operator==( const SIterator &a ) const { return p == a.p; }
		bool operator!=( const SIterator &a ) const { return p != a.p; }
	};
	CPool(): pCurrent(0), pLast(0) {}
	T* Alloc()
	{
		if ( pCurrent == pLast )
			AllocNewBlock();
		return ++pCurrent;
	}
	void Clear() { data.clear(); pCurrent = pLast = 0; }
};


