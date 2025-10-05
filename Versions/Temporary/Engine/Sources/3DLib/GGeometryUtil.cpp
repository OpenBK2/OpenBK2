#include "stdafx.h"
#include "GGeometryUtil.h"

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
		nMax = Max( nMax, Max( (int)tris[k].i1, Max( (int)tris[k].i2, (int)tris[k].i3 ) ) );
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
		++freeLinks[ t.i1 ];
		++freeLinks[ t.i2 ];
		++freeLinks[ t.i3 ];
	}
	tpvIndex[0] = 0;
	for ( int k = 1; k < tpvIndex.size(); ++k )
		tpvIndex[k] = tpvIndex[k-1] + freeLinks[k-1];
	triPerVertex.resize( tpvIndex.back() );
	std::vector<int> writePtr( tpvIndex );
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &t = tris[k];
		triPerVertex[ writePtr[ t.i1 ]++ ] = k;
		triPerVertex[ writePtr[ t.i2 ]++ ] = k;
		triPerVertex[ writePtr[ t.i3 ]++ ] = k;
	}
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
		int nVertex = cache[ cache.size() - nDistance - 1 ];
		if ( nVertex < 0 )
			continue;
		for ( int k = tpvIndex[nVertex]; k < tpvIndex[nVertex + 1]; ++k )
		{
			int nTri = triPerVertex[ k ];
			if ( isUsed[nTri] )
				continue;
			if ( lastTry[nTri] == nTryCount )
				continue;
			lastTry[nTri] = nTryCount;
			bs.Try( nTri );
			if ( bs.HasFound() )
				break;
		}
	}
	if ( bs.nBestIdx == -1 || bs.nBestCached == 0 )
	{
		SStartSearch start( this, tris );
		// start over
		for ( int k = 0; k < tris.size(); ++k )
		{
			if ( isUsed[k] )
				continue;
			start.Try( k );
		}
		return start.nBestIdx;
	}
	ASSERT( bs.nBestIdx != -1 );
	return bs.nBestIdx;
}

void CTriVertexCacheOptimizer::MeasureEfficiency( const std::vector<STriangle> &tris )
{
	SVxCache<10> cache;
	int nMisses = 0, nTotal = 0;
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &q = tris[k];
		if ( !cache.IsIn( q.i1 ) )
			++nMisses;
		cache.Push( q.i1 );
		if ( !cache.IsIn( q.i2 ) )
			++nMisses;
		cache.Push( q.i2 );
		if ( !cache.IsIn( q.i3 ) )
			++nMisses;
		cache.Push( q.i3 );
		nTotal += 3;
	}
	char szBuf[1024];
	sprintf( szBuf, "vertices per triangle = %g\n", 3 * nMisses / (float)(nTotal) );
	OutputDebugString( szBuf );
}

bool CTriVertexCacheOptimizer::OutputVertex( int n )
{
	--freeLinks[n];
	int nQDistance;
	int nCachePos = cache.size() - 1;
	nQDistance = nCachePos - cachePos[ n ];
	outQueue[ nOutQueueSize++ ] = n;
	outPrevPos[ nOutPrevPosSize++ ] = cachePos[n];
	if ( nQDistance >= nVCacheSize )
	{
		//if ( cachePos[ n ] != 0 )
		//	bEvictedReferencedVertex = true;
		cache.push_back( n );
		++nCachePos;
		cachePos[ n ] = nCachePos;
		return false;
	}
	return true;
}

void CTriVertexCacheOptimizer::ReverseVertex()
{
	int n = outQueue[ nOutQueueSize - 1 ];
	if ( cachePos[n] != outPrevPos[ nOutPrevPosSize - 1 ] )
	{
		// was cached
		cachePos[n] = outPrevPos[ nOutPrevPosSize - 1 ];
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
	for ( int k = tpvIndex[nVertex]; k < tpvIndex[nVertex+1]; ++k )
	{
		int nTri = triPerVertex[k];
		if ( isUsed[nTri] )
			continue;
		const STriangle &t = tris[nTri];
		nRes += !OutputVertex( t.i1 );
		nRes += !OutputVertex( t.i2 );
		nRes += !OutputVertex( t.i3 );
	}
	while ( nState != nOutQueueSize )
		ReverseVertex();
	return nRes;
}

void CTriVertexCacheOptimizer::OptimizeVertexOrder( std::vector<STriangle> &tris, int *pnResVerts, std::vector<WORD> *pVertexReorder )
{
	std::vector<WORD> &position = *pVertexReorder;
	position.resize(0);
	position.resize( nVertices, 0xffff );
	WORD nPos = 0;
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &q = tris[k];
		if ( position[q.i1] == 0xffff )
			position[q.i1] = nPos++;
		if ( position[q.i2] == 0xffff )
			position[q.i2] = nPos++;
		if ( position[q.i3] == 0xffff )
			position[q.i3] = nPos++;
	}
	*pnResVerts = nPos;
	// clean up to avoid crashes
	for ( int k = 0; k < position.size(); ++k )
	{
		if ( position[k] == 0xffff )
			position[k] = nPos++;
	}
	ASSERT( nPos <= position.size() );
}

void CTriVertexCacheOptimizer::Optimize( std::vector<STriangle> *pTris, std::vector<WORD> *pVertexReorder, int *pnResVerts, int _nVCacheSize )
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
		const STriangle &q = (*pTris)[nBest];
		OutputVertex( q.i1 );
		OutputVertex( q.i2 );
		OutputVertex( q.i3 );
		isUsed[nBest] = true;
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
		pMesh->LockIndexBuffer( 0, (BYTE**)&pBuf );
		for ( int k = 0; k < nTris; ++k )
			pBuf[k] = (*pTris)[k];
		pMesh->UnlockIndexBuffer();
		vector<DWORD> dwAdj( nTris * 3 );
		vector<DWORD> dwRes( nTris );
		pMesh->ConvertPointRepsToAdjacency( 0, &dwAdj[0] );
		NWin32Helper::com_ptr<ID3DXBuffer> pReorder;
		pMesh->OptimizeInplace( D3DXMESHOPT_VERTEXCACHE, &dwAdj[0], 0, &dwRes[0], pReorder.GetAddr() );
		ASSERT( pReorder->GetBufferSize() == nVertices * 4 );
		pVertexReorder->resize( nVertices );
		DWORD *pReorderBuf = (DWORD*)pReorder->GetBufferPointer();
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

}

