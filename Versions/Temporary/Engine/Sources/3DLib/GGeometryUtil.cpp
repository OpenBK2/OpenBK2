#include "stdafx.h"

#include "GGeometryUtil.h"

#include <cstdint>

/*#include <d3dx8.h>
namespace NGfx
{
	extern NWin32Helper::com_ptr<IDirect3DDevice8> pDevice;
}*/
namespace NGScene
{

// CTriVertexCacheOptimizer

void CTriVertexCacheOptimizer::CountVertices( const std::vector<STriangle> &tris )
{
	int nMax = 0;
	for ( int k = 0; k < tris.size(); ++k )
	{
		nMax = (std::max)( nMax, (std::max)( (int)tris[k].i1, (std::max)( (int)tris[k].i2, (int)tris[k].i3 ) ) );
	}
	nVertices = nMax + 1;
}

void CTriVertexCacheOptimizer::Init( const std::vector<STriangle> &tris )
{
	tpvIndex.resize( nVertices + 1 );
	freeLinks.resize( 0 );
	freeLinks.resize( nVertices, 0 );
	isUsed.resize( 0 );
	isUsed.resize( tris.size(), false );
	cachePos.resize( 0 );
	cachePos.resize( nVertices, 0 );
	cache.resize( 100, -1 );
	//nCachePos = cache.size() - 1;
	// calc freeLinks
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &t = tris[k];
		++freeLinks[t.i1];
		++freeLinks[t.i2];
		++freeLinks[t.i3];
	}
	tpvIndex[0] = 0;
	for ( int k = 1; k < tpvIndex.size(); ++k )
	{
		tpvIndex[k] = tpvIndex[k - 1] + freeLinks[k - 1];
	}
	triPerVertex.resize( tpvIndex.back() );
	std::vector<int> writePtr( tpvIndex );
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &t = tris[k];
		triPerVertex[writePtr[t.i1]++] = k;
		triPerVertex[writePtr[t.i2]++] = k;
		triPerVertex[writePtr[t.i3]++] = k;
	}
	// The start queue is built only if the fallback turns out to need it often,
	// so here it is only emptied.
	startKey.resize( 0 );
	startKey.resize( tris.size(), 0 );
	startQueue = CStartQueue();
	nStartSearches = 0;
	bUseStartQueue = false;

	lastTry.resize( tris.size(), 0 );
	nTryCount = 0;
	nOutQueueSize = 0;
	nOutPrevPosSize = 0;
	outQueue.resize( ( tris.size() + 100 ) * 3 );
	outPrevPos.resize( ( tris.size() + 100 ) * 3 );
}

int CTriVertexCacheOptimizer::SearchBest( const std::vector<STriangle> &tris )
{
	++nTryCount;
	SBestSearch bs( this, tris );
	for ( int nDistance = nVCacheSize - 1; nDistance >= 0; --nDistance )
	{
		int nVertex = cache[cache.size() - nDistance - 1];
		if ( nVertex < 0 )
		{
			continue;
		}
		for ( int k = tpvIndex[nVertex]; k < tpvIndex[nVertex + 1]; ++k )
		{
			int nTri = triPerVertex[k];
			if ( isUsed[nTri] )
			{
				continue;
			}
			if ( lastTry[nTri] == nTryCount )
			{
				continue;
			}
			lastTry[nTri] = nTryCount;
			bs.Try( nTri );
			if ( bs.HasFound() )
			{
				break;
			}
		}
	}
	if ( bs.nBestIdx == -1 || bs.nBestCached == 0 )
	{
		// Nothing next to what is in the cache, so start a new strip somewhere.
		// Scanned while the fallback is rare, and through the heap once it is
		// not; both answer the same question and the switch is one way.
		if ( !bUseStartQueue )
		{
			++nStartSearches;
			if ( nStartSearches < START_QUEUE_THRESHOLD )
			{
				return ScanBestStart( tris );
			}
			BuildStartQueue( tris );
		}
		return PopBestStart();
	}
	ASSERT( bs.nBestIdx != -1 );
	return bs.nBestIdx;
}

//! The unused triangle with the lowest key, found by looking at all of them.
//!
//! What the fallback always did. Kept for the runs where it fires too rarely to
//! be worth maintaining anything, and as the definition the heap has to agree
//! with: lowest key, and among equal keys the lowest index, which falls out of
//! taking the first strict improvement while walking in index order.
int CTriVertexCacheOptimizer::ScanBestStart( const std::vector<STriangle> &tris ) const
{
	int nBestIdx = -1;
	int nBestKey = 1000000;
	for ( int k = 0; k < tris.size(); ++k )
	{
		if ( isUsed[k] )
		{
			continue;
		}
		const int nKey = StartKey( tris, k );
		if ( nKey < nBestKey )
		{
			nBestKey = nKey;
			nBestIdx = k;
		}
	}
	return nBestIdx;
}

//! Build the heap from the current keys, once the fallback has proved frequent.
//!
//! Every unused triangle goes in, used ones are left out rather than entered and
//! skipped later, and the heap is built in one pass over the range rather than
//! by pushing each entry.
void CTriVertexCacheOptimizer::BuildStartQueue( const std::vector<STriangle> &tris )
{
	std::vector<std::pair<int, int>> entries;
	entries.reserve( tris.size() );
	for ( int k = 0; k < tris.size(); ++k )
	{
		if ( isUsed[k] )
		{
			continue;
		}
		startKey[k] = StartKey( tris, k );
		entries.push_back( std::make_pair( startKey[k], k ) );
	}
	startQueue = CStartQueue( entries.begin(), entries.end() );
	bUseStartQueue = true;
}

//! Republish the keys that the emission of nUsed has moved.
//!
//! Only triangles sharing a vertex with it can have moved, since a key reads
//! nothing else, and emitting a triangle lowers the free link count of exactly
//! its own three vertices. A degenerate triangle names the same vertex twice and
//! so is walked twice; the second pass finds every key already current and
//! pushes nothing.
void CTriVertexCacheOptimizer::OnTriangleUsed( const std::vector<STriangle> &tris, int nUsed )
{
	if ( !bUseStartQueue )
	{
		return; // nothing to keep current yet
	}
	const STriangle &q = tris[nUsed];
	const int verts[3] = { q.i1, q.i2, q.i3 };
	for ( int i = 0; i < 3; ++i )
	{
		const int nVertex = verts[i];
		for ( int k = tpvIndex[nVertex]; k < tpvIndex[nVertex + 1]; ++k )
		{
			const int nTri = triPerVertex[k];
			if ( isUsed[nTri] )
			{
				continue;
			}
			const int nKey = StartKey( tris, nTri );
			if ( nKey != startKey[nTri] )
			{
				startKey[nTri] = nKey;
				startQueue.push( std::make_pair( nKey, nTri ) );
			}
		}
	}
}

//! The unused triangle with the lowest key, and the lowest index among those,
//! which is what the scan this replaces returned.
//!
//! Stale entries are dropped on the way past rather than when they go stale: an
//! entry is stale once its triangle has been used, or once startKey records a
//! different key for it, which means a later push superseded this one. The
//! winner is left on the heap, since the caller is about to use it and it will
//! be recognised as stale next time.
//!
//! -1 when nothing is left, which is what the scan returned for an exhausted
//! mesh. The caller emits one triangle per iteration and runs for as many
//! iterations as there are triangles, so it cannot actually happen.
int CTriVertexCacheOptimizer::PopBestStart()
{
	while ( !startQueue.empty() )
	{
		const std::pair<int, int> top = startQueue.top();
		if ( isUsed[top.second] || top.first != startKey[top.second] )
		{
			startQueue.pop();
			continue;
		}
		return top.second;
	}
	return -1;
}

void CTriVertexCacheOptimizer::MeasureEfficiency( const std::vector<STriangle> &tris )
{
	SVxCache<10> cache;
	int nMisses = 0, nTotal = 0;
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &q = tris[k];
		if ( !cache.IsIn( q.i1 ) )
		{
			++nMisses;
		}
		cache.Push( q.i1 );
		if ( !cache.IsIn( q.i2 ) )
		{
			++nMisses;
		}
		cache.Push( q.i2 );
		if ( !cache.IsIn( q.i3 ) )
		{
			++nMisses;
		}
		cache.Push( q.i3 );
		nTotal += 3;
	}
	// DebugTrace picks OutputDebugString or stderr per platform and adds the
	// newline itself, so the buffer this formatted into is not needed either.
	DebugTrace( "vertices per triangle = %g", 3 * nMisses / (float)( nTotal ) );
}

bool CTriVertexCacheOptimizer::OutputVertex( int n )
{
	--freeLinks[n];
	int nQDistance;
	int nCachePos = cache.size() - 1;
	nQDistance = nCachePos - cachePos[n];
	outQueue[nOutQueueSize++] = n;
	outPrevPos[nOutPrevPosSize++] = cachePos[n];
	if ( nQDistance >= nVCacheSize )
	{
		//if ( cachePos[ n ] != 0 )
		//	bEvictedReferencedVertex = true;
		cache.push_back( n );
		++nCachePos;
		cachePos[n] = nCachePos;
		return false;
	}
	return true;
}

void CTriVertexCacheOptimizer::ReverseVertex()
{
	int n = outQueue[nOutQueueSize - 1];
	if ( cachePos[n] != outPrevPos[nOutPrevPosSize - 1] )
	{
		// was cached
		cachePos[n] = outPrevPos[nOutPrevPosSize - 1];
		ASSERT( cache.back() == n );
		cache.pop_back();
	}
	++freeLinks[n];
	--nOutQueueSize;
	--nOutPrevPosSize;
}

int CTriVertexCacheOptimizer::CountNotCachedFL( const std::vector<STriangle> &tris, int nVertex )
{
	/*#ifdef _DEBUG
	int nChk = 0;
	for ( int k = tpvIndex[nVertex]; k < tpvIndex[nVertex+1]; ++k )
	{
	int nTri = triPerVertex[k];
	nChk += !isUsed[nTri];
	}
	ASSERT( freeLinks[nVertex] == nChk );
	#endif*/
	int nRes = 0;
	int nState = nOutQueueSize;
	for ( int k = tpvIndex[nVertex]; k < tpvIndex[nVertex + 1]; ++k )
	{
		int nTri = triPerVertex[k];
		if ( isUsed[nTri] )
		{
			continue;
		}
		const STriangle &t = tris[nTri];
		nRes += !OutputVertex( t.i1 );
		nRes += !OutputVertex( t.i2 );
		nRes += !OutputVertex( t.i3 );
	}
	while ( nState != nOutQueueSize )
	{
		ReverseVertex();
	}
	return nRes;
}

void CTriVertexCacheOptimizer::OptimizeVertexOrder( std::vector<STriangle> &tris,
                                                    int *pnResVerts,
                                                    std::vector<uint16_t> *pVertexReorder )
{
	std::vector<uint16_t> &position = *pVertexReorder;
	position.resize( 0 );
	position.resize( nVertices, 0xffff );
	uint16_t nPos = 0;
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &q = tris[k];
		if ( position[q.i1] == 0xffff )
		{
			position[q.i1] = nPos++;
		}
		if ( position[q.i2] == 0xffff )
		{
			position[q.i2] = nPos++;
		}
		if ( position[q.i3] == 0xffff )
		{
			position[q.i3] = nPos++;
		}
	}
	*pnResVerts = nPos;
	// clean up to avoid crashes
	for ( int k = 0; k < position.size(); ++k )
	{
		if ( position[k] == 0xffff )
		{
			position[k] = nPos++;
		}
	}
	ASSERT( nPos <= position.size() );
}

void CTriVertexCacheOptimizer::Optimize( std::vector<STriangle> *pTris,
                                         std::vector<uint16_t> *pVertexReorder,
                                         int *pnResVerts,
                                         int _nVCacheSize )
{
	nVCacheSize = _nVCacheSize;
	/*vector<STriangle> tt;
	for ( int y = 0; y < 8; ++y )
	{
	for ( int x = 0; x < 8; ++x )
	{
	tt.push_back( STriangle( (y)*9 + x  , (y)*9 + x+1, (y+1)*9+x ) );
	tt.push_back( STriangle( (y)*9 + x+1, (y+1)*9 + x, (y+1)*9+x+1 ) );
	}
	}
	*pTris = tt;*/
	if ( pTris->size() <= nVCacheSize / 3 )
	{
		CountVertices( *pTris );
		OptimizeVertexOrder( *pTris, pnResVerts, pVertexReorder );
		return;
	}
	//	DebugTrace( "%d tris\n", pTris->size() );
	//	MeasureEfficiency( *pTris );
	// prepare some data structures
	CountVertices( *pTris );
	Init( *pTris );
	// form result
	std::vector<STriangle> res;
	for ( int k = 0; k < pTris->size(); ++k )
	{
		// search for a best candidate
		int nBest = SearchBest( *pTris );
		ASSERT( !isUsed[nBest] );
		const STriangle &q = ( *pTris )[nBest];
		OutputVertex( q.i1 );
		OutputVertex( q.i2 );
		OutputVertex( q.i3 );
		isUsed[nBest] = true;
		// After isUsed, so that the triangle just emitted is skipped, and after
		// the three OutputVertex calls, which are what lowered the free links the
		// keys are made of.
		OnTriangleUsed( *pTris, nBest );
		res.push_back( q );
		//if ( bEvictedReferencedVertex )
		//	break;
	}
	/*	if ( bEvictedReferencedVertex )
	{
//		OutputDebugString( "non trivial mesh, using d3dx optimizer\n" );
		int nTris = pTris->size();
		NWin32Helper::com_ptr<ID3DXMesh> pMesh;
		D3DXCreateMeshFVF( nTris, nVertices, D3DXMESH_SYSTEMMEM, D3DFVF_XYZ, NGfx::pDevice, pMesh.GetAddr() );
		STriangle *pBuf;
		pMesh->LockIndexBuffer( 0, (uint8_t**)&pBuf );
		for ( int k = 0; k < nTris; ++k )
			pBuf[k] = (*pTris)[k];
		pMesh->UnlockIndexBuffer();
		vector<uint32_t> dwAdj( nTris * 3 );
		vector<uint32_t> dwRes( nTris );
		pMesh->ConvertPointRepsToAdjacency( 0, &dwAdj[0] );
		NWin32Helper::com_ptr<ID3DXBuffer> pReorder;
		pMesh->OptimizeInplace( D3DXMESHOPT_VERTEXCACHE, &dwAdj[0], 0, &dwRes[0], pReorder.GetAddr() );
		ASSERT( pReorder->GetBufferSize() == nVertices * 4 );
		pVertexReorder->resize( nVertices );
		uint32_t *pReorderBuf = (uint32_t*)pReorder->GetBufferPointer();
		for ( int k = 0; k < nVertices; ++k )
			(*pVertexReorder)[k] = pReorderBuf[k];
		res.resize( nTris );
		for ( int k = 0; k < res.size(); ++k )
			res[k] = (*pTris)[ dwRes[k] ];
	}
	else*/
	{
		*pTris = res;
		//		MeasureEfficiency( *pTris );
		OptimizeVertexOrder( *pTris, pnResVerts, pVertexReorder );
	}
}

} // namespace NGScene
