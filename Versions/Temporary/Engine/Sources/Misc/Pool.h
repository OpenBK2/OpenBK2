#pragma once

#include <boost/config.hpp>

template <class T, int N_BLOCK_SIZE = 4096>
class CPool
{
	struct SBlock
	{
		T data[N_BLOCK_SIZE];
	};

	std::list<SBlock> data;
	T *pCurrent, *pLast;

	CPool( const CPool &a )
	{
		ASSERT( 0 );
	}

	void operator=( const CPool &a )
	{
		ASSERT( 0 );
	}

	int operator&( IBinSaver &f )
	{
		ASSERT( 0 );
		return 0;
	}

	// Returns the block's first element, already taken. pCurrent was set to
	// &data[-1] here so that Alloc's ++pCurrent would land on data[0], but a
	// pointer before the start of an array is undefined however carefully it is
	// left alone afterwards. Handing out data[0] from here keeps pCurrent on the
	// element last given out, which is what SIterator reads it as.
	BOOST_NOINLINE T *AllocNewBlock()
	{
		SBlock *pBlock = &data.emplace_back();
		pCurrent = pBlock->data;
		pLast = &pBlock->data[N_BLOCK_SIZE - 1];
		return pCurrent;
	}

public:
	struct SIterator
	{
		T *p, *pBlockStart;
		typename std::list<SBlock>::iterator i;
		CPool *pPool;

		SIterator()
			: pPool( 0 )
			, p( 0 )
			, pBlockStart( 0 )
		{
		}

		SIterator( CPool *_pPool )
		{
			Assign( _pPool );
		}

		void Assign( CPool *_pPool )
		{
			pPool = _pPool;
			i = pPool->data.end();
			if ( !pPool->data.empty() )
			{
				--i;
				pBlockStart = i->data;
			}
			else
			{
				pBlockStart = 0;
			}
			p = pPool->pCurrent;
		}

		// pBlockStart is the first element of the block p is in, so the test has
		// to come before the step: stepping first and comparing against
		// &data[-1], as this did, forms a pointer before the array.
		void operator--()
		{
			ASSERT( pPool );
			if ( p != pBlockStart )
			{
				--p;
				return;
			}
			if ( i == pPool->data.begin() )
			{
				p = 0;
				pBlockStart = 0;
				return;
			}
			--i;
			p = &i->data[N_BLOCK_SIZE - 1];
			pBlockStart = i->data;
		}

		bool operator==( const SIterator &a ) const
		{
			return p == a.p;
		}

		bool operator!=( const SIterator &a ) const
		{
			return p != a.p;
		}
	};

	CPool()
		: pCurrent( 0 )
		, pLast( 0 )
	{
	}

	T *Alloc()
	{
		if ( pCurrent == pLast )
		{
			return AllocNewBlock();
		}
		return ++pCurrent;
	}

	void Clear()
	{
		data.clear();
		pCurrent = pLast = 0;
	}
};
