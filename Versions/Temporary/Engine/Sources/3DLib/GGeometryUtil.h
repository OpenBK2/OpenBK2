#pragma once

#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

namespace NGScene
{

template <int N_SIZE>
struct SVxCache
{
	uint16_t nData[N_SIZE];
	int nPos;

	SVxCache()
	{
		memset( nData, 0xffff, sizeof( nData ) );
		nPos = 0;
	}

	void Push( uint16_t n )
	{
		if ( IsIn( n ) )
		{
			return;
		}
		nData[( ++nPos ) % N_SIZE] = n;
	}

	bool IsIn( uint16_t n ) const
	{
		for ( int k = 0; k < N_SIZE; ++k )
		{
			if ( nData[k] == n )
			{
				return true;
			}
		}
		return false;
	}

	int GetPos( uint16_t n ) const
	{
		for ( int k = 0; k < N_SIZE; ++k )
		{
			if ( nData[k] == n )
			{
				return ( nPos - k + N_SIZE ) % N_SIZE;
			}
		}
		return -1;
	}
};

//const int N_VX_TRI_CACHE_SIZE = 10;//24;
class CTriVertexCacheOptimizer
{
private:
	std::vector<int> triPerVertex;
	std::vector<int> tpvIndex;
	std::vector<int> freeLinks;
	std::vector<char> isUsed;
	std::vector<int> cachePos;
	std::vector<int> cache;
	std::vector<int> outQueue, outPrevPos;
	int nVertices;
	bool bEvictedReferencedVertex;
	int nVCacheSize;
	std::vector<int> lastTry;
	int nTryCount;
	int nOutQueueSize, nOutPrevPosSize;

	// Picking a triangle to start a new strip from, which is what SearchBest
	// falls back to when nothing adjacent to the cache will do. It wants the
	// unused triangle whose vertices have the fewest free links left, the lowest
	// index breaking a tie, and it used to find that by scanning every triangle
	// in the mesh. SearchBest runs once per output triangle, so that was
	// quadratic, and on a model made of many small pieces, which is what a
	// vehicle is, the fallback fires once per piece rather than once.
	//
	// A lazily deleted min-heap of (key, triangle) gives the same answer. Pairs
	// compare lexicographically, so the smallest is the lowest key and then the
	// lowest index, exactly the rule the scan applied. A triangle's key only ever
	// falls, since a free link is a triangle not yet emitted, so when it changes
	// the new pair is pushed and the old one is left to be skipped on the way
	// past: an entry is stale when its triangle has been used or when its key is
	// no longer the one startKey records.
	//
	// Keys are republished only for the triangles that share a vertex with the
	// one just emitted, since no other key can have moved.
	//
	// The heap is not built until the fallback has fired often enough to be worth
	// it, because keeping it current costs something on every emitted triangle
	// whether or not the fallback ever asks. On a connected mesh it fires about
	// once, so the scan is cheaper than the bookkeeping and measurably so: a
	// 7,200 triangle grid was 19 per cent slower with the heap maintained from the
	// start. On a mesh of many small pieces, which is what a vehicle model is, it
	// fires once per piece and the scan is quadratic. Counting the calls and
	// switching over picks the right one without having to know the shape of the
	// mesh in advance, and bounds the scan's total cost at the threshold times the
	// triangle count.
	typedef std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>>
			CStartQueue;

	//! How many times the fallback may scan before the heap is built instead.
	//! Scanning costs one pass over the mesh each time and the heap costs about
	//! the same to build and to keep current for the rest of the run, so the
	//! crossover is low; this is comfortably past it in both directions.
	static const int START_QUEUE_THRESHOLD = 32;

	std::vector<int> startKey;
	CStartQueue startQueue;
	int nStartSearches;
	bool bUseStartQueue;

	//! The key the fallback orders triangles by: the largest number of free links
	//! on any of the triangle's three vertices.
	int StartKey( const std::vector<STriangle> &tris, int nTri ) const
	{
		const STriangle &q = tris[nTri];
		int nFreeLinks = 0;
		nFreeLinks = (std::max)( nFreeLinks, freeLinks[q.i1] );
		nFreeLinks = (std::max)( nFreeLinks, freeLinks[q.i2] );
		nFreeLinks = (std::max)( nFreeLinks, freeLinks[q.i3] );
		return nFreeLinks;
	}

	void OnTriangleUsed( const std::vector<STriangle> &tris, int nUsed );
	int ScanBestStart( const std::vector<STriangle> &tris ) const;
	void BuildStartQueue( const std::vector<STriangle> &tris );
	int PopBestStart();

	void CountVertices( const std::vector<STriangle> &tris );
	void Init( const std::vector<STriangle> &tris );

	struct SBestSearch
	{
		int nBestIdx, nBestCached, nBestFreeLinks, nBestDistance;
		CTriVertexCacheOptimizer *pOptimizer;
		const std::vector<STriangle> &tris;

		SBestSearch( CTriVertexCacheOptimizer *pO, const std::vector<STriangle> &_tris )
			: nBestIdx( -1 )
			, nBestCached( -1 )
			, pOptimizer( pO )
			, tris( _tris )
		{
		}

		bool GetL( int *pnResDistance, int n, int nVertex, int nVCacheSize )
		{
			if ( n <= *pnResDistance )
			{
				return false;
			}
			if ( n + 1 >= nVCacheSize )
			{
				return false;
			}
			if ( n + pOptimizer->freeLinks[nVertex] * 3 + 1 < nVCacheSize )
			{
				*pnResDistance = n;
				return true;
			}
			int nFL = pOptimizer->CountNotCachedFL( tris, nVertex );
			ASSERT( nFL <= pOptimizer->freeLinks[nVertex] * 3 );
			if ( n + nFL + 1 >= nVCacheSize )
			{
				return false;
			}
			*pnResDistance = n;
			return true; // - 0 * fl;// - 3 * fl;
		}

		bool HasFound() const
		{
			return nBestCached == 3;
		}

		void Try( int nTri )
		{
			const STriangle &q = tris[nTri];
			int nCached = 0;
			int nDistance = 0;
			int nFreeLinks = 0;
			//int nQDistance;
			int nVCacheSize = pOptimizer->nVCacheSize;

			nCached += pOptimizer->OutputVertex( q.i1 );
			nCached += pOptimizer->OutputVertex( q.i2 );
			nCached += pOptimizer->OutputVertex( q.i3 );

			if ( nCached == 3 )
			{
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				nBestIdx = nTri;
				nBestCached = nCached;
				return;
			}
			if ( nCached < nBestCached )
			{
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				return;
			}
			if ( nCached == 2 )
			{
				nCached = 1;
			}

			int nCachePos = pOptimizer->cache.size() - 1;
			int nQDistance1, nQDistance2, nQDistance3;
			nQDistance1 = nCachePos - pOptimizer->cachePos[q.i1];
			nQDistance2 = nCachePos - pOptimizer->cachePos[q.i2];
			nQDistance3 = nCachePos - pOptimizer->cachePos[q.i3];
			int nDistanceGuess = 0;
			if ( nQDistance1 + 1 < nVCacheSize )
			{
				nDistanceGuess = nQDistance1;
			}
			if ( nQDistance2 + 1 < nVCacheSize )
			{
				nDistanceGuess = (std::max)( nDistanceGuess, nQDistance2 );
			}
			if ( nQDistance3 + 1 < nVCacheSize )
			{
				nDistanceGuess = (std::max)( nDistanceGuess, nQDistance3 );
			}
			if ( nCached == nBestCached && nDistanceGuess < nBestDistance )
			{
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				return;
			}

			ASSERT( !pOptimizer->isUsed[nTri] );
			pOptimizer->isUsed[nTri] = 1;

			GetL( &nDistance, nQDistance1, q.i1, nVCacheSize );
			GetL( &nDistance, nQDistance2, q.i2, nVCacheSize );
			GetL( &nDistance, nQDistance3, q.i3, nVCacheSize );

			nFreeLinks = (std::max)( nFreeLinks, pOptimizer->freeLinks[q.i1] );
			nFreeLinks = (std::max)( nFreeLinks, pOptimizer->freeLinks[q.i2] );
			nFreeLinks = (std::max)( nFreeLinks, pOptimizer->freeLinks[q.i3] );

			pOptimizer->ReverseVertex();
			pOptimizer->ReverseVertex();
			pOptimizer->ReverseVertex();
			pOptimizer->isUsed[nTri] = 0;

			if ( nCached > nBestCached )
			{
				nBestIdx = nTri;
				nBestCached = nCached;
				nBestFreeLinks = nFreeLinks;
				nBestDistance = nDistance;
			}
			else if ( nCached == nBestCached )
			{
				if ( nDistance > nBestDistance )
				{
					nBestIdx = nTri;
					nBestFreeLinks = nFreeLinks;
					nBestDistance = nDistance;
				}
				else if ( nDistance == nBestDistance )
				{
					if ( nFreeLinks < nBestFreeLinks )
					{
						nBestFreeLinks = nFreeLinks;
						nBestIdx = nTri;
					}
				}
			}
		}
	};

	int SearchBest( const std::vector<STriangle> &tris );
	void MeasureEfficiency( const std::vector<STriangle> &tris );
	bool OutputVertex( int n );
	void ReverseVertex();
	int CountNotCachedFL( const std::vector<STriangle> &tris, int nVertex );
	void OptimizeVertexOrder( std::vector<STriangle> &tris, int *pnResVerts, std::vector<uint16_t> *pVertexReorder );

public:
	void
	Optimize( std::vector<STriangle> *pTris, std::vector<uint16_t> *pVertexReorder, int *pnResVerts, int nVCacheSize );
};

struct SEdgeInfo
{
	int nU, nV, nXSize, nYSize;
};

inline bool operator==( const SEdgeInfo &a, const SEdgeInfo &b )
{
	return memcmp( &a, &b, sizeof( a ) ) == 0;
}

struct SEdgeInfoHash
{
	int operator()( const SEdgeInfo &e ) const
	{
		return e.nU ^ e.nV ^ e.nXSize ^ e.nYSize;
	}
};

} // namespace NGScene
