#include "stdafx.h"
#include "MemObject.h"
#include "Bound.h"
#include "Transform.h"
#include "PolyUtils.h"

#include <cstdint>

#include "3DLib_export.h"

struct SMemBuilderVertex
{
	CVec3 point;
	CVec3 normal;
	bool operator==( const SMemBuilderVertex &a ) const 
	{
		return point == a.point && fabs2( normal - a.normal ) < 1e-6;
	}
};

struct SMemBuilderVertexHash
{
	int operator()( const SMemBuilderVertex &v ) const
	{ 
		float f = v.point.x + v.point.y * 123 + v.point.z * 15352331;
		return *(int*)&f;
	}
};

class CMemObjectBuilder
{
	typedef std::unordered_map<SMemBuilderVertex, int, SMemBuilderVertexHash> CPointsHash;
	CPointsHash points;
	CMemObject *p;
public:
	CMemObjectBuilder( CMemObject *_p ): p(_p) {}
	uint16_t GetPointIndex( const CVec3 &point, const CVec3 &normal );
	void AddSphereTriangle( const CVec3 &a, const CVec3 &b, const CVec3 &c, int nSubs );
};

uint16_t CMemObjectBuilder::GetPointIndex( const CVec3 &point, const CVec3 &normal )
{
	int nCounter = 0;
	SMemBuilderVertex v;
	v.point = point;
	v.normal = normal;
	CPointsHash::iterator i = points.find( v );
	if ( i == points.end() )
	{
		int nIndex = p->resPoints.size();
		p->resPoints.push_back( point );
		p->resNormals.push_back( normal );
		points[v] = nIndex;
		return nIndex;
	}
	return i->second;
}

void CMemObjectBuilder::AddSphereTriangle( const CVec3 &a, const CVec3 &b, const CVec3 &c, int nSubs )
{
	if ( nSubs == 0 )
	{
		uint16_t w1 = GetPointIndex( a, a );
		uint16_t w2 = GetPointIndex( b, b );
		uint16_t w3 = GetPointIndex( c, c );
		p->resTris.push_back( STriangle( w1, w2, w3 ) );
	}
	else
	{
		CVec3 ab( a + b );
		CVec3 bc( b + c );
		CVec3 ca( a + c );
		Normalize( &ab );
		Normalize( &bc );
		Normalize( &ca );
		AddSphereTriangle( ab, b, bc, nSubs - 1 );
		AddSphereTriangle( a, ab, ca, nSubs - 1 );
		AddSphereTriangle( ca, bc, c, nSubs - 1 );
		AddSphereTriangle( ab, bc, ca, nSubs - 1 );
		Normalize( &ca );// COMPILER - compiler generate wrong code without this line
	}
}

// CMemObject

void CMemObject::CreateCube( const CVec3 &base, const CVec3 &size, bool bTwoSided )
{
	std::vector<CVec3> points;
	std::vector<STriangle> tris;
	//
	points.resize( 8 );
	points[0] = CVec3( base.x,          base.y,          base.z );
	points[1] = CVec3( base.x,          base.y + size.y, base.z );
	points[2] = CVec3( base.x + size.x, base.y + size.y, base.z );
	points[3] = CVec3( base.x + size.x, base.y         , base.z );
	points[4] = CVec3( base.x,          base.y,          base.z + size.z);
	points[5] = CVec3( base.x,          base.y + size.y, base.z + size.z );
	points[6] = CVec3( base.x + size.x, base.y + size.y, base.z + size.z );
	points[7] = CVec3( base.x + size.x, base.y         , base.z + size.z );
	//
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
	if ( bTwoSided )
	{
		points.insert( points.end(), points.begin(), points.end() );
		for ( int i = 0; i < 12; ++i )
			tris.push_back( STriangle( tris[i].i1, tris[i].i3, tris[i].i2 ) );
	}
	//
	Create( points, tris );
}

/*static void TypeVector( const CVec3 &a )
{
	char szBuf[128];
	sprintf( szBuf, "x = %f  y = %f  z = %f\n", a.x, a.y, a.z );
	OutputDebugString( szBuf );
}*/

void CMemObject::CreateSphere( const CVec3 &ptCenter, float fRadius, int nSubs )
{
	Clear();
	CMemObjectBuilder mb( this );
	CVec3 a( 1.000f, 0.000f, -0.707f );
  CVec3 b( 0.000f, -1.000f, 0.707f );
  CVec3 c( 0.000f, 1.000f, 0.707f );
  CVec3 d( -1.000f, 0.000f, -0.707f );
	Normalize( &a );
	Normalize( &b );
	Normalize( &c );
	Normalize( &d );
	mb.AddSphereTriangle( a, c, b, nSubs );
	mb.AddSphereTriangle( a, b, d, nSubs );
	mb.AddSphereTriangle( a, d, c, nSubs );
	mb.AddSphereTriangle( b, c, d, nSubs );
	for ( int i = 0; i < resPoints.size(); ++i )
		resPoints[i] = resPoints[i] * fRadius + ptCenter;
}

void CMemObject::CreatePolyline( const std::vector<CVec3> &points )
{
	resPoints = points;
	bPolyLine = true;
}

struct SMOTessPoint
{
	int nIndex;
	float fNormal;
	CVec2 vPoint;
	
	SMOTessPoint( float _fNormal, const CVec2 &_vPoint, int _nIndex ): fNormal(_fNormal), vPoint(_vPoint), nIndex(_nIndex) {}
};
typedef CVec2 TTriangle[3];

void CMemObject::CreatePolygone( const std::vector<CVec2> &points, float fZ )
{
	if ( points.size() < 3 )
		return;
	
	std::vector<CVec3> vert;
	vert.resize( points.size() );

	std::list<SMOTessPoint> poly;
	for ( int i = 0; i < points.size(); i++ )
	{
		const CVec2 &vPrev2 = points[ (i-1 + points.size()) % points.size() ];
		const CVec2 &vNext2 = points[ (i+1) % points.size() ];
		const CVec2 &vTemp2 = points[i];
		CVec3 vPrev( vPrev2.x, vPrev2.y, 0 );
		CVec3 vNext( vNext2.x, vNext2.y, 0 );
		CVec3 vTemp( vTemp2.x, vTemp2.y, 0 );
		CVec3 vNormal( (vPrev - vTemp) ^ (vNext - vTemp) );
		poly.push_back( SMOTessPoint( vNormal.z, vTemp2, i ) );

		vert[i] = CVec3( vTemp2.x, vTemp2.y, fZ );
	}

	bool bTryAgain = true;
	float fRange = -FP_EPSILON;
	std::vector<STriangle> tris;
	while( poly.size() > 2 )	
	{
		bool bRemoved = false;
		std::list<SMOTessPoint>::iterator iPrev, iPrev2;
		iPrev = poly.end();
		iPrev--;
		iPrev2 = iPrev;
		iPrev2--;
		for ( std::list<SMOTessPoint>::iterator i = poly.begin(); i != poly.end(); iPrev2 = iPrev, iPrev = i, ++i )
		{
			const SMOTessPoint &prev = *iPrev2;
			const SMOTessPoint &temp = *iPrev;
			const SMOTessPoint &next = *i;
			
			if ( temp.fNormal > fRange )
				continue;

			bool bHasInside = false;
			std::vector<CVec2> vTri( 3 );
			vTri[0] = prev.vPoint;
			vTri[1] = temp.vPoint;
			vTri[2] = next.vPoint;
			for ( int i = 0; i < points.size(); i++ )
			{
				const CVec2 &vTest = points[i];

				bool bGoOut = false;
				for ( int nTemp = 0; nTemp < 3; nTemp++ )
				{
					if ( vTest == vTri[nTemp] )
						bGoOut = true;
				}
				if ( bGoOut )
					continue;
				
				bHasInside = IsPointInPolygon( vTri, vTest );
				if ( bHasInside )
					break;
			}
			if ( bHasInside )
				continue;

			tris.push_back( STriangle( next.nIndex, temp.nIndex, prev.nIndex ) );
			poly.erase( iPrev );
			bRemoved = true;
			break;
		}
		if ( bRemoved )
		{
			std::list<SMOTessPoint>::iterator iPrev, iPrev2;
			iPrev = poly.end();
			iPrev--;
			iPrev2 = iPrev;
			iPrev2--;
			for ( std::list<SMOTessPoint>::iterator i = poly.begin(); i != poly.end(); iPrev2 = iPrev, iPrev = i, ++i )
			{
				SMOTessPoint &temp = *iPrev;
				const SMOTessPoint &prev = *iPrev2;
				const SMOTessPoint &next = *i;

				CVec3 vPrev( prev.vPoint.x, prev.vPoint.y, 0 );
				CVec3 vNext( next.vPoint.x, next.vPoint.y, 0 );
				CVec3 vTemp( temp.vPoint.x, temp.vPoint.y, 0 );
				CVec3 vNormal( (vPrev - vTemp) ^ (vNext - vTemp) );
				temp.fNormal = vNormal.z;
			}
		}
		else if ( bTryAgain )
		{
			bTryAgain = false;
			fRange = 0;
		}
		else
			break;
	}
	////
	Create( vert, tris );
}

void CMemObject::CreateCylinder( const CVec3 &ptStart, const CVec3 &ptEnd, float fRadius, int nSubs, bool bClose )
{
	CMemObjectBuilder b( this );
	std::vector<CVec3> base;
	++nSubs;

	const float fStep = FP_PI2 / nSubs + 0.001;
	for ( float fA = fStep; fA < FP_PI2; fA += fStep )
		base.push_back( CVec3( fRadius * cos( fA ), 0, fRadius * sin( fA ) ) );
	for ( int i = base.size() - 1; i >= 0; --i )
		base.push_back( CVec3( -base[i].x, base[i].y, base[i].z ) );
	for ( int i = base.size() - 1; i >= 0; --i )
		base.push_back( CVec3( base[i].x, base[i].y, -base[i].z ) );
	if ( base.size() < 2 )
		return;
	CVec3 ptDir = ptEnd - ptStart;
	base.push_back( base.front() );
	SHMatrix m;
	MakeMatrix( &m, ptStart, ptDir );
	m.RotateHVector( &base[0], base[0] );
	CVec3 n(0, 1, 0);
	m.RotateHVector( &n, n );
	uint16_t capd = b.GetPointIndex( base[0], -n );
	uint16_t capu = b.GetPointIndex( base[0] + ptDir, n );
	for ( int i = 1; i < base.size(); ++i )
	{
		m.RotateHVector( &base[i], base[i] );
		uint16_t w1 = b.GetPointIndex( base[i-1], VNULL3 );
		uint16_t w2 = b.GetPointIndex( base[i], VNULL3 );
		uint16_t w3 = b.GetPointIndex( base[i-1] + ptDir, VNULL3 );
		uint16_t w4 = b.GetPointIndex( base[i] + ptDir, VNULL3 );
		CVec3 n1 = (resPoints[w1] - resPoints[w4]) ^ (resPoints[w2] - resPoints[w4]);
		CVec3 n2 = (resPoints[w4] - resPoints[w1]) ^ (resPoints[w3] - resPoints[w1]);
		Normalize( &n1 );
		Normalize( &n2 );
		const CVec3 n3 = 0.5f * (n1 + n2);
		resNormals[w1] = n3;
		resNormals[w2] = n1;
		resNormals[w3] = n2;
		resNormals[w4] = n3;
		resTris.push_back( STriangle( w1, w4, w2 ) );
		resTris.push_back( STriangle( w1, w3, w4 ) );
		if ( bClose )
		{
			uint16_t c1 = b.GetPointIndex( base[i-1], -n );
			uint16_t c2 = b.GetPointIndex( base[i], -n );
			uint16_t c3 = b.GetPointIndex( base[i-1] + ptDir, n );
			uint16_t c4 = b.GetPointIndex( base[i] + ptDir, n );
			resTris.push_back( STriangle( capd, c1, c2 ) );
			resTris.push_back( STriangle( capu, c4, c3 ) );
		}
	}
}

void CMemObject::CreateIsoscelesColumn( const CVec3 &ptBase, float fHeight, float fBase )
{
	CMemObjectBuilder b( this );
	CVec3 ptLeft( ptBase );
	ptLeft.y += 0.5f * fBase;
	CVec3 ptRight( ptLeft );
	ptRight.y -= fBase;
	CVec3 ptMiddle( ptBase );
	ptMiddle.x += 0.5f * fBase;

	CVec3 ptN( -1, 0, 0 );
	uint16_t w1 = b.GetPointIndex( ptLeft, ptN );
	uint16_t w2 = b.GetPointIndex( ptLeft + CVec3(0,0,fHeight), ptN );
	uint16_t w3 = b.GetPointIndex( ptRight + CVec3(0,0,fHeight), ptN );
	uint16_t w4 = b.GetPointIndex( ptRight, ptN );
	resTris.push_back( STriangle( w1, w3, w2 ) );
	resTris.push_back( STriangle( w1, w4, w3 ) );
	//
	ptN = CVec3( 1, 1, 0 );
	Normalize( &ptN );
	w1 = b.GetPointIndex( ptMiddle, ptN );
	w2 = b.GetPointIndex( ptMiddle + CVec3(0,0,fHeight), ptN );
	w3 = b.GetPointIndex( ptLeft + CVec3(0,0,fHeight), ptN );
	w4 = b.GetPointIndex( ptLeft, ptN );
	resTris.push_back( STriangle( w1, w3, w2 ) );
	resTris.push_back( STriangle( w1, w4, w3 ) );
	//
	ptN = CVec3( 1, -1, 0 );
	Normalize( &ptN );
	w1 = b.GetPointIndex( ptRight, ptN );
	w2 = b.GetPointIndex( ptRight + CVec3(0,0,fHeight), ptN );
	w3 = b.GetPointIndex( ptMiddle + CVec3(0,0,fHeight), ptN );
	w4 = b.GetPointIndex( ptMiddle, ptN );
	resTris.push_back( STriangle( w1, w3, w2 ) );
	resTris.push_back( STriangle( w1, w4, w3 ) );
	//
	ptN = CVec3( 0, 0, -1 );
	w1 = b.GetPointIndex( ptRight, ptN );
	w2 = b.GetPointIndex( ptMiddle, ptN );
	w3 = b.GetPointIndex( ptLeft, ptN );
	resTris.push_back( STriangle( w1, w3, w2 ) );
	//
	ptN = CVec3( 0, 0, 1 );
	CVec3 ptH(0,0,fHeight);
	w1 = b.GetPointIndex( ptRight + ptH, ptN );
	w2 = b.GetPointIndex( ptLeft + ptH, ptN );
	w3 = b.GetPointIndex( ptMiddle + ptH, ptN );
	resTris.push_back( STriangle( w1, w3, w2 ) );
}

void CMemObject::Clear()
{
	resPoints.clear();
	resNormals.clear();
	resTris.clear();
	bPolyLine = false;
}

void CMemObject::Create( const std::vector<CVec3> &points, const std::vector<STriangle> &tris )
{
	Clear();
	CMemObjectBuilder b( this );
	for ( int i = 0; i < tris.size(); ++i )
	{
		const STriangle &t = tris[i];
		//CVec3 normal(0,0,1);
		CVec3 normal = ( points[t.i2] - points[t.i1] ) ^ ( points[t.i3] - points[t.i1] );
		Normalize( &normal );
		resTris.push_back( STriangle( 
			b.GetPointIndex(points[t.i1], normal ),
			b.GetPointIndex(points[t.i2], normal ),
			b.GetPointIndex(points[t.i3], normal )
			) );
	}
}

void CMemObject::CalcBound( SSphere *pRes ) const
{
	::CalcBound( pRes, resPoints, SGetSelf<CVec3>() );
}

void CMemObject::CalcBound( SBound *pRes ) const
{
	::CalcBound( pRes, resPoints, SGetSelf<CVec3>() );
}

REGISTER_SAVELOAD_CLASS( _3DLIB, 0x2012CD00, CMemObject )

