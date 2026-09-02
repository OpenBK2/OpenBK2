#include "stdafx.h"
#include "3DLib/Bound.h"
#include "aiObject.h"

#include <cstdint>

//#include "PrecalcSpheres.h"

namespace NAI
{

// CEdgesInfo

int SelectNonZero( const std::vector<int> & count )
{
	for ( int i = 0; i < count.size(); ++i )
		if ( count[i] != 0 )
			return i;
	return -1;
}

inline void CountEdge( std::vector<int> *pRes, int n )
{
	if ( n & 0x8000 )
		(*pRes)[n&0x7fff]--;
	else
		(*pRes)[n&0x7fff]++;
}

int PushTri( const STriangle &t, const std::vector<SEdge> &edges, std::vector<int> *pCount, std::vector<STriangle> *pTris)
{
	uint16_t i1, i2, i3;
	if ( t.i1 & 0x8000 )
	{
		i1 = edges[ t.i1 & 0x7fff ].wFinish; 
		i2 = edges[ t.i1 & 0x7fff ].wStart;
	}
	else
	{
		i1 = edges[ t.i1 & 0x7fff ].wStart; 
		i2 = edges[ t.i1 & 0x7fff ].wFinish;
	}
	if ( t.i2 & 0x8000 )
		i3 = edges[ t.i2 & 0x7fff ].wStart;
	else
		i3 = edges[ t.i2 & 0x7fff ].wFinish;
	pTris->push_back( STriangle( i1, i2, i3 ) );
	std::vector<int> &count = *pCount;
	CountEdge( &count, t.i1 );
	CountEdge( &count, t.i2 );
	CountEdge( &count, t.i3 );
	// вернуть индекс незакрытого ребра
	if ( count[ t.i1 & 0x7fff ] )
		return t.i1 & 0x7fff;
	if ( count[ t.i2 & 0x7fff ] )
		return t.i2 & 0x7fff;
	if ( count[ t.i3 & 0x7fff ] )
		return t.i3 & 0x7fff;
	return -1;
}

bool CEdgesInfo::IsClosed() const
{
	std::vector<int> count;
	count.resize( edges.size(), 0 );
	for ( int i = 0; i < mesh.size(); ++i )
	{
		const STriangle &t = mesh[i];
		CountEdge( &count, t.i1 );
		CountEdge( &count, t.i2 );
		CountEdge( &count, t.i3 );
	}
	bool bRes = true;
	for ( int k = 0; k < count.size(); ++k )
		bRes &= ( count[k] == 0 );
	return bRes;
}

void CEdgesInfo::BuildClosedMeshes( std::vector<std::vector<STriangle> > *pMeshes ) const
{
	std::vector<std::vector<STriangle> > &meshes = *pMeshes;
	meshes.clear();
	std::vector<STriangle> *pCurrMesh = 0;
	if ( !IsClosed() )
	{
		ASSERT(0);
		pMeshes->push_back( std::vector<STriangle>() );
		pCurrMesh = &pMeshes->back();
		BuildTriangleList( pCurrMesh );
		return;
	}
	int nFreeTris = mesh.size();
	std::vector<int> count;
	std::vector<char> triUsed;
	count.resize( edges.size(), 0 );
	triUsed.resize( nFreeTris, 0 );
	int nEdge = -1;
	while ( nFreeTris > 0 )	
	{
		int nTri;
		if ( nEdge == -1 )
		{
			// make new mesh
			pMeshes->push_back( std::vector<STriangle>() );
			pCurrMesh = &pMeshes->back();
			// push next free triangle
			nTri = 0;
			while ( triUsed[ nTri ] )
				++nTri;
		}
		else
		{
			if ( count[ nEdge ] > 0 )
				nEdge += 0x8000;
			for ( nTri = 0; ; ++nTri )
			{
				if ( triUsed[ nTri ] )
					continue;
				const STriangle &t = mesh[ nTri ];
				if ( t.i1 == nEdge )
					break;
				if ( t.i2 == nEdge )
					break;
				if ( t.i3 == nEdge )
					break;
			}
		}
		--nFreeTris;
		triUsed[ nTri ] = 1;
		nEdge = PushTri( mesh[ nTri ], edges, &count, pCurrMesh );
		if ( nEdge == -1 )
			nEdge = SelectNonZero( count );
	}
	ASSERT( SelectNonZero( count ) == -1 );
}

void CEdgesInfo::BuildTriangleList( std::vector<STriangle> *pRes ) const
{
	ASSERT( pRes != 0 );
	pRes->resize( mesh.size() );
	for ( int i = 0; i < mesh.size(); ++i )
	{
		uint16_t i1, i2, i3;
		const STriangle &t = mesh[i];
		if ( t.i1 & 0x8000 )
		{
			i1 = edges[ t.i1 & 0x7fff ].wFinish; 
			i2 = edges[ t.i1 & 0x7fff ].wStart;
		}
		else
		{
			i1 = edges[ t.i1 & 0x7fff ].wStart; 
			i2 = edges[ t.i1 & 0x7fff ].wFinish;
		}
		if ( t.i2 & 0x8000 )
			i3 = edges[ t.i2 & 0x7fff ].wStart;
		else
			i3 = edges[ t.i2 & 0x7fff ].wFinish;
		//
		(*pRes)[i] = STriangle( i1, i2, i3 );
	}
}

uint16_t CEdgesInfo::InsertEdge( uint16_t i1, uint16_t i2, const std::vector<CVec3> &pts )
{
	for ( int i = 0; i < edges.size(); i++ )
	{
		if ( edges[i].wStart == i1 && edges[i].wFinish == i2 )
			return i;
		if ( edges[i].wStart == i2 && edges[i].wFinish == i1 )
			return i | 0x8000;
	}
	uint16_t f = 0;
	SEdge edge( i1, i2 );
	const CVec3 &p1 = pts[i1];
	const CVec3 &p2 = pts[i2];
	if ( p1.x > p2.x )
	{
		edge.wStart = i2;
		edge.wFinish = i1;
		f = 0x8000;
	}
	else if ( p1.x == p2.x )
	{
		if ( p1.y > p2.y )
		{
			edge.wStart = i2;
			edge.wFinish = i1;
			f = 0x8000;
		}
		else if ( p1.y == p2.y )
		{
			if ( p1.z > p2.z )
			{
				edge.wStart = i2;
				edge.wFinish = i1;
				f = 0x8000;
			}
//			else if ( p1.z == p2.z )
//			ASSERT(0);
		}
	}
	edges.push_back( edge );
	return (edges.size() - 1) | f;
}

void CEdgesInfo::GenerateEdgeList( const std::vector<STriangle> &tris, const std::vector<CVec3> &pts )
{
	for ( int i = 0; i < tris.size(); ++i )
	{
		const STriangle &t = tris[i];
		if ( t.i1 == t.i2 || t.i1 == t.i3 || t.i2 == t.i3 )
			continue;
		mesh.push_back( STriangle( 
			InsertEdge( t.i1, t.i2, pts ), 
			InsertEdge( t.i2, t.i3, pts ), 
			InsertEdge( t.i3, t.i1, pts ) ) );
	}
}

// CGeometryInfo

SPiece* CGeometryInfo::GetPiece( int nPieceID ) 
{ 
	CPieceMap::iterator i = pieces.find( nPieceID );
	if ( i == pieces.end() )
		return 0;
	return &i->second;
}
void CGeometryInfo::AddPiece( int nPieceID, const std::vector<CVec3> &_points, 
	const std::vector<STriangle> &_tris, float fVolume, std::vector<SJunction> juncs, bool _bClosed )
	//, const std::vector<CPtr<CPrecalcSpheres> > &spheres )
{
	ASSERT( pieces.find( nPieceID ) == pieces.end() );
	if ( _tris.empty() )
		return;
	SPiece &p = pieces[nPieceID];
	p.points = _points;
	p.edges.GenerateEdgeList( _tris, _points );
	p.edges.bClosed = _bClosed;
	p.fVolume = fVolume;
	p.juncs = juncs;
	//p.precalc = spheres;
	//for ( int i = 0; i < p.precalc.size(); ++i )
	//	p.precalc[i]->MakeSureBoundIsGood();
	/*if ( !p.pPrecalc )
	{
		p.pPrecalc = new CPrecalcSpheres;
		std::vector<STriangle> tris;
		p.edges.BuildTriangleList( &tris );
		p.pPrecalc->Generate( p.points, tris );
	}*/
	//ASSERT( !_bClosed || p.edges.IsClosed() );
}

void CGeometryInfo::CalcBound()
{
	SBoundCalcer b;
	for ( CPieceMap::const_iterator i = pieces.begin(); i != pieces.end(); ++i )
		b.LookSet( i->second.points, SGetSelf<CVec3>() );
	
	b.Make( &bound );
}

/*void CGeometryInfo::PrecalcCollideInfo( bool bTerrain )
{
	CPieceMap::iterator i;
	if ( bTerrain )
		return; // crap
	for ( i = pieces.begin(); i != pieces.end(); ++i )
	{
		SPiece &p = i->second;
		std::vector<STriangle> tris;
		p.edges.BuildTriangleList( &tris );
		GeneratePrecalcSpheres( &p.precalc, p.points, tris );
	}
}*/

/*void CGeometryInfo::SetCollideInfo( const CPrecalcPieces &precalc )
{
	CPrecalcPieces::const_iterator it;
	for ( it = precalc.begin(); it != precalc.end(); ++it )
	{
		CPieceMap::iterator iPiece = pieces.find( it->first );
		if ( iPiece == pieces.end() )
			continue;
		SPiece &p = iPiece->second;
		p.precalc.clear();
		p.precalc.push_back( it->second );
	}
}*/

/*void MakeCube( CConvexHull *pRes, const CVec3 &base, const CVec3 &size )
{
	std::vector<CVec3> &gpos = pRes->points;
	gpos.resize( 8 );
	gpos[0] = CVec3( base.x,          base.y,          base.z );
	gpos[1] = CVec3( base.x,          base.y + size.y, base.z );
	gpos[2] = CVec3( base.x + size.x, base.y + size.y, base.z );
	gpos[3] = CVec3( base.x + size.x, base.y         , base.z );
	gpos[4] = CVec3( base.x,          base.y,          base.z + size.z);
	gpos[5] = CVec3( base.x,          base.y + size.y, base.z + size.z );
	gpos[6] = CVec3( base.x + size.x, base.y + size.y, base.z + size.z );
	gpos[7] = CVec3( base.x + size.x, base.y         , base.z + size.z );
	//
	std::vector<STriangle> tris;
	tris.resize( 12 );
	tris[0] = STriangle( 0, 1, 2 );
	tris[1] = STriangle( 0, 2, 3 );
	tris[2] = STriangle( 3, 6, 7 );
	tris[3] = STriangle( 3, 2, 6 );
	tris[4] = STriangle( 0, 7, 4 );
	tris[5] = STriangle( 0, 3, 7 );
	tris[6] = STriangle( 5, 0, 4 );
	tris[7] = STriangle( 5, 1, 0 );
	tris[8] = STriangle( 6, 1, 5 );
	tris[9] = STriangle( 6, 2, 1 );
	tris[10] = STriangle( 4, 6, 5 );
	tris[11] = STriangle( 4, 7, 6 );
	//
	GenerateEdgeList( pRes, tris );
}*/

} // namespace
using namespace NAI;

