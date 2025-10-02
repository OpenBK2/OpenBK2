#include "stdafx.h"
#include "../System/FastMath.h"
#include "AIGeometry.h"

const float GetDistanceToSegment( const CVec2 &vSegmentStart, const CVec2 &vSegmentEnd, const CVec2 &vPoint )
{
	CLine2 line( vSegmentStart, vSegmentEnd );
	
	CVec2 vNormal;
	line.ProjectPoint( vPoint, &vNormal );

	const float fDiff1 = fabs( vSegmentStart - vNormal );
	const float fDiff2 = fabs( vSegmentEnd - vNormal );
	const float fDiff3 = fabs( vSegmentEnd - vSegmentStart );
	
	if ( fDiff3 < fDiff2 + fDiff1 ) // нормаль от точки не падает на отрезок
	{
		const float fDist1 = fabs( vSegmentStart - vPoint );
		const float fDist2 = fabs( vSegmentEnd - vPoint );
		return Min( fDist1, fDist2 );
	}
	else 
	{
		return fabs( vNormal - vPoint );
	}
}

void MakeQuatBySpeedAndNormale( CQuat *pQuat, const CVec3 &vSpeed, const CVec3 &vNormale )
{
	// unit's coordinate system  ( M,N,O )
	CVec3 vN( vSpeed );
	CVec3 vO( vNormale );
	CVec3 vM( vN ^ vO );

	Normalize( &vO );
	Normalize( &vN );
	Normalize( &vM );
/**/
	// calculate euler matrix (transform from world to unit coordinate system)
	SHMatrix m(
		vM.x, vN.x, vO.x, 0,
		vM.y, vN.y, vO.y, 0,
		vM.z, vN.z, vO.z, 0,
		0,		0,		0,		1 );
/**/
/**
	SHMatrix m(
		vM.x, vM.y, vM.z, 0,
		vN.x, vN.y, vN.z, 0,
		vO.x, vO.y, vO.z, 0,
		0,		0,		0,		1 );
	/**/	
	pQuat->FromEulerMatrix( m );
	//pQuat->Negate();
}

const CVec2 MoveVectorByDirection( const CVec2 &vPoint, WORD wDir )
{
	/*vRes = GetVectorByDirection( wDir );
	vRes.Set( vPoint.x * vRes.x - vPoint.y * vRes.y, vPoint.x * vRes.y + vPoint.y * vRes.x );
	return vRes;*/
	const float fAngle = wDir * FP_2PI / 65535.0f;
	const float fSin = NMath::Sin( fAngle );
	const float fCos = NMath::Cos( fAngle );
	return CVec2( vPoint.x * fCos - vPoint.y * fSin, vPoint.x * fSin + vPoint.y * fCos );
}


