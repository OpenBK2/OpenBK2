#ifndef __SuperCollider_H_
#define __SuperCollider_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "VolumeContainer.h"
namespace NCollider
{
bool DoesTriSphereIntersect( const CVec3 &a1, const CVec3 &b1, const CVec3 &c1,
	const CVec3 &ptCenter, float fR );
////////////////////////////////////////////////////////////////////////////////////////////////////
// направленная прямая в пространстве Плюкера
struct SSegment
{
	float fC0, fC1, fC2, fC3, fC4, fC5; // координаты прямой в 6D пространстве Плюкера

	SSegment() {}
	SSegment( const CVec3 &p, const CVec3 &q )
	{
		CalcCoords( p, q );
	}
	// вычисление координат по двум точкам
	void CalcCoords( const CVec3 &p, const CVec3 &q )
	{
		fC0 = p.x*q.y-q.x*p.y;
		fC1 = p.x*q.z-q.x*p.z;
		fC2 = p.x-q.x;
		fC3 = p.y*q.z-q.y*p.z;
		fC4 = p.z-q.z;
		fC5 = q.y-p.y;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
inline float SegmentDotProduct( const SSegment &a, const SSegment &b )
{
	//return a.fC3*b.fC2+a.fC1*b.fC5+a.fC0*b.fC4;
  return a.fC0*b.fC4+a.fC1*b.fC5+a.fC2*b.fC3+a.fC3*b.fC2+a.fC4*b.fC0+a.fC5*b.fC1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class CSuperCollider
{
protected:
	struct SGlobalTriangle
	{
		int n1, n2, n3, nUserDataIndex;
		SPlane plane;
	};
	struct STriangleSegments
	{
		SSegment s1, s2, s3;
	};
	struct SVelocity
	{
		CVec3 vel, normVel;
		float fVel;
		
		SVelocity( const CVec3 &a ): vel(a), normVel(a) { fVel = fabs(vel); Normalize( &normVel ); }
	};
	vector<SGlobalTriangle> tris;
	vector<STriangleSegments> trisSegments; // CRAP - most of the time this data is not used
	vector<CVec3> points;
	CVolumeContainer volume;

	// returns distance to collision and triangle point to collide (pIntersect)
	float CollideSphereTriangle( const SSphere &sphere, const SVelocity &velocity,
		const SGlobalTriangle &tri, CVec3 *pIntersect );
	bool DoesTriSphereIntersect( const CVec3 &ptCenter, float fR, const SGlobalTriangle &tri );
	void AddEntity( const SHMatrix &pos, const vector<CVec3> &_points, const vector<STriangle> &tris, int nUserDataIndex );
public:
};
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class TUserData>
class CUserCollider: public CSuperCollider
{
	vector<TUserData> userData;
public:
	void PrepareCollider( const SBound &b, float fLeng )
	{
		points.clear();
		tris.clear();
		trisSegments.clear();
		userData.clear();
		volume.Init( b.s.ptCenter - b.ptHalfBox, b.s.ptCenter + b.ptHalfBox, fLeng );
	}
	void AddEntity( const SHMatrix &pos, const vector<CVec3> &_points, const vector<STriangle> &tris, const TUserData &ud )
	{
		int nUserDataIndex = userData.size();
		userData.push_back( ud );
		CSuperCollider::AddEntity( pos, _points, tris, nUserDataIndex );
	}
	template<class TResultAnalyzer>
	void Collide( const SSphere &sphere, const CVec3 &vel, TResultAnalyzer *pResult )
	{
		CVec3 ptCollision;
		SBound b;
		CVec3 ptHalfBox( fabs(vel.x) * 0.5f, fabs(vel.y) * 0.5f, fabs(vel.z) * 0.5f );
		b.BoxExInit( sphere.ptCenter + vel * 0.5f, ptHalfBox );
		b.Extend( sphere.fRadius );
		volume.Fetch( b );
		const vector<int> &fetchRes = volume.GetFetchBuffer();
		//nCollidedTris = fetchBufferArray.size();
		SVelocity velocity( vel );
		for ( int i = 0; i < volume.GetFetchedNum(); ++i )
		{
			const SGlobalTriangle &t = tris[ fetchRes[i] ];
			float fDist = CollideSphereTriangle( sphere, velocity, t, &ptCollision );
			if ( fDist == F_NO_COLLISION ) // not collides
				continue;
			if ( (*pResult)( fDist, ptCollision, t.plane, userData[ t.nUserDataIndex ] ) )
				break;
		}
	}

	template<class TResultAnalyzer>
		bool DoesIntersect( const CVec3 &ptCenter, float fR, TResultAnalyzer *pResult )
	{
		CVec3 ptCollision;
		SBound b;
		b.SphereInit( ptCenter, fR );
		volume.Fetch( b );
		const vector<int> &fetchRes = volume.GetFetchBuffer();
		for ( int i = 0; i < volume.GetFetchedNum(); ++i )
		{
			const SGlobalTriangle &t = tris[ fetchRes[i] ];
			if ( DoesTriSphereIntersect( ptCenter, fR, t ) )
			{
				if ( (*pResult)( userData[ t.nUserDataIndex ] ) )
					return true;
			}
		}
		return false;
	}
	bool DoesIntersect( const CVec3 &ptCenter, float fR )
	{
		return DoesIntersect( ptCenter, fR, &skipAnalyzer );
	}
};
}
#endif
