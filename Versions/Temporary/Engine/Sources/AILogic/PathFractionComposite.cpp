#include "StdAfx.h"
#include "./pathfractioncomposite.h"
#include "IPlane.h"
#include "DebugTools/DebugInfoManager.h"

REGISTER_SAVELOAD_CLASS( 0x11097AC0, CPathFractionComposite )


void CPathFractionComposite::GetPrevPoints( struct SPrevPathParams *pParams ) const
{
	pParams->p0 = p0;
	pParams->p1 = p1;
	pParams->p2 = p2; 
	pParams->vNormale = vNormale;

/*
	if ( nIndex >= points.size() - 2 )
	{
		pParams->p0 = p0;
		pParams->p1 = p1;
		pParams->p2 = p2; 
		pParams->vNormale = vNormale;
	}
	else
	{
		pParams->p0 = p1;
		pParams->p1 = p2;
		pParams->p2 = p3;
		pParams->vNormale = vNormale;
	}
*/
	pParams->fSplineProgress = fSplineProgress;
	pParams->vCur = vPos;
	pParams->vSpeed = vSpeed;
}

CPathFractionComposite::CPathFractionComposite( struct IPlane *pPlane, struct IPathFraction *pPath )
: vSpeed ( pPlane->GetSpeedB2() ),
	vNormale( pPlane->GetNormalB2() ),
	pInitialPath( pPath ),
	nIndex( 0 ), fSplineProgress( 0 )
{
	Normalize( &vSpeed );
	CPathList pathList;
	pInitialPath->GetSimplePath( &pathList );

	// ASK pPlane for these values
	SPrevPathParams param( pPlane );
	p0 = param.p0;
	p1 = param.p1;
	p2 = param.p2;
	
	substitute.insert( substitute.end(), pathList.begin(), pathList.end() );

	// substitute every N points with spline path
	float fAtom = SConsts::PLANE_SPLINE_POINT_DIST;
	fPathLenght = CPathFractionComplexBase::GetLength();
	float fCur = fAtom;

	// progressive diminishing for fAtom
	if ( fCur * 6 >= fPathLenght )
	{
		fAtom = fPathLenght / 7;
		fCur = fAtom;
	}

	for ( ; fCur < fPathLenght; fCur += fAtom )
	{
		const CVec3 vPoint( CPathFractionComplexBase::GetPoint( fCur ) );
		const CVec3 vNormale( CPathFractionComplexBase::GetNormale( fCur ) );
		points.push_back( vPoint );
		normales.push_back( vNormale );
	}

	points.push_back( CPathFractionComplexBase::GetPoint( fPathLenght ) );
	normales.push_back( CPathFractionComplexBase::GetNormale( fPathLenght ) );

	vPos = param.vCur;
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_plane_path", 0 ) == 1 )
	{
		for ( int i = 0; i < points.size() - 1; ++i )
		{
			CSegment segm;
			NDebugInfo::SArrowHead a1( points[i] );
			NDebugInfo::SArrowHead a2( points[i+1] );
			NDebugInfo::SArrowHead a3( points[i] + normales[i] * 100, 50, 5 );
			DebugInfoManager()->DrawLine( NDebugInfo::OBJECT_ID_GENERATE, a1, a2, NDebugInfo::BLUE );
			DebugInfoManager()->DrawLine( NDebugInfo::OBJECT_ID_GENERATE, a1, a3, NDebugInfo::WHITE );
		}
	}
#endif

	if ( points.empty() )
	{
		bFinished = true;
		return;
	}
	else
		bFinished = false;

	p3 = points[0];

	vNormale = normales[0];
	spline.Setup( p0, p1, p2, p3 );
	
	fSplineProgress = param.fSplineProgress;
	float fDistRemain;
	vPosLastPoint = vPos;					
	vSpeedLastPoint = vSpeed;
	vNormaleLastPoint = vNormaleLastPoint;

	Iterate( param.fDistToGo, &fDistRemain );
	// draw path points
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "plane_path_markers", 0 ) )
	{
		CSegment segm;
		for ( int i = 0; i < points.size() -1; ++i )
		{
			segm.p1 = CVec2( points[i].x, points[i].y );
			segm.p2 = CVec2( points[i+1].x, points[i+1].y );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
		}
	}
#endif
}

bool CPathFractionComposite::Iterate( const float fDist, float *pfDistRemain )
{
	if ( bFinished )
		return true;

	const float fDistToGo = fDist;

	while( true )
	{
		// iterate one step trough spline

		CVec3 vNew;
		float fPassed;

		if ( fSplineProgress + SConsts::PLANE_SPLINE_STEP >= 1.0f ) // spline finished
		{
			++nIndex;
			// get next point, init spline, iterate trough it
			if ( nIndex < points.size() - 2 ) //CHEAT: don't finish path, use 1 point as a reserve.
			{
				p0 = p1; 
				p1 = p2; 
				p2 = p3; 
				p3 = points[nIndex];
				//vNormale = normales[nIndex];
				spline.Setup( p0, p1, p2, p3 );
				fSplineProgress = 0.0f;
				continue;
			}
			else	// no points, whole path is finished
			{
				nIndex = points.size() - 1;
				fSplineProgress = 1.0f;
				vNew = spline.Get( fSplineProgress );
				fPassed = fabs( vPos - vNew );
			}
		}
		else if ( fDistToGo == 0 )
		{
			vNew = vPos;
			fPassed = 0;
		}
		else
		{
			fSplineProgress += SConsts::PLANE_SPLINE_STEP;
			vNew = spline.Get( fSplineProgress );
			fPassed = fabs( vPos - vNew );	// distance passed for segment duration.
		}

		// check
		if ( fDistToGo <= fPassed )		// reach needed distance on this step.
		{
			// recalculate plane parameters
			if ( fPassed == 0.0f )
			{
				vPos = vNew;
			}
			else
			{
				//vPos = vNew;
				//CVec3 vTangent = spline.GetDiff1( fSplineProgress );
				//Normalize( &vTangent );
				//vSpeed = vTangent;
				//vNormale = normales[nIndex];

				vPos = vPosLastPoint + ( vNew - vPosLastPoint ) * fDistToGo / fPassed;
				CVec3 vTangent = spline.GetDiff1( fSplineProgress );
				Normalize( &vTangent );
				vSpeed = vSpeedLastPoint + ( vTangent - vSpeedLastPoint ) * fDistToGo / fPassed;
				Normalize( &vSpeed );
				vNormale = normales[nIndex] - vSpeed * ( normales[nIndex] * vSpeed );
				Normalize( &vNormale );

				vPosLastPoint = vNew;					
				vSpeedLastPoint = vTangent;
				vNormaleLastPoint = normales[nIndex];
			}
			*pfDistRemain = 0.0f;
			return false;
		}
		else if ( fSplineProgress == 1.0f ) // not passed needed distance, but reached end of spline and end of path
		{
			vPos = vNew;
			CVec3 vTangent = spline.GetDiff1( fSplineProgress );
			Normalize( &vTangent );
			vSpeed = vTangent;
			vNormale = normales[nIndex];
			*pfDistRemain = fDistToGo - fPassed;
			return true;
		}
	}
}

