#include "stdafx.h"

#include "Misc/2Darray.h"
#include "System/FastMath.h"
#include "TerrUtils.h"

CVec3dEx CVec3fEx::GetVec3dEx() const { return CVec3dEx( (double)x, (double)y, (double)z, flag ); }
CVec3fEx CVec3dEx::GetVec3fEx() const { return CVec3fEx( (float)x, (float)y, (float)z, flag ); }

void AttachIntersection( vector<NDb::SVSOPoint> *pIntersection, const vector<NDb::SVSOPoint> &points, const bool bSetFlag )
{
	if ( pIntersection->size() < 2 )
		return;

	static vector<SIntersectPoint> inters(16);
	static vector<NDb::SVSOPoint> newPoints(128);

	newPoints.resize( 0 );

	vector<NDb::SVSOPoint>::const_iterator itCurPoint = pIntersection->begin();
	vector<NDb::SVSOPoint>::const_iterator itNextPoint = itCurPoint;
	++itNextPoint;

	NDb::SVSOPoint addPoint;
	addPoint.fWidth = bSetFlag ? 1.0f : -1.0f;

	for ( ; itNextPoint != pIntersection->end(); ++itCurPoint, ++itNextPoint )
	{
		inters.resize( 0 );
		GetIntersection( &inters, points, itCurPoint->vPos, itNextPoint->vPos );
		newPoints.push_back( *itCurPoint );
		sort( inters.begin(), inters.end() );
		for ( vector<SIntersectPoint>::const_iterator it = inters.begin(); it != inters.end(); ++it )
		{
			addPoint.vPos = it->vPoint;
			addPoint.vNorm = itCurPoint->vNorm + ( itNextPoint->vNorm - itCurPoint->vNorm ) * it->fDist;
			//addPoint.fWidth = -1.0f;
			//newPoints.push_back( addPoint );
			newPoints.push_back( addPoint );
			//addPoint.fWidth = -1.0f;
			//newPoints.push_back( addPoint );
		}
	}

	itCurPoint = pIntersection->end();
	--itCurPoint;
	newPoints.push_back( *itCurPoint );

	pIntersection->swap( newPoints );
}


bool IsInside( const vector<CVec3dEx> &rPoly, const CVec3dEx &rV, bool bIncludeBorders )
{
	int nLeftInters = 0, nRightInters = 0;
	double fFirstX = -FP_MAX_VALUE, fLastX = -FP_MAX_VALUE;
	for ( int i = 0; i < rPoly.size(); ++i )
	{
		const CVec3dEx &v1 = rPoly[i];
		const CVec3dEx &v2 = ( i < (rPoly.size() - 1) ) ? rPoly[i + 1] : rPoly[0];
		// find horizontal intersection
		if ( fabs(v2.y - v1.y) > DEF_EPS )
		{
			const double t = (double)( rV.y - v1.y ) / ( v2.y - v1.y );
			// if point is placed on current segment
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) )
			//if ( ( t >= 0.0f ) && ( t <= 1.0 ) )
			{
				const double x = (double)v1.x + ( v2.x - v1.x ) * t;
				if ( fabs(x - rV.x) < DEF_EPS ) // point is placed on the border
					return bIncludeBorders;
				if ( i == 0 )
				{
					fFirstX = fLastX = x;
				}
				else
				{
					if ( fabs(x - fLastX) < DEF_EPS )
						continue;
					if ( i == (rPoly.size() - 1) )
					{
						if ( fabs(x - fFirstX) < DEF_EPS )
							continue;
					}
				}
				fLastX = x;
				if ( x > rV.x )
					++nRightInters;
				else
					++nLeftInters;
			}
			else
				fLastX = -FP_MAX_VALUE;
		}
		else
		{
			if ( (fabs(v1.y - rV.y) < DEF_EPS) && (rV.x >= min(v1.x, v2.x)) && (rV.x <= max(v1.x, v2.x)) )
				return false;
			fLastX = -FP_MAX_VALUE;
		}
	}
	return ( (nLeftInters & 1) && (nRightInters & 1) );
}

// whether "rV" is outside of "rPoly"
bool IsOutside( const vector<CVec3dEx> &rPoly, const CVec3dEx &rV )
{
	//return (!IsInside(rPoly, rV));	/// why not?

	int nLeftInters = 0, nRightInters = 0;
	double fFirstX = -FP_MAX_VALUE, fLastX = -FP_MAX_VALUE;
	for ( int i = 0; i < rPoly.size(); ++i )
	{
		const CVec3dEx &v1 = rPoly[i];
		const CVec3dEx &v2 = ( i < (rPoly.size() - 1) ) ? rPoly[i + 1] : rPoly[0];
		// find horizontal intersection
		if ( fabs(v2.y - v1.y) > DEF_EPS )
		{
			const double t = (double)( rV.y - v1.y ) / ( v2.y - v1.y );
			// if point is placed on current segment
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) )
			{
				const double x = (double)v1.x + ( v2.x - v1.x ) * t;
				if ( fabs(x - rV.x) < DEF_EPS ) // point is placed on the border
					return false;
				if ( i == 0 )
				{
					fFirstX = fLastX = x;
				}
				else
				{
					if ( fabs(x - fLastX) < DEF_EPS )
						continue;
					if ( i == (rPoly.size() - 1) )
					{
						if ( fabs(x - fFirstX) < DEF_EPS )
							continue;
					}
				}
				fLastX = x;
				if ( x > rV.x )
					++nRightInters;
				else
					++nLeftInters;
			}
			else
				fLastX = -FP_MAX_VALUE;
		}
		else
		{
			if ( (fabs(v1.y - rV.y) < DEF_EPS) && (rV.x >= min(v1.x, v2.x)) && (rV.x <= max(v1.x, v2.x)) )
				return false;
			fLastX = -FP_MAX_VALUE;
		}
	}
	return ( (!(nLeftInters & 1)) || (!(nRightInters & 1)) );
}

void GetIntersection( vector<CVec3dEx> *pIntersection, const vector<CVec3dEx> &rPoly, const CVec3dEx &rV1, const CVec3dEx &rV2 )
{
	for ( int i = 0; i < rPoly.size(); ++i )
	{
		const CVec3dEx &v1 = rPoly[i];
		const CVec3dEx &v2 = ( i < (rPoly.size() - 1) ) ? rPoly[i + 1] : rPoly[0];
		const double fDet = (double)( rV2.x - rV1.x ) * ( v2.y - v1.y ) - ( rV2.y - rV1.y ) * ( v2.x - v1.x );
		if ( fabs(fDet) > DEF_EPS )
		{
			const double fInvDet = 1.0 / fDet;
			const double t = ( (double)(v1.x - rV1.x) * (v2.y - v1.y) - (v1.y - rV1.y) * (v2.x - v1.x) ) * fInvDet;
			const double k = ( (double)(v1.x - rV1.x) * (rV2.y - rV1.y) - (v1.y - rV1.y) * (rV2.x - rV1.x) ) * fInvDet;
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
			{
				const double fHeight = v1.z + ( v2.z - v1.z ) * k;
				//const float fHeight = 0;
				PushBackUnique( pIntersection, CVec3dEx(rV1.x + (rV2.x - rV1.x) * t, rV1.y + (rV2.y - rV1.y) * t, fHeight, 1) );
			}
		}
	}
}

void GetIntersection( vector<SIntersectPoint> *pIntersection, const vector<CVec3fEx> &rPoly, const CVec3 &rV1, const CVec3 &rV2 )
{
	for ( int i = 0; i < rPoly.size(); ++i )
	{
		const CVec3fEx &v1 = rPoly[i];
		const CVec3fEx &v2 = ( i < (rPoly.size() - 1) ) ? rPoly[i + 1] : rPoly[0];
		const float fDet = ( rV2.x - rV1.x ) * ( v2.y - v1.y ) - ( rV2.y - rV1.y ) * ( v2.x - v1.x );
		if ( fabs(fDet) > DEF_EPS )
		{
			const float fInvDet = 1.0 / fDet;
			const float t = ( (v1.x - rV1.x) * (v2.y - v1.y) - (v1.y - rV1.y) * (v2.x - v1.x) ) * fInvDet;
			const float k = ( (v1.x - rV1.x) * (rV2.y - rV1.y) - (v1.y - rV1.y) * (rV2.x - rV1.x) ) * fInvDet;
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
			{
				const float fHeight = v1.z + ( v2.z - v1.z) * k;
				//const float fHeight = 0;
				pIntersection->push_back( SIntersectPoint(CVec3(rV1.x + (rV2.x - rV1.x) * t, rV1.y + (rV2.y - rV1.y) * t, fHeight), t) );
			}
		}
	}
}

void GetIntersection( vector<SIntersectPoint> *pIntersection, const vector<NDb::SVSOPoint> &rPoly, const CVec3 &rV1, const CVec3 &rV2 )
{
	if ( rPoly.size() < 2 )
		return;

	vector<NDb::SVSOPoint>::const_iterator itVec1 = rPoly.begin();
	vector<NDb::SVSOPoint>::const_iterator itVec2 = itVec1;
	++itVec2;
	for ( ; itVec2 != rPoly.end(); ++itVec1, ++itVec2 )
	{
		const float fDet = ( rV2.x - rV1.x ) * ( itVec2->vPos.y - itVec1->vPos.y ) - ( rV2.y - rV1.y ) * ( itVec2->vPos.x - itVec1->vPos.x );
		if ( fabs( fDet ) > DEF_EPS )
		{
			const float fInvDet = 1.0 / fDet;
			const float t = ( (itVec1->vPos.x - rV1.x) * (itVec2->vPos.y - itVec1->vPos.y) - (itVec1->vPos.y - rV1.y) * (itVec2->vPos.x - itVec1->vPos.x) ) * fInvDet;
			const float k = ( (itVec1->vPos.x - rV1.x) * (rV2.y - rV1.y) - (itVec1->vPos.y - rV1.y) * (rV2.x - rV1.x) ) * fInvDet;
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
			{
				const CVec3 v1( itVec1->vPos );
				const CVec3 v2( itVec2->vPos );

				const float fHeight = v1.z + ( v2.z - v1.z ) * k;
				//const float fHeight = 0;

				CVec3 vIntersection( rV1.x + (rV2.x - rV1.x) * t, rV1.y + (rV2.y - rV1.y) * t, fHeight );
				pIntersection->push_back( SIntersectPoint(vIntersection, t) );
			}
		}
	}
}

void GetBorderIntersection( vector<CVec3> *pBorderIntersection, const CTriangleEx &rTriangle1, const CTriangleEx &rTriangle2 )
{
	for ( int g = 0; g < 3; ++g )
	{
		const CVec3dEx &r1v1 = rTriangle1.points[g];
		const CVec3dEx &r1v2 = g < 2 ? rTriangle1.points[g + 1] : rTriangle1.points[0];
		for ( int i = 0; i < 3; ++i )
		{
			const CVec3dEx &r2v1 = rTriangle2.points[i];
			const CVec3dEx &r2v2 = i < 2 ? rTriangle2.points[i + 1] : rTriangle2.points[0];
			const double fDet = (double)( r1v2.x - r1v1.x ) * ( r2v2.y - r2v1.y ) - ( r1v2.y - r1v1.y ) * ( r2v2.x - r2v1.x );
			if ( fabs(fDet) > DEF_EPS )
			{
				const double fInvDet = 1.0 / fDet;
				const double t = ( (double)(r2v1.x - r1v1.x) * (r2v2.y - r2v1.y) - (r2v1.y - r1v1.y) * (r2v2.x - r2v1.x) ) * fInvDet;
				const double k = ( (double)(r2v1.x - r1v1.x) * (r1v2.y - r1v1.y) - (r2v1.y - r1v1.y) * (r1v2.x - r1v1.x) ) * fInvDet;
				if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
				{
					//const float fHeight = r2v1.z + ( r2v2.z - r2v1.z ) * k;
					const float fHeight = 0;
					AddUnique( pBorderIntersection, CVec3( float(r1v1.x + (r1v2.x - r1v1.x) * t),
																								 float(r1v1.y + (r1v2.y - r1v1.y) * t),
																								 fHeight) );
				}
			}
		}
	}
}

//	whether the "rV1-rV2" segment does intersect the "rPoly"
bool IsIntersect( const vector<CVec3dEx> &rPoly, const CVec3dEx &rV1, const CVec3dEx &rV2 )
{
	for ( int i = 0; i < rPoly.size(); ++i )
	{
		const CVec3dEx &vert1 = rPoly[i];
		const CVec3dEx &vert2 = ( i < (rPoly.size() - 1) ) ? rPoly[i + 1] : rPoly[0];
		const double fDet = (double)( rV2.x - rV1.x ) * ( vert2.y - vert1.y ) - ( rV2.y - rV1.y ) * ( vert2.x - vert1.x );
		if ( fabs(fDet) > DEF_EPS )
		{
			const double fInvDet = 1.0 / fDet;
			const double t = ( (double)(vert1.x - rV1.x) * (vert2.y - vert1.y) - (vert1.y - rV1.y) * (vert2.x - vert1.x) ) * fInvDet;
			const double k = ( (double)(vert1.x - rV1.x) * (rV2.y - rV1.y) - (vert1.y - rV1.y) * (rV2.x - rV1.x) ) * fInvDet;
			if ( (t > DEF_EPS) && (t < (1.0 - DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
				return true;
		}
	}
	return false;
}

//	whether the "rV1-rV2" segment does intersect the mesh triangle
bool IsIntersect( const vector<STriangle> &rTriangles, const vector<CVec3dEx> &rVerts, const CVec3dEx &rV1, const CVec3dEx &rV2 )
{
	for ( vector<STriangle>::const_iterator it = rTriangles.begin(); it != rTriangles.end(); ++it )
	{
		if ( IsIntersect(rVerts[it->i1], rVerts[it->i2], rV1, rV2) )
			return true;
		if ( IsIntersect(rVerts[it->i2], rVerts[it->i3], rV1, rV2) )
			return true;
		if ( IsIntersect(rVerts[it->i3], rVerts[it->i1], rV1, rV2) )
			return true;
	}
	return false;
}

static vector<BYTE> flags( 64 );
static CArray2D<CVec3> normVects( 64, 64 );
void CreateConvexHull( vector<CVec3> *pResPoints, const vector<CVec3> &rSourcePoints )
{
	pResPoints->resize( 0 );
	if ( rSourcePoints.empty() )
		return;
	flags.resize( rSourcePoints.size() );
	fill( flags.begin(), flags.end(), 0 );
	normVects.SetSizes( rSourcePoints.size(), rSourcePoints.size() );

	// create normalize vectors
	for ( int g = 0; g < rSourcePoints.size(); ++g )
	{
		for ( int i = g + 1; i < rSourcePoints.size(); ++i )
		{
			//NI_ASSERT( fabs((src[g].x - src[i].x) * (src[g].x - src[i].x) + ( src[g].y - src[i].y ) * ( src[g].y - src[i].y ) ) > DEF_EPS2,
			//	"CreateConvexHull: source vector contains duplicate points" );
			const float d = NMath::Sqrt( (rSourcePoints[g].x - rSourcePoints[i].x) * (rSourcePoints[g].x - rSourcePoints[i].x) +
																	 (rSourcePoints[g].y - rSourcePoints[i].y) * (rSourcePoints[g].y - rSourcePoints[i].y) );
			const float fDist = ( fabs( d ) > DEF_EPS ) ? ( 1.0f / d ) : 1.0f;
			normVects[g][i].x = ( rSourcePoints[g].x - rSourcePoints[i].x ) * fDist;
			normVects[g][i].y = ( rSourcePoints[g].y - rSourcePoints[i].y ) * fDist;
			normVects[i][g] = -normVects[g][i];
		}
	}

	CVec3 vPrev( -1, 0, 0 );

	// find lower point
	unsigned int nFirstPnt = 0;
	for ( int i = 1; i < rSourcePoints.size(); ++i )
	{
		if ( rSourcePoints[i].y < rSourcePoints[nFirstPnt].y )
			nFirstPnt = i;
	}

	pResPoints->resize( 0 );
	pResPoints->push_back( rSourcePoints[nFirstPnt] );
	flags[nFirstPnt] = 1;

	// create convex hull
	int bFlag = true;
	int nPrev = nFirstPnt;
	int nNext = -1;
	float fMinAng, fAng;
	while ( bFlag )
	{
		bFlag = false;
		fMinAng = -2.0f;
		for ( int i = 0; i < rSourcePoints.size(); ++i )
		{
			if ( (i != nPrev) && (!flags[i]) )
			{
				if ( (fAng = normVects[i][nPrev].x * vPrev.x + normVects[i][nPrev].y * vPrev.y) > fMinAng )
				{
					fMinAng = fAng;
					nNext = i;
					bFlag = true;
				}
			}
		}
		if ( bFlag )
		{
			vPrev = normVects[nNext][nPrev];
			nPrev = nNext;
			pResPoints->push_back( rSourcePoints[nNext] );
			flags[nNext] = 1;
		}
	}
}

//	get the result triangles of the intersection
void GetIntersectionTriangles( vector<CTriangleEx> *pIntersection, const CTriangleEx &rTriangle1, const CTriangleEx &rTriangle2 )
{
	pIntersection->clear();

	vector<CVec3> intersectionPoints( 6 );
	intersectionPoints.clear();
	GetBorderIntersection( &intersectionPoints, rTriangle1, rTriangle2 );

	if ( intersectionPoints.size() < 1 )	// the border intersection is void
	{
		//	whether the triangles are taken_up
		//	it's enough to check single points
		if ( IsInsideTriangle(rTriangle1, rTriangle2.points[0]) )
		{
			//	the rTriangle1 covers rTriangle2
			pIntersection->push_back( rTriangle2 );
		}
		else if ( IsInsideTriangle(rTriangle2, rTriangle1.points[0]) )
		{
			//	the rTriangle2 covers rTriangle1
			pIntersection->push_back( rTriangle1 );
			return;
		}
		else
		{
			//	the intersection is void
			return;
		}
	}
	else
	{
		//	need to calculate real intersection
		//	let it be full for the first time
		pIntersection->push_back( rTriangle1 );
		pIntersection->push_back( rTriangle2 );

		return;
	}
}

bool IsInsideTriangle( const CTriangleEx &rTriangle, const CVec3dEx &rPoint, bool bIncludeBorders )
{
	//	{CRAP !!! create faster method !!! /CRAP}
	//	what about CW/CCW ?
	
	vector<CVec3dEx> trianglePoly( 3 );
	trianglePoly.clear();

	trianglePoly.push_back( rTriangle.points[0].GetVec3fEx() );
	trianglePoly.push_back( rTriangle.points[1].GetVec3fEx() );
	trianglePoly.push_back( rTriangle.points[2].GetVec3fEx() );

	return IsInside( trianglePoly, rPoint, bIncludeBorders );
}

bool AreTrianglesTakenUp( const CTriangleEx rOuterTriangle, const CTriangleEx rInnerTriangle, bool bIncludeBorders )
{
	bool bCovers = true;

	bCovers = bCovers && IsInsideTriangle( rOuterTriangle, rInnerTriangle.points[0], bIncludeBorders );
	bCovers = bCovers && IsInsideTriangle( rOuterTriangle, rInnerTriangle.points[1], bIncludeBorders );
	bCovers = bCovers && IsInsideTriangle( rOuterTriangle, rInnerTriangle.points[2], bIncludeBorders );

	return bCovers;
}


