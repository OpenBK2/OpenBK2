#include "StdAfx.h"
#include "SuperCollider.h"
namespace NCollider
{
////////////////////////////////////////////////////////////////////////////////////////////////////
bool DoesTriSphereIntersect( const CVec3 &a1, const CVec3 &b1, const CVec3 &c1,
	const CVec3 &ptCenter, float fR )
{
	// distance to vertices
	float fR2 = sqr( fR );
	CVec3 a = ptCenter - a1;
	if ( fabs2(a) < fR2 )
		return true;
	CVec3 b = ptCenter - b1;
	if ( fabs2(b) < fR2 )
		return true;
	CVec3 c = ptCenter - c1;
	if ( fabs2(c) < fR2 )
		return true;
	CVec3 sideAB = b1 - a1;
	CVec3 sideBC = c1 - b1;
	CVec3 sideCA = a1 - c1;
	float fAB2 = fabs2(sideAB);
	float fBC2 = fabs2(sideBC);
	float fCA2 = fabs2(sideCA);
	if ( fAB2 < 1e-6f )
	{
		if ( fBC2 < 1e-6f ) // degenerated triangle -> point
			return false;
		// only b-c edge exists
		float tBC = b * sideBC / fBC2;
		if ( tBC <= 0 || tBC >= 1 )
			return false;
		return ( fabs2( b - tBC * sideBC ) < fR2 );
	}
	if ( fBC2 < 1e-6f )
	{
		if ( fCA2 < 1e-6f ) // degenerated triangle -> point
			return false;
		// only c-a edge exists
		float tCA = c * sideCA / fCA2;
		if ( tCA <= 0 || tCA >= 1 )
			return false;
		return ( fabs2( c - tCA * sideCA ) < fR2 );
	}
	if ( fCA2 < 1e-6f )
	{
		if ( fAB2 < 1e-6f ) // degenerated triangle -> point
			return false;
		// only a-b edge exists
		float tAB = a * sideAB / fAB2;
		if ( tAB <= 0 || tAB >= 1 )
			return false;
		return ( fabs2( a - tAB * sideAB ) < fR2 );
	}
	// projections to edges - 0 = 1st vertex; 1 = 2nd vertex
	float tAB = a * sideAB / fAB2;
	float tBC = b * sideBC / fBC2;
	float tCA = c * sideCA / fCA2;
	// voronoi regions check for vertices
	if ( tAB >= 1 && tBC <= 0 )
		return false;
	if ( tBC >= 1 && tCA <= 0 )
		return false;
	if ( tCA >= 1 && tAB <= 0 )
		return false;
	CVec3 normal = sideCA ^ sideAB;
	float fLeng2 = fabs2( normal );
	if ( fLeng2 < 1e-6f ) // degenerated triangle -> segment, but no coinsiding points
		return ( fabs2( a - tAB * sideAB ) < fR2 );
	float fDist2 = sqr( a * normal );
	if ( fDist2 >= fR2 * fLeng2 ) // distance to triangle plane check
		return false;
	if ( (a ^ sideAB) * normal > 0 ) // projection outside of triangle
		return ( tAB > 0 && tAB < 1 && fabs2( a - tAB * sideAB ) < fR2 ); // edge cylinder
	if ( (b ^ sideBC) * normal > 0 ) // projection outside of triangle
		return ( tBC > 0 && tBC < 1 && fabs2( b - tBC * sideBC ) < fR2 ); // edge cylinder
	if ( (c ^ sideCA) * normal > 0 ) // projection outside of triangle
		return ( tCA > 0 && tCA < 1 && fabs2( c - tCA * sideCA ) < fR2 ); // edge cylinder
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CalcAplusBC( CVec3 *pRes, const CVec3 &a, float b, const CVec3 &c )
{
	pRes->x = a.x + b * c.x;
	pRes->y = a.y + b * c.y;
	pRes->z = a.z + b * c.z;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CalcDif( CVec3 *pRes, const CVec3 &a, const CVec3 &b )
{
	pRes->x = a.x - b.x;
	pRes->y = a.y - b.y;
	pRes->z = a.z - b.z;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void ClosestPointOnSegment( CVec3 *pRes, const CVec3 &p, const CVec3 &a, const CVec3 &b, const CVec3 &ba )
{
	// a-b is the line, p the point in question
	CVec3 c = p - a;
	const CVec3 &v = ba; 
	float d = fabs2(v);
	if ( d < 1e-6f )
	{
		*pRes = a;
		return;
	}
	float t = v * c;
	// Check to see if ‘t’ is beyond the extents of the line segment
	if (t < 0)
	{
		*pRes = a;
		return;
	}
	if (t > d)
	{
		*pRes = b;
		return;
	}
	CalcAplusBC( pRes, a, t/d, v );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CSuperCollider
////////////////////////////////////////////////////////////////////////////////////////////////////
// returns distance to collision and triangle point to collide (pIntersect)
float CSuperCollider::CollideSphereTriangle( const SSphere &sphere, const SVelocity &velocity,
	const SGlobalTriangle &tri, CVec3 *pIntersect )
{
	const SPlane &plane = tri.plane;
	float scal;
	scal = plane.n * velocity.normVel;
	if ( scal > -1e-6f )
		return F_NO_COLLISION;  // velocity parallel to plane or in opposite direction
	CVec3 spherePoint;
	CalcAplusBC( &spherePoint, sphere.ptCenter, -sphere.fRadius, plane.n );
	// calculate plane contact point
	CVec3 planePoint;
	float fDistToPlane = plane.GetDistanceToPoint( spherePoint );
	if ( fDistToPlane > -scal * velocity.fVel )
		return F_NO_COLLISION; // too far
	else if ( fDistToPlane < 0 )
		CalcAplusBC( &planePoint, spherePoint, -fDistToPlane, plane.n ); // embedded sphere
	else
		CalcAplusBC( &planePoint, spherePoint, -(fDistToPlane / scal), velocity.normVel );
	
	const CVec3 &v0 = points[tri.n1];
	const CVec3 &v1 = points[tri.n2];
	const CVec3 &v2 = points[tri.n3];
	// check whether we are inside triangle or not
	CVec3 p0, p1, p2, side01, side12, side20;
	CalcDif( &p0, planePoint, v0 );
	CalcDif( &p1, planePoint, v1 );
	CalcDif( &p2, planePoint, v2 );
	CalcDif( &side01, v1, v0 );
	CalcDif( &side12, v2, v1 );
	CalcDif( &side20, v0, v2 );
	CVec3 *pTriPoint = &planePoint;
	if ( (p0 ^ side01) * plane.n > 0 || (p1 ^ side12) * plane.n > 0 || (p2 ^ side20) * plane.n > 0 )
	{
		// calculate closest triangle point
		ClosestPointOnSegment( &p0, planePoint, v0, v1, side01 );
		ClosestPointOnSegment( &p1, planePoint, v1, v2, side12 );
		ClosestPointOnSegment( &p2, planePoint, v2, v0, side20 );
		float fMin = fabs2( p0 - planePoint );
		pTriPoint = &p0;
		float fDist = fabs2( p1 - planePoint );
		if ( fDist < fMin )
		{
			fMin = fDist;
			pTriPoint = &p1;
		}
		fDist = fabs2( p2 - planePoint );
		if ( fDist < fMin )
			pTriPoint = &p2;
	}
	CVec3 q;
	CalcDif( &q, *pTriPoint, sphere.ptCenter );
	float c = fabs2(q);
	float v = q * velocity.normVel;
	if ( v < 0 )
		return F_NO_COLLISION; // different direction
	float d = sphere.fRadius * sphere.fRadius - c + v * v;
	if ( d < 0 )
		return F_NO_COLLISION; // no intersection
	float fDistToSphere = v - sqrt(d);
	if ( fDistToSphere > velocity.fVel )
		return F_NO_COLLISION; // too far
	*pIntersect = *pTriPoint;
	return fDistToSphere;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSuperCollider::DoesTriSphereIntersect( const CVec3 &ptCenter, float fR, const SGlobalTriangle &tri )
{
	return NCollider::DoesTriSphereIntersect( points[tri.n1], points[tri.n2], points[tri.n3], ptCenter, fR );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSuperCollider::AddEntity( const SHMatrix &pos, const vector<CVec3> &_points, 
	const vector<STriangle> &_tris, int nUserDataIndex )
{
	int nStartIndex = points.size();
	points.resize( points.size() + _points.size() );
	vector<CVolumeContainer::SIntCoords> clipped;
	clipped.resize( _points.size() );
	for ( int k = 0; k < _points.size(); ++k )
	{
		CVec3 p;
		pos.RotateHVector( &p, _points[ k ] );
		points[ k + nStartIndex ] = p;
		volume.GetCoords( &clipped[k], p );
	}
	for ( int j = 0; j < _tris.size(); ++j )
	{
		const STriangle &t = _tris[j];
		SGlobalTriangle tout;
		STriangleSegments tsegout;
		tout.n1 = t.i1 + nStartIndex;
		tout.n2 = t.i2 + nStartIndex;
		tout.n3 = t.i3 + nStartIndex;
		tout.nUserDataIndex = nUserDataIndex;
		tsegout.s1.CalcCoords( points[tout.n1], points[tout.n2] );
		tsegout.s2.CalcCoords( points[tout.n2], points[tout.n3] );
		tsegout.s3.CalcCoords( points[tout.n3], points[tout.n1] );
		CVolumeContainer::SVolumeBounds bounds;
		volume.MakeVolume( &bounds, clipped[t.i1], clipped[t.i2], clipped[t.i3] );
		if ( volume.IsOut( bounds ) )
			continue;
		if ( !tout.plane.Set( points[tout.n1], points[tout.n2], points[tout.n3], true ) )
			continue; // bad triangle
		volume.ClipVolume( &bounds );
		int nTri = tris.size();
		tris.push_back( tout );
		trisSegments.push_back( tsegout );
		ASSERT( tris.size() == trisSegments.size() );
		volume.Add( bounds, nTri );			
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
