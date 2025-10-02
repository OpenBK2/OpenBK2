#include "StdAfx.h"
#include "./pathfractioncomplex.h"
#include "IPlane.h"
#include "Manuver.h"

REGISTER_SAVELOAD_CLASS( 0x11097A80, CPathFractionComplex )



//	SPrevManuverParams


SPrevPathParams::SPrevPathParams( interface IPlane *pPlane )
{
	if ( pPlane->GetManuver() == 0 )
	{
		vSpeed = pPlane->GetSpeedB2();
		Normalize( &vSpeed );
		
		p2 = pPlane->GetPosB2();
		vCur = p1 = p2 - vSpeed * SConsts::PLANE_SPLINE_POINT_DIST;
		p0 = p1 - vSpeed * SConsts::PLANE_SPLINE_POINT_DIST;
		fDistToGo = 0;
		fSplineProgress = 0.0f;
		vNormale = V3_AXIS_Z;
		fCurTiltSpeed = 0.0f;
	}
	else
		pPlane->GetManuver()->GetManuverParams( this );
}


//	CPathFractionComplexBase

CPathFractionComplexBase::CPathFractionComplexBase( const CPathList &pathList )
{
	for ( CPathList::const_iterator it = pathList.begin(); it != pathList.end(); ++it )
	{
		IPathFraction *pPathFraction = *it;
		if ( IsValid( pPathFraction ) )
			substitute.push_back( pPathFraction );
	}
}

CCurFraction CPathFractionComplexBase::GetCur( const float fDist ) const
{
	if ( !substitute.empty() )
	{
		float fDistSoFar = 0;
		float fTmp = 0;

		for ( CSubstitutes::const_iterator it = substitute.begin(); it != substitute.end(); ++it )
		{
			IPathFraction *pPathFraction = *it;
			NI_VERIFY( IsValid( pPathFraction ), "Invalid path fraction", continue );
			const float fTmp = fDistSoFar;
			fDistSoFar += pPathFraction->GetLength();
			if ( fDistSoFar > fDist )
				return CCurFraction( pPathFraction, fTmp );
		}
		return CCurFraction( substitute.back(), fTmp );
	}
	return CCurFraction( 0, 0 );
}

void CPathFractionComplexBase::AfterSubstitute()
{
	for( CSubstitutes::iterator it = substitute.begin(); it != substitute.end(); )
	{
		if ( (*it)->GetLength() == 0 )
			it = substitute.erase( it );
		else
			++it;
	}
}

float CPathFractionComplexBase::GetLength() const
{
	float fDistSoFar = 0;
	for ( CSubstitutes::const_iterator it = substitute.begin(); it < substitute.end(); ++it )
	{
		fDistSoFar += (*it) ? (*it)->GetLength() : 0.0f ;
	}
	return fDistSoFar;
}

CVec3 CPathFractionComplexBase::GetPoint( const float fDist ) const
{
	CCurFraction fr = GetCur( fDist );
	if ( fr.first )
		return fr.first->GetPoint( fDist - fr.second );
	return VNULL3;
}

CVec3 CPathFractionComplexBase::GetTangent( const float fDist ) const
{
	CCurFraction fr = GetCur( fDist );
	if ( fr.first )
		return fr.first->GetTangent( fDist - fr.second );
	return VNULL3;
}

CVec3 CPathFractionComplexBase::GetNormale( const float fDist ) const
{
	CCurFraction fr = GetCur( fDist );
	if ( fr.first )
		return fr.first->GetNormale( fDist - fr.second );
	return V3_AXIS_Z;
}

