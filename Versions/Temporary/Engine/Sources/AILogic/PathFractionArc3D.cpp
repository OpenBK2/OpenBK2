#include "StdAfx.h"
#include ".\pathfractionarc3d.h"
#include "../DebugTools/DebugInfoManager.h"

REGISTER_SAVELOAD_CLASS( 0x11095C02, CPathFractionArc3D )


//	CPathFractionArc3D


void CPathFractionArc3D::GetSimplePath( CPathList *pPaths )
{
	pPaths->push_back( this );
}

CVec3 CPathFractionArc3D::GetEndTangent() const
{
	return GetTangent( fLength );
}

CVec3 CPathFractionArc3D::GetPoint( const float _fDist ) const
{
	const float fDist = bNegative ? fLength - _fDist : _fDist;

	const CVec2 vt = GetVectorByDirection( GetDirectionByVector( -circle.center ) + nDiff * fDist / fLength ) * circle.r;
	return CVec3( vt.x * i + vt.y * j + x0 + j * circle.r );
}

CVec3 CPathFractionArc3D::GetNormale( const float __fDist ) const
{
	const float _fDist = Min( fLength, __fDist );
	const float fDist = bNegative ? fLength - _fDist : _fDist;

	const CVec2 vn( -GetVectorByDirection( GetDirectionByVector( -circle.center ) + nDiff * fDist / fLength ) );
	CVec3 vNormale = vn.x * i + vn.y * j;
	Normalize( &vNormale );
	return vNormale;
}

CVec3 CPathFractionArc3D::GetTangent( const float _fDist ) const
{
	const float fDist = bNegative ? fLength - _fDist : _fDist;

	const CVec2 vt = GetVectorByDirection( GetDirectionByVector( -circle.center ) + nDiff * fDist / fLength );
	return CVec3( - vt.y  * i + vt.x  * j );
}

CVec3 CPathFractionArc3D::GetStartTangent() const
{
	return GetTangent( 0.0f );
}

CVec3 CPathFractionArc3D::GetStartPoint() const
{
	return GetPoint( 0.0f );
}

CVec3 CPathFractionArc3D::GetEndPoint() const
{
	return GetPoint( fLength );
}

void CPathFractionArc3D::Init( const CVec3 &_i, const CVec3 &_j, const CVec3 &_k,
															const CDirectedCircle &_circle, const CVec3 &_x0, const CVec3 &_x1, const float _fLength, const WORD _nDiff )
{
	bNegative = false;
	i = _i;
	j = _j;
	k = _k;
	circle = _circle;
	x0 = _x0;
	x1 = _x1;
	fLength = _fLength;
	nDiff = _nDiff;
	/*
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_plane_path", 0 ) == 1 )
	{
		CSegment segm;
		CVec3 vPos( x0 + j * circle.r );
		segm.p1 = CVec2( vPos.x - 10, vPos.y - 10 );
		segm.p2 = CVec2( vPos.x + 10, vPos.y + 10 );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		segm.p1 = CVec2( vPos.x + 10, vPos.y - 10 );
		segm.p2 = CVec2( vPos.x - 10, vPos.y + 10 );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );

		vector<CVec3> points;
		for ( int nAngle = 0; nAngle < 65535; nAngle += 500 )
		{
			const CVec2 vt = GetVectorByDirection( GetDirectionByVector( -circle.center ) + nAngle  ) * circle.r;
			points.push_back( CVec3( vt.x * i + vt.y * j + x0 + j * circle.r ) );
		}
		for ( int i = 0; i < points.size() - 1; ++i )
		{
			CSegment segm;
			segm.p1 = CVec2( points[i].x, points[i].y );
			segm.p2 = CVec2( points[1+i].x, points[1+i].y );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
		}
	}
#endif
	*/
}

