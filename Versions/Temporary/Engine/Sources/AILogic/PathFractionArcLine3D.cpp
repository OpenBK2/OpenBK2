#include "stdafx.h"
#include "./pathfractionarcline3d.h"
#include "PathFractionArc3D.h"
#include "PathFractionLine.h"
#include "DebugTools/DebugInfoManager.h"

REGISTER_SAVELOAD_CLASS(0x11095C41, CPathFractionArcLine3D)


#ifndef _FINALRELEASE
void DrawWhiteCross( const CVec3 &vPoint )
{
	if ( NGlobal::GetVar( "show_plane_path", 0 ) == 1 )
	{
		CSegment segm;
		segm.p1 = CVec2( vPoint.x + 10, vPoint.y + 10 );
		segm.p2 = CVec2( vPoint.x - 10, vPoint.y - 10 );
		segm.dir = segm.p2 - segm.p1;

		NDebugInfo::SArrowHead a1 ( vPoint, 100, 100 );
		NDebugInfo::SArrowHead a2 ( vPoint - V3_AXIS_Z * 10 );
		DebugInfoManager()->DrawLine( NDebugInfo::OBJECT_ID_GENERATE, a1, a2, NDebugInfo::WHITE );
	}
}
#endif


//	CPathFractionArcLine3D


void CPathFractionArcLine3D::GetSimplePath( CPathList *pPaths )
{
	if ( pArc )
		pPaths->push_back( pArc.GetPtr() );
	if ( pLine )
		pPaths->push_back( pLine.GetPtr() );
}

CPathFractionArc3D * CPathFractionArcLine3D::GetArc() 
{ 
	return pArc; 
}

IPathFraction * CPathFractionArcLine3D::GetLine() 
{ 
	return pLine; 
}

float CPathFractionArcLine3D::GetLength() const 
{ 
	return (pArc ? pArc->GetLength(): 0.0f) + (pLine? pLine->GetLength(): 0.0f); 
}

void CPathFractionArcLine3D::Init( const SPrevPathParams &prevPath, const CVec3 &x1, const float fR/*circle radius*/)
{
	Init( prevPath.p2, prevPath.vSpeed, x1, fR );
}

void CPathFractionArcLine3D::Init( const CVec3 &x0, const CVec3 &_v0, const CVec3 &x1, const float fR/*circle radius*/ )
{
	CVec3 v0 = _v0;
	Normalize( &v0 );

	pLine = new CPathFractionLine;

	CPtr<CPathFractionArc3D> pArc1, pArc2;

	CVec3 vT1, vT2;

	CVec3 vDir( x1 - x0 );
	Normalize( &vDir );
	if ( fabs( v0 - vDir ) > 0.05f  ) // the circle is needed
	{
		if ( TryCircle( x0, v0, x1, fR, 1, &vT1 ) )
			pArc1 = pArc;
		if ( TryCircle( x0, v0, x1, fR, -1, &vT2 ) )
			pArc2 = pArc;
	}

	if ( !pArc1 && !pArc2 )
	{
		pLine->Init( x0, x1 );
	}
	else if ( !pArc1 )
	{
		pArc = pArc2;
		pLine->Init( vT2, x1 );
	}
	else if ( !pArc2 )
	{
		pArc = pArc1;
		pLine->Init( vT1, x1 );
	}
	else if ( pArc1->GetLength() + fabs( vT1 - x1 ) > pArc2->GetLength() + fabs( vT2 -x1 ) )
	{
		pArc = pArc2;
		pLine->Init( vT2, x1 );
	}
	else
	{
		pArc = pArc1;
		pLine->Init( vT1, x1 );
	}

	if ( pArc && pLine->GetLength() < 10 )
		pLine = 0;
}

bool CPathFractionArcLine3D::TryCircle( const CVec3 &x0, const CVec3 &v0, const CVec3 &x1, const float fR, const int nDir, CVec3 *vT )
{
	CVec3 x0x1 = x1 - x0;									
	CVec3 _x0x1 = x1 - x0;									
	Normalize( &_x0x1 );
	// introduce new (_right_ if nDir == 1 or _left_ if nDir == -1 ) coordinate system ( i, j, k ), coordinate center (0,0) = x0.
	// i,y - manuver plane, k - normal to manuver plane

	// coordinate system first vector
	CVec3 i = v0;
	Normalize( &i );

	// k (normal to maneuver plane)
	CVec3 k = v0 ^ _x0x1;
	if ( fabs2( k ) < 1e-8f )
	{
		if ( i * x0x1 > 0 )
		{
			*vT = x0;
			pArc = 0;
			return true;
		}
		else
			k.z = 1;
	}

	Normalize( &k );

	// j ( perpendicular to x0, in manuver plane)
	CVec3 j = k ^ i * nDir;

	{
		pArc = new CPathFractionArc3D;

		// vR0 - vector from x0 to circle center
		// this vector must satisfy nonequation x1x0 * R0 > 0
		CVec3 vO = j * fR;
		CDirectedCircle circle;
		// create circle with center in x0+R0 with radius fR (in manuver plane)
		circle.r = fR;
		circle.center = CVec2( 0, fR ); // circle center transformed to local coordinate system

		// transform x1 to new coordiante system ( a, b - its new coordinates )
		//const CVec3 x1t( x0x1 * i, x0x1 * j, x0x1 * k );
		const CVec2 x1t( x0x1 * i, x0x1 * j );
		CVec2 t1t, t2t;													// tangent points to find (in new coordinate system)
		const bool bFound = FindTangentPoints( x1t, circle, &t1t, &t2t );
		if ( !bFound )
			return false;
		// choose needed tangent point

		const CVec2 vR1t( t1t - circle.center );						// transformed radius to tangent point 1
		const CVec2 vR2t( t2t - circle.center );						// transformed radius to tangent point 2
		const CVec2 vTangent1( x1t - t1t );			// tangent line 1
		const CVec2 vTangent2( x1t - t2t );			// tangent line 2

		const CVec2 vR0t( - circle.center );		// transformed radius vector of x0
		const CVec2 v0t( 1, 0 );

		// ---- correct up to there ---

		DRAW_WHITE_CROSS( x0 + CVec3( t1t.x * i + t1t.y * j ) );
		DRAW_WHITE_CROSS( x0 + CVec3( t2t.x * i + t2t.y * j ) );
		// circle rotation sign
		//const int nCircleSign = Sign( vR0t.y * v0t.x - vR0t.x * v0t.y ); // always -1

		// check if pair of ( vR1t & vTangent1 ) and ( v0 & R0 ) is of same allignment
		//DEBUG{
		int nS1 = Sign( vR1t.y * vTangent1.x - vR1t.x * vTangent1.y );
		int nS2 = Sign( vR2t.y * vTangent2.x - vR2t.x * vTangent2.y );
		//DEBUG}
		if ( Sign( vR1t.y * vTangent1.x - vR1t.x * vTangent1.y ) == -1 ) //nCircleSign )
		{
			// if it is so, then 1st tangent - is needed tangent
			const int nDiff = DirectedDirsDifference( vR0t, vR1t, -1 );//nCircleSign );
			*vT = i * t1t.x + j * t1t.y + x0;
			pArc->Init( i, j, k, circle, x0, *vT, fR * 2.0f * PI * nDiff / 65535.0f, nDiff );
		}
		else
		{
			// else, 2nd line is needed tangent
			const int nDiff = DirectedDirsDifference( vR0t, vR2t, -1 );//nCircleSign );
			*vT = i * t2t.x + j * t2t.y + x0;
			pArc->Init( i, j, k, circle, x0, *vT, fR * 2.0f * PI * nDiff / 65535.0f, nDiff );
		}
	}
	return true;
}

