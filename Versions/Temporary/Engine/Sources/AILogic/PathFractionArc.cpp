#include "StdAfx.h"
#include "./pathfractionarc.h"


REGISTER_SAVELOAD_CLASS(0x11095C40, CPathFractionArc)


//	CPathFractionArc


void CPathFractionArc::Init( const CDirectedCircle &_circle, const CVec2 &_x0, const CVec2 &_x1, const float _fZ )
{
	fZ = _fZ;
	circle = _circle;
	x0 = _x0;
	x1 = _x1;
	fLength = 1.0f * ( DirectedDirsDifference(x0 - circle.center, x1 - circle.center, circle.nDir) ) * 2* PI * circle.r / 65535;
}

CVec3 CPathFractionArc::GetNormale( const float fDist ) const
{
	const CVec3 vPoint( GetPoint( fDist ) );
	CVec3 vNormale( CVec3(circle.center, fZ) - vPoint );
	Normalize( &vNormale );
	return vNormale;
}

CVec3 CPathFractionArc::GetPoint( const float fDist ) const
{
	//CRAP{ WILL OPTIMIZE 
	const CVec2 vFromX( x0 );
	const CVec2 vFromR( vFromX - circle.center );
	const WORD wFrom = GetDirectionByVector( vFromR );
	//CRAP}

	const WORD wSingleAngle = fDist/ circle.r / ( 2.0f * PI ) * 65535;
	const WORD wTo = wFrom + circle.nDir * nOrientation * wSingleAngle;
	const CVec2 vToR( circle.r * GetVectorByDirection( wTo ) );

	return CVec3( circle.center + vToR, fZ );
}

CVec3 CPathFractionArc::GetTangent( const float fDist ) const
{
	//CRAP{ WILL OPTIMIZE
	const CVec2 vFromX( x0 );
	const CVec2 vFromR( vFromX - circle.center );
	const WORD wFrom = GetDirectionByVector( vFromR );
	//CRAP}

	const WORD wSingleAngle = fDist/ circle.r / ( 2.0f * PI ) * 65535;
	const WORD wTo = wFrom + circle.nDir * nOrientation * wSingleAngle;
	const CVec2 vToR( circle.r * GetVectorByDirection( wTo ) );

	return CVec3( -nOrientation * vToR.y * circle.nDir, nOrientation * vToR.x * circle.nDir, 0 );
}

