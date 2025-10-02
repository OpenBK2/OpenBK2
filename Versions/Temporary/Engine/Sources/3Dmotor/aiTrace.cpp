#include "StdAfx.h"
#include "aiTrace.h"
namespace NAI
{

// CTracer

void CTracer::InitProjection( const CVec3 &_ptOrigin, const CVec3 &_ptDir )
{
	CVec3 v = _ptDir, vAxis1, vAxis2;
	ASSERT( fabs2( v ) > 0 );
	if ( fabs(v.x) > fabs(v.y) && fabs(v.x) > fabs( v.z ) )
		vAxis1 = v ^ CVec3(0,1,0); 
	else
		vAxis1 = v ^ CVec3(1,0,0); 
	Normalize( &vAxis1 );
	vAxis2 = v ^ vAxis1;
	Normalize( &vAxis2 );
	ptOrig = _ptOrigin;
	ptDir = _ptDir;
	ptDirNormalized = ptDir;
	Normalize( &ptDirNormalized );
	ptAxis1 = CVec4( vAxis1.x, vAxis1.y, vAxis1.z, -(vAxis1 * ptOrig) );
	ptAxis2 = CVec4( vAxis2.x, vAxis2.y, vAxis2.z, -(vAxis2 * ptOrig) );
}

//! return true if ray intersects sphere
bool CTracer::TestSphere( const CVec3 &vSCenter, float fR )
{
	CVec3 v = (vSCenter - ptOrig) ^ ptDirNormalized;
	return fabs(v) <= fR;
}

void CTracer::InitProjection( const CRay &r )
{
	InitProjection( r.ptOrigin, r.ptDir );
}

static float CrossProduct( const CVec2 &a, const CVec2 &b )
{
	return a.x * b.y - a.y * b.x;
}

CTracer::SRefTriangle CTracer::GetTriangle( const SConvexHull &e, const SHMatrix &pos, const STriangle &t )
{
	int i1, i2, i3;
	if ( t.i1 & 0x8000 )
	{
		i1 = e.tris.edges[ t.i1 & 0x7fff ].wFinish; 
		i2 = e.tris.edges[ t.i1 & 0x7fff ].wStart;
	}
	else
	{
		i1 = e.tris.edges[ t.i1 & 0x7fff ].wStart; 
		i2 = e.tris.edges[ t.i1 & 0x7fff ].wFinish;
	}
	if ( t.i2 & 0x8000 )
		i3 = e.tris.edges[ t.i2 & 0x7fff ].wStart;
	else
		i3 = e.tris.edges[ t.i2 & 0x7fff ].wFinish;
	CVec3 v1, v2, v3;
	pos.RotateHVector( &v1, e.points[i1] );
	pos.RotateHVector( &v2, e.points[i2] );
	pos.RotateHVector( &v3, e.points[i3] );
	return SRefTriangle( v1, v2, v3 );
}

static CVec3 GetPlanePoint( const CVec3 &a, float f1, const CVec3 &a2, float f2 )
{
	return ( a2 * f1 - a * f2 ) / (f1 - f2);
}
SInterval::SCrossPoint CTracer::CalcCross( const SRefTriangle &t )
{
	const CVec3 &ptOnPlane = t.a;
	CVec3 ptNormal = (t.a2-t.a)^(t.a3-t.a);
	Normalize( &ptNormal );
	float fDenominator = ( ptDir * ptNormal );
	if ( fDenominator == 0 )
	{
		// trouble happened, calc distance to triangle in different way
		CVec3 ptSeparate = ptNormal ^ ptDir;
		float f0 = ptOrig * ptSeparate;
		float f1 = t.a * ptSeparate - f0;
		float f2 = t.a2 * ptSeparate - f0;
		float f3 = t.a3 * ptSeparate - f0;
		float fMinDistance = 1e30f, fD, fDirLeng = fabs( ptDir );
		ASSERT( fDirLeng > 0 );
		if ( f1 * f2 < 0 )
		{
			fD = GetPlanePoint( t.a, f1, t.a2, f2 ) * ptDir;
			fMinDistance = Min( fD, fMinDistance );
		}
		if ( f2 * f3 < 0 )
		{
			fD = GetPlanePoint( t.a2, f2, t.a3, f3 ) * ptDir;
			fMinDistance = Min( fD, fMinDistance );
		}
		if ( f3 * f1 < 0 )
		{
			fD = GetPlanePoint( t.a3, f3, t.a, f1 ) * ptDir;
			fMinDistance = Min( fD, fMinDistance );
		}
		if ( fMinDistance == 1e30f )
		{
			// мало того, что параллельный треугольник подсунули, 
			// так он еще и не пересекается
			ASSERT(0); 
			fMinDistance = t.a * ptDir;
		}
		fMinDistance = ( fMinDistance - ptOrig * ptDir ) / fDirLeng;
		return SInterval::SCrossPoint( fMinDistance, ptNormal );
	}
	return SInterval::SCrossPoint( ( (ptOnPlane - ptOrig) * ptNormal ) / fDenominator, ptNormal );
}

inline float Dot( const CVec3 &a, const CVec4 &b ) { return a.x * b.x + a.y * b.y + a.z * b.z + b.w; }
static float fTraceSign[] = {1,-1};
void CTracer::TraceEntity( const SConvexHull &e, vector<SInterval::SCrossPoint> *pEnter, vector<SInterval::SCrossPoint> *pExit )
{
	vector<SInterval::SCrossPoint> &enter = *pEnter;
	vector<SInterval::SCrossPoint> &exit = *pExit;
	// transform vertices
	vector<CVec2> transformed;
	vector<float> edgeSide;
	//
	transformed.resize( e.points.size() );
	//
	CVec4 vAxis1, vAxis2;
	e.trans.RotateHVectorTransposed( &vAxis1, ptAxis1 );
	e.trans.RotateHVectorTransposed( &vAxis2, ptAxis2 );
	for ( int i = 0; i < e.points.size(); ++i )
	{
		const CVec3 src = e.points[i];
		CVec2 &dst = transformed[i];
		dst.x = Dot( src, vAxis1 );
		dst.y = Dot( src, vAxis2 );
		if ( fabs2( dst ) < 1e-6 )
			dst.x = -1e-3f; // handle degenerate case via converting it to non degenerate :)
	}
	// calc cross products for edges
	edgeSide.resize( e.tris.edges.size() );
	for ( int i = 0; i < e.tris.edges.size(); i++ )
	{
		edgeSide[i] = CrossProduct( transformed[ e.tris.edges[i].wStart ], transformed[ e.tris.edges[i].wFinish ] );
		if ( edgeSide[i] == 0 )
			edgeSide[i] = 1e-6f; // same as with points, somebody has to win ;)
	}
	// check tris
	for ( vector<STriangle>::const_iterator i = e.tris.mesh.begin(); i != e.tris.mesh.end(); ++i )
	{
		float f1 = edgeSide[ i->i1 & 0x7fff ] * fTraceSign[ i->i1>>15 ];
		float f2 = edgeSide[ i->i2 & 0x7fff ] * fTraceSign[ i->i2>>15 ];
		float f3 = edgeSide[ i->i3 & 0x7fff ] * fTraceSign[ i->i3>>15 ];
		if ( f1 > 0 && f2 > 0 && f3 > 0 )
			exit.push_back( CalcCross( GetTriangle( e, e.trans, *i ) ) );
		if ( f1 < 0 && f2 < 0 && f3 < 0 )
			enter.push_back( CalcCross( GetTriangle( e, e.trans, *i ) ) );
	}
}

void CTracer::TraceEntity( const SConvexHull &e, bool bTerrain )
{
	vector<SInterval::SCrossPoint> enter;
	vector<SInterval::SCrossPoint> exit;
	TraceEntity( e, &enter, &exit );
	ASSERT( bTerrain || e.tris.bClosed );
	// fill intervals structure
	FillIntersectionResults( &intersections, &enter, &exit, e.src, e.nUserID, bTerrain );//!e.tris.bClosed );
}

void CTracer::TraceEntity( const vector<SConvexHull> &hulls, bool bTerrain )
{
	vector<SInterval::SCrossPoint> enter;
	vector<SInterval::SCrossPoint> exit;
	for ( int i = 0; i < hulls.size(); ++i )
		TraceEntity( hulls[i], &enter, &exit );
	// fill intervals structure
	if ( !hulls.empty() )
	{
		const SConvexHull &f = hulls[0];
		FillIntersectionResults( &intersections, &enter, &exit, f.src, f.nUserID, bTerrain );//!f.tris.bClosed );
	}
}

}
