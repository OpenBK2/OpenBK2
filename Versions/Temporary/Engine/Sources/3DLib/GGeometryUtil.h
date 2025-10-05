#pragma once

namespace NGScene
{

template<int N_SIZE>
struct SVxCache
{
	WORD nData[N_SIZE];
	int nPos;

	SVxCache() { memset( nData, 0xffff, sizeof(nData) ); nPos = 0; }
	void Push( WORD n ) { if ( IsIn( n ) ) return; nData[(++nPos)%N_SIZE] = n; }
	bool IsIn( WORD n ) const { for ( int k = 0; k < N_SIZE; ++k ) if ( nData[k] == n ) return true; return false; }
	int GetPos( WORD n ) const { for ( int k = 0; k < N_SIZE; ++k ) if ( nData[k] == n ) return (nPos - k + N_SIZE ) % N_SIZE; return -1; }
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

	void CountVertices( const std::vector<STriangle> &tris );
	void Init( const std::vector<STriangle> &tris );
	struct SStartSearch
	{
		int nBestIdx, nBestFreeLinks;
		CTriVertexCacheOptimizer *pOptimizer;
		const std::vector<STriangle> &tris;

		SStartSearch( CTriVertexCacheOptimizer *pO, const std::vector<STriangle> &_tris ) : nBestIdx(-1), nBestFreeLinks(1000000), pOptimizer(pO), tris(_tris) {}
		void Try( int nTri )
		{
			const STriangle &q = tris[nTri];
			int nFreeLinks = 0;
			nFreeLinks = Max( nFreeLinks, pOptimizer->freeLinks[ q.i1 ] );
			nFreeLinks = Max( nFreeLinks, pOptimizer->freeLinks[ q.i2 ] );
			nFreeLinks = Max( nFreeLinks, pOptimizer->freeLinks[ q.i3 ] );

			if ( nFreeLinks < nBestFreeLinks )
			{
				nBestFreeLinks = nFreeLinks;
				nBestIdx = nTri;
			}
		}
	};
	struct SBestSearch
	{
		int nBestIdx, nBestCached, nBestFreeLinks, nBestDistance;
		CTriVertexCacheOptimizer *pOptimizer;
		const std::vector<STriangle> &tris;
		SBestSearch( CTriVertexCacheOptimizer *pO, const std::vector<STriangle> &_tris ) : nBestIdx( -1 ), nBestCached( -1 ), pOptimizer(pO), tris(_tris) {}
		bool GetL( int *pnResDistance, int n, int nVertex, int nVCacheSize ) 
		{
			if ( n <= *pnResDistance )
				return false;
			if ( n + 1 >= nVCacheSize )
				return false;
			if ( n + pOptimizer->freeLinks[nVertex] * 3 + 1 < nVCacheSize )
			{
				*pnResDistance = n;
				return true;
			}
			int nFL = pOptimizer->CountNotCachedFL( tris, nVertex );
			ASSERT( nFL <= pOptimizer->freeLinks[nVertex] * 3 );
			if ( n + nFL + 1 >= nVCacheSize ) 
				return false; 
			*pnResDistance = n;
			return true;// - 0 * fl;// - 3 * fl;
		}
		bool HasFound() const { return nBestCached == 3; }
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
				nCached = 1;

			int nCachePos = pOptimizer->cache.size() - 1;
			int nQDistance1, nQDistance2, nQDistance3;
			nQDistance1 = nCachePos - pOptimizer->cachePos[ q.i1 ];
			nQDistance2 = nCachePos - pOptimizer->cachePos[ q.i2 ];
			nQDistance3 = nCachePos - pOptimizer->cachePos[ q.i3 ];
			int nDistanceGuess = 0;
			if ( nQDistance1 + 1 < nVCacheSize )
				nDistanceGuess = nQDistance1;
			if ( nQDistance2 + 1 < nVCacheSize )
				nDistanceGuess = Max( nDistanceGuess, nQDistance2 );
			if ( nQDistance3 + 1 < nVCacheSize )
				nDistanceGuess = Max( nDistanceGuess, nQDistance3 );
			if ( nCached == nBestCached && nDistanceGuess < nBestDistance )
			{
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				pOptimizer->ReverseVertex();
				return;
			}

			ASSERT( !pOptimizer->isUsed[nTri] );
			pOptimizer->isUsed[ nTri ] = 1;

			GetL( &nDistance, nQDistance1, q.i1, nVCacheSize );
			GetL( &nDistance, nQDistance2, q.i2, nVCacheSize );
			GetL( &nDistance, nQDistance3, q.i3, nVCacheSize );

			nFreeLinks = Max( nFreeLinks, pOptimizer->freeLinks[ q.i1 ] );
			nFreeLinks = Max( nFreeLinks, pOptimizer->freeLinks[ q.i2 ] );
			nFreeLinks = Max( nFreeLinks, pOptimizer->freeLinks[ q.i3 ] );

			pOptimizer->ReverseVertex();
			pOptimizer->ReverseVertex();
			pOptimizer->ReverseVertex();
			pOptimizer->isUsed[ nTri ] = 0;

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
	void OptimizeVertexOrder( std::vector<STriangle> &tris, int *pnResVerts, std::vector<WORD> *pVertexReorder );
public:
	void Optimize( std::vector<STriangle> *pTris, std::vector<WORD> *pVertexReorder, int *pnResVerts, int nVCacheSize );
};

struct SEdgeInfo
{
	int nU, nV, nXSize, nYSize;
};
inline bool operator==( const SEdgeInfo &a, const SEdgeInfo &b ) { return memcmp( &a, &b, sizeof(a) ) == 0; }
struct SEdgeInfoHash
{
	int operator()( const SEdgeInfo &e ) const { return e.nU ^ e.nV ^ e.nXSize ^ e.nYSize; }
};

}

