#include "StdAfx.h"
#include "..\misc\win32random.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "VSOManager.h"
#include "../Misc/Spline.h"
#include "..\B2_M1_Terrain\DBVSO.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static consts
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float CVSOManager::DEFAULT_LEFT_BANK_HEIGHT = 1.0f;
const float CVSOManager::DEFAULT_RIGHT_BANK_HEIGHT = 1.0f;

const float CVSOManager::DEFAULT_WIDTH = 120.0f;
const float CVSOManager::DEFAULT_HEIGHT = 0.0f;
const float CVSOManager::DEFAULT_STEP = 32.0f;
const float CVSOManager::DEFAULT_OPACITY = 1.0f;
//
const float	CVSOManager::CONTROL_POINT_RADIUS	= AI_TILE_SIZE / 1.0f;
const int		CVSOManager::CONTROL_POINT_PARTS	= 8;
const float	CVSOManager::CENTER_POINT_RADIUS	= AI_TILE_SIZE / 1.5f;
const int		CVSOManager::CENTER_POINT_PARTS		= 8;
const float	CVSOManager::NORMALE_POINT_RADIUS	= AI_TILE_SIZE / 2.0f;
const int		CVSOManager::NORMALE_POINT_PARTS	= 8;
const DWORD	CVSOManager::CONTROL_POINT_COLOR	= 0xFFFF4040;
const DWORD	CVSOManager::CENTER_POINT_COLOR		= 0xFF4040F0;
const DWORD	CVSOManager::NORMALE_POINT_COLOR	= 0xFF40FF40;
const DWORD	CVSOManager::CONTROL_LINE_COLOR		= 0xFFFF4040;
const DWORD	CVSOManager::CENTER_LINE_COLOR		= 0xFF4040FF;
const DWORD	CVSOManager::NORMALE_LINE_COLOR		= 0xFF40FF40;
const float CVSOManager::DEFAULT_POINT_RADIUS	= AI_TILE_SIZE / 8.0f;
const DWORD CVSOManager::DEFAULT_POINT_PARTS	= 8;
const DWORD CVSOManager::DEFAULT_POINT_COLOR	= 0xFFFFFF80;
const DWORD CVSOManager::DEFAULT_LINE_COLOR		= 0xFFFFFF80;
const float	CVSOManager::OPACITY_DELIMITER		= 100.0f;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::SaveKeyPoints( const CVSOPointList &rVSOPointList )
{
	keyPointList.clear();
	for ( CVSOPointList::const_iterator itPoint = rVSOPointList.begin(); itPoint != rVSOPointList.end(); ++itPoint )
	{
		if ( itPoint->bKeyPoint )
		{
			CKeyPointList::iterator posKeyPoint = keyPointList.insert( keyPointList.end(), SKeyPoint() );
			if ( posKeyPoint != keyPointList.end() )
			{
				posKeyPoint->fWidth = itPoint->fWidth;
				posKeyPoint->fHeight = itPoint->vPos.z;
				posKeyPoint->fOpacity = itPoint->fOpacity;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::LoadKeyPoints( CVSOPointList *pVSOPointList )
{
	NI_ASSERT( pVSOPointList != 0, "CVSOManager::SBackupKeyPoints::LoadKeyPoints(): Wrong parameter: pVSOPointList == 0" );
	//
	CKeyPointList::const_iterator posKeyPoint = keyPointList.begin();
	for ( CVSOPointList::iterator itPoint = pVSOPointList->begin(); itPoint != pVSOPointList->end(); ++itPoint )
	{
		if ( itPoint->bKeyPoint && ( posKeyPoint != keyPointList.end() ) )
		{
			itPoint->fWidth = posKeyPoint->fWidth;
			itPoint->vPos.z = posKeyPoint->fHeight;
			itPoint->fOpacity = posKeyPoint->fOpacity;
			++posKeyPoint;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::AddKeyPoint( int nKeyPointIndex, float fWidth, float fHeight, float fOpacity )
{
	CKeyPointList::iterator posKeyPoint = keyPointList.end();
	if ( nKeyPointIndex < 0 )
	{
		posKeyPoint = keyPointList.insert( keyPointList.begin(), SKeyPoint() );
	}
	else if ( nKeyPointIndex >= keyPointList.size() )
	{
		posKeyPoint = keyPointList.insert( keyPointList.end(), SKeyPoint() );
	}
	else
	{
		CKeyPointList::iterator itKeyPoint = keyPointList.begin();
		for ( int nIndex = 0; nIndex < nKeyPointIndex; ++nIndex )
		{
			++itKeyPoint;
		}
		posKeyPoint = keyPointList.insert( itKeyPoint, SKeyPoint() );
	}
	if ( posKeyPoint != keyPointList.end() )
	{
		posKeyPoint->fWidth = fWidth;
		posKeyPoint->fHeight = fHeight;
		posKeyPoint->fOpacity = fOpacity;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::RemoveKeyPoint( int nKeyPointIndex )
{
	if ( ( nKeyPointIndex >= 0 ) && ( nKeyPointIndex < keyPointList.size() ) )
	{
		CKeyPointList::iterator itKeyPoint = keyPointList.begin();
		for ( int nIndex = 0; nIndex < nKeyPointIndex; ++nIndex )
		{
			++itKeyPoint;
		}
		keyPointList.erase( itKeyPoint );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::InsertToFront( float fWidth, float fHeight, float fOpacity  )
{
	CKeyPointList::iterator posKeyPoint = keyPointList.insert( keyPointList.begin(), SKeyPoint() );
	if ( posKeyPoint != keyPointList.end() )
	{
		posKeyPoint->fWidth = fWidth;
		posKeyPoint->fHeight = fHeight;
		posKeyPoint->fOpacity = fOpacity;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::InsertToBack( float fWidth, float fHeight, float fOpacity )
{
	CKeyPointList::iterator posKeyPoint = keyPointList.insert( keyPointList.end(), SKeyPoint() );
	if ( posKeyPoint != keyPointList.end() )
	{
		posKeyPoint->fWidth = fWidth;
		posKeyPoint->fHeight = fHeight;
		posKeyPoint->fOpacity = fOpacity;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::SetFrontOpacity( float fOpacity )
{
	keyPointList.front().fOpacity = fOpacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::SetBackOpacity( float fOpacity )
{
	keyPointList.back().fOpacity = fOpacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SBackupKeyPoints::Clear()
{
	keyPointList.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::SVSOCircle::CreateFromDirection( const CVec2 &vBegin, const CVec2 &vEnd, float _fRadius, EClassifyRotation _classifyRotation, bool bBegin )
{
	r = _fRadius;
	classifyRotation = _classifyRotation;

	if ( bBegin )
	{
		vCreationPoint = vBegin;
	}
	else
	{
		vCreationPoint = vEnd;
	}

	CVec2 vNormal = GetNormal( vEnd - vBegin );
	Normalize( &vNormal );
	if ( classifyRotation == CR_COUNTERCLOCKWISE )
	{
		center = vCreationPoint + vNormal * r;
	}
	else if ( classifyRotation == CR_CLOCKWISE )
	{
		center = vCreationPoint - vNormal * r;
	}
	else
	{
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::SVSOCircle::GetTangentPoint( const CVec2 &v, CVec2 *pTangentPoint ) const
{
	const CVec2 vCenterV = center - v;
	const float fDistance2 = fabs2( vCenterV );
	const float fRadius2 = sqr( r );
	if ( fDistance2 < fRadius2 )
	{
		return false;
	}
	else if ( fDistance2 == fRadius2 )
	{
		*pTangentPoint = v;
	}
	else
	{
		const float fLeg2 = fDistance2 - fRadius2;
		const float fCossin = float( sqrt( fLeg2 ) ) * r / fDistance2;
		const float fCos2 = fLeg2 / fDistance2;

		if ( classifyRotation == CR_COUNTERCLOCKWISE )
		{
			pTangentPoint->x = v.x + ( vCenterV.x * fCos2 ) - ( vCenterV.y * fCossin );
			pTangentPoint->y = v.y + ( vCenterV.x * fCossin ) + ( vCenterV.y * fCos2 );
		}
		else if ( classifyRotation == CR_CLOCKWISE )
		{
			pTangentPoint->x = v.x + ( vCenterV.x * fCos2 ) + ( vCenterV.y * fCossin );
			pTangentPoint->y = v.y - ( vCenterV.x * fCossin ) + ( vCenterV.y * fCos2 );
		}
		else
		{
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::SVSOCircle::GetPointsSequence( const CVec2 &v, int nSegmentsCount, list<CVec2> *pPointsSequence ) const
{
	NI_ASSERT( pPointsSequence != 0,
						 "CVSOManager::SVSOCircle::GetPointsSequence(): Wrong parameter: pPointsSequence == 0" );

	float fStartPolarAngle = GetPolarAngle( vCreationPoint - center );
	const float fEndPolarAngle = GetPolarAngle( v - center );
	const float fAngleStep = FP_2PI / nSegmentsCount;
	if ( classifyRotation == CR_CLOCKWISE )
	{
		if ( fStartPolarAngle < fEndPolarAngle )
		{
			fStartPolarAngle += FP_2PI;	
		}
		for ( float fAngle = fStartPolarAngle; fAngle > fEndPolarAngle; fAngle -= fAngleStep )
		{
			pPointsSequence->push_back( center + CreateFromPolarCoord( r, fAngle ) );
		}
	}
	else
	{
		if ( fStartPolarAngle > fEndPolarAngle )
		{
			fStartPolarAngle -= FP_2PI;
		}
		for ( float fAngle = fStartPolarAngle; fAngle < fEndPolarAngle; fAngle += fAngleStep )
		{
			pPointsSequence->push_back( center + CreateFromPolarCoord( r, fAngle ) );
		}
	}
	const float fStepSize2 = fabs2( CreateFromPolarCoord( r, fStartPolarAngle ) - CreateFromPolarCoord( r, fStartPolarAngle + fAngleStep ) );
	if ( fabs2( pPointsSequence->back() - v ) > ( fStepSize2 / 4.0f ) )
	{
		pPointsSequence->push_back( v );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static methods
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CVSOManager::SliceSpline( const CAnalyticBSpline2 &rSpline,
															list<NDb::SVSOPoint> *pPoints,
															float *pfRest,
															const float fStep )
{
	const float fSplineLength = rSpline.GetLengthAdaptive( 1 );
	const float dt = fStep / ( fSplineLength * 10.0f );
	int nCounter = 0;
	float fLen = *pfRest + fStep;
	float fLastT = 0, fT = 0;
	CVec2 vLastPos, vPos = rSpline.Get( fT );
	while ( 1 ) 
	{
		fLen -= fStep;
		while ( fLen < fStep ) 
		{
			vLastPos = vPos;
			// make step
			fT += dt;
			vPos = rSpline.Get( fT );
			fLen += fabs( vLastPos - vPos );
			if ( fT > 1 ) 
				break;
		}
		if ( (fT > 1) || (fLen < fStep) ) 
			break;
		//
		NDb::SVSOPoint point;
		point.vPos = CVec3( rSpline( fT ), 0.0f );
		point.vNorm = CVec3( rSpline.GetDiff1( fT ), 0.0f );
		point.vNorm.Set( point.vNorm.y, -point.vNorm.x, 0.0f );
		point.bKeyPoint = false;
		Normalize( &point.vNorm );
		point.fRadius = rSpline.GetCurvatureRadius( fT );
		pPoints->push_back( point );
		fLastT = fT;
		//
		++nCounter;
	}
	pPoints->front().bKeyPoint = true;
	pPoints->back().bKeyPoint = true;
	//
	*pfRest = rSpline.GetLength( fLastT, 1.0f, 100 );
	//
	return nCounter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SampleCurve( const vector<CVec3> &rControlPointList,
															 CVSOPointList *pVSOPointList,
															 float fStep, 
															 float fWidth,
															 float fHeight,
															 float fOpacity,
															 bool bCircle,
															 bool bComplete )
{
	NI_ASSERT( rControlPointList.size() > 1,
						 StrFmt( "CVSOManager::SampleCurve(): Invalid size: rControlPointList: %d\n",
										 rControlPointList.size() ) );
	NI_ASSERT( pVSOPointList != 0, "CVSOManager::SampleCurve(): Wrong parameter: pVSOPointList == 0" );

	//collect spline points
	vector<CVec3> splinePointList;
	if ( bCircle && ( rControlPointList.size() > 2 ) )
	{
		splinePointList.push_back( rControlPointList[rControlPointList.size() - 1] );
		for( vector<CVec3>::const_iterator itControlPoint = rControlPointList.begin(); itControlPoint != rControlPointList.end(); ++itControlPoint )
		{
			splinePointList.push_back( *itControlPoint );
		}
		splinePointList.push_back( rControlPointList[0] );
	}
	else
	{
		splinePointList.push_back( rControlPointList[0] - 
															( rControlPointList[1] - rControlPointList[0] ) );
		for( vector<CVec3>::const_iterator itControlPoint = rControlPointList.begin(); itControlPoint != rControlPointList.end(); ++itControlPoint )
		{
			splinePointList.push_back( *itControlPoint );
		}
		splinePointList.push_back( rControlPointList[rControlPointList.size() - 1] + 
															( rControlPointList[rControlPointList.size() - 1] - rControlPointList[rControlPointList.size() - 2] ) );
	}
	//form spline points
	const float fSplineStep = fStep;
	float fRestLength = fSplineStep - 1e-8f;
	list<NDb::SVSOPoint> vsoPointList;
	CAnalyticBSpline2 spline;
	int nCounter = 0;
	//
	for ( int nSplinePointIndex = 0; nSplinePointIndex < ( splinePointList.size() - 3 ); ++nSplinePointIndex )
	{
		spline.Setup( splinePointList[nSplinePointIndex], splinePointList[nSplinePointIndex + 1], splinePointList[nSplinePointIndex + 2], splinePointList[nSplinePointIndex + 3] );
		nCounter += SliceSpline( spline, &vsoPointList, &fRestLength, fSplineStep );
	}
	//
	if ( bCircle && ( rControlPointList.size() > 2 ) )
	{
		spline.Setup( rControlPointList[rControlPointList.size() - 2], rControlPointList[rControlPointList.size() - 1], rControlPointList[0], rControlPointList[1] );
		nCounter += SliceSpline( spline, &vsoPointList, &fRestLength, fSplineStep );
		//
		vsoPointList.back().bKeyPoint = false;
		list<NDb::SVSOPoint>::iterator posVSOPoint = vsoPointList.insert( vsoPointList.end(), vsoPointList.front() );
		posVSOPoint->bKeyPoint = false;
	}
	else if ( bComplete )
	{
		vsoPointList.back().bKeyPoint = false;
		list<NDb::SVSOPoint>::iterator posVSOPoint = vsoPointList.insert( vsoPointList.end(), vsoPointList.back() );
		posVSOPoint->vPos = rControlPointList.back();
		posVSOPoint->bKeyPoint = true;
	}
	//
	for ( list<NDb::SVSOPoint>::const_iterator itVSOPoint = vsoPointList.begin(); itVSOPoint != vsoPointList.end(); ++itVSOPoint )
	{
		CVSOPointList::iterator posVSOPoint = pVSOPointList->insert( pVSOPointList->end(), *itVSOPoint );
		if ( posVSOPoint != pVSOPointList->end() )
		{
			posVSOPoint->fWidth = fWidth;
			posVSOPoint->vPos.z = fHeight;
			posVSOPoint->fOpacity = fOpacity;
		}
	}
	if ( !pVSOPointList->empty() )
	{
		pVSOPointList->begin()->bKeyPoint = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SmoothCurve( const vector<int> &rKeyPointList, vector<float> *pPointList, bool bCircle, bool bComplete )
{
	NI_ASSERT( pPointList != 0, "CVSOManager::SmoothCurveWidth(): pPointList == 0" );
	
	// form indices for spline
	vector<float> originalPointList = ( *pPointList );
	vector<int> indexList;
	indexList.reserve( rKeyPointList.size() + 4 );
	if ( bCircle )
	{
    if ( rKeyPointList.size() < 3 )   
    {
      return;
    }
    indexList.push_back( rKeyPointList.size() - 2 );
		indexList.push_back( rKeyPointList.size() - 1 );
		for ( int nKeyPointIndex = 0; nKeyPointIndex < rKeyPointList.size(); ++nKeyPointIndex )
		{
			indexList.push_back( nKeyPointIndex );
		}
		indexList.push_back( 0 );
		indexList.push_back( 1 );
	}
	else
	{
    if ( rKeyPointList.size() < 2 )   
    {
      return;
    }
    indexList.push_back( 0 );
		indexList.push_back( 0 );
		for ( int nKeyPointIndex = 0; nKeyPointIndex < rKeyPointList.size(); ++nKeyPointIndex )
		{
			indexList.push_back( nKeyPointIndex );
		}
		indexList.push_back( rKeyPointList.size() - 1 );
		indexList.push_back( rKeyPointList.size() - 1 );
	}

	CAnalyticBSpline spline;
	for ( int nIndex = 0; nIndex < ( indexList.size() - 3 ); ++nIndex )
	{
		const int idx0 = rKeyPointList[ indexList[nIndex + 0] ];
		const int idx1 = rKeyPointList[ indexList[nIndex + 1] ];
		const int idx2 = rKeyPointList[ indexList[nIndex + 2] ];
		const int idx3 = rKeyPointList[ indexList[nIndex + 3] ];
		//
		if ( idx2 <= idx1 )
		{
			continue;
		}
		//
		spline.Setup( originalPointList[idx0],
									originalPointList[idx1],
									originalPointList[idx2],
									originalPointList[idx3] );
		//
		for ( int nInnerIndex = idx1; nInnerIndex < idx2; ++nInnerIndex )
		{
			const float fRatio = float( nInnerIndex - idx1 ) / float( idx2 - idx1 );
			( *pPointList )[nInnerIndex] = spline( fRatio );
		}
		if ( idx2 == ( pPointList->size() - 1 ) )
		{
			( *pPointList )[idx2] = spline( 1 );
		}
	}
	if ( bCircle && ( rKeyPointList.size() > 2 ) )
	{
		const int idx0 = rKeyPointList[rKeyPointList.size() - 2];
		const int idx1 = rKeyPointList[rKeyPointList.size() - 1];
		const int idx2 = rKeyPointList[0];
		const int idx3 = rKeyPointList[1];
		if ( ( ( originalPointList.size() - 1 ) - idx1 ) > 0 )
		{
			spline.Setup( originalPointList[idx0],
										originalPointList[idx1],
										originalPointList[idx2],
										originalPointList[idx3] );
			//
			for ( int nInnerIndex = idx1; nInnerIndex < originalPointList.size(); ++nInnerIndex )
			{
				const float fRatio = float( nInnerIndex - idx1 ) / float( ( originalPointList.size() - 1 ) - idx1 );
				( *pPointList )[nInnerIndex] = spline( fRatio );
			}
			pPointList->back() = pPointList->front();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SmoothCurveWidth( CVSOPointList *pVSOPointList, bool bCircle, bool bComplete )
{
	NI_ASSERT( pVSOPointList != 0, "CVSOManager::SmoothCurveWidth(): pVSOPointList == 0" );

	vector<int> keyPointList;
	vector<float> pointList;
	pointList.reserve( pVSOPointList->size() );
	for ( int nVSOPointIndex = 0; nVSOPointIndex < pVSOPointList->size(); ++nVSOPointIndex )
	{
		if ( ( *pVSOPointList )[nVSOPointIndex].bKeyPoint )
		{
			keyPointList.push_back( nVSOPointIndex );
		}
		pointList.push_back( ( *pVSOPointList )[nVSOPointIndex].fWidth );
	}
	//
	SmoothCurve( keyPointList, &pointList, bCircle, bComplete );
	//
	// write widths to the result array
	for ( int nVSOPointIndex = 0; nVSOPointIndex < pVSOPointList->size(); ++nVSOPointIndex )
	{
		( *pVSOPointList )[nVSOPointIndex].fWidth = pointList[nVSOPointIndex];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SmoothCurveHeight( CVSOPointList *pVSOPointList, bool bCircle, bool bComplete )
{
	NI_ASSERT( pVSOPointList != 0, "CVSOManager::SmoothCurveWidth(): pVSOPointList == 0" );

	vector<int> keyPointList;
	vector<float> pointList;
	pointList.reserve( pVSOPointList->size() );
	for ( int nVSOPointIndex = 0; nVSOPointIndex < pVSOPointList->size(); ++nVSOPointIndex )
	{
		if ( ( *pVSOPointList )[nVSOPointIndex].bKeyPoint )
		{
			keyPointList.push_back( nVSOPointIndex );
		}
		pointList.push_back( ( *pVSOPointList )[nVSOPointIndex].vPos.z );
	}
	//
	if ( keyPointList.size() > 0 )
	{
		pointList[keyPointList[0]] = 0.0f;
		pointList[keyPointList[keyPointList.size() - 1]] = 0.0f;
	}
	//
	SmoothCurve( keyPointList, &pointList, bCircle, bComplete );
	//
	// write height to the result array
	for ( int nVSOPointIndex = 0; nVSOPointIndex < pVSOPointList->size(); ++nVSOPointIndex )
	{
		if ( pointList[nVSOPointIndex] > 0.0f )
		{
			( *pVSOPointList )[nVSOPointIndex].vPos.z = pointList[nVSOPointIndex];
		}
		else
		{
			( *pVSOPointList )[nVSOPointIndex].vPos.z = 0.0f;
		}
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::SmoothCurveOpacity( CVSOPointList *pVSOPointList, bool bCircle, bool bComplete )
{
	NI_ASSERT( pVSOPointList != 0, "CVSOManager::SmoothCurveOpacity(): pVSOPointList == 0" );
	//
	vector<int> keyPointList;
	int nStartPointIndex = 0;
	while ( nStartPointIndex < pVSOPointList->size() )
	{
		if ( ( *pVSOPointList )[nStartPointIndex].bKeyPoint )
		{
			keyPointList.push_back( nStartPointIndex );
			break;
		}
		++nStartPointIndex;
	}
	//
	int nFinishPointIndex = nStartPointIndex + 1;
	while ( nFinishPointIndex < pVSOPointList->size() )
	{
		while ( ( nFinishPointIndex < pVSOPointList->size() ) && ( !( *pVSOPointList )[nFinishPointIndex].bKeyPoint ) )
		{
			++nFinishPointIndex;
		}
		if ( nFinishPointIndex < pVSOPointList->size() )
		{
			keyPointList.push_back( nFinishPointIndex );

			const float fStep = ( ( *pVSOPointList )[nFinishPointIndex].fOpacity - ( *pVSOPointList )[nStartPointIndex].fOpacity ) / ( nFinishPointIndex - nStartPointIndex );
			for ( int nPointIndex = ( nStartPointIndex + 1 ); nPointIndex < nFinishPointIndex; ++nPointIndex )
			{
				( *pVSOPointList )[nPointIndex].fOpacity = ( *pVSOPointList )[nStartPointIndex].fOpacity + fStep * ( nPointIndex - nStartPointIndex );
			}
			nStartPointIndex = nFinishPointIndex;
			keyPointList.push_back( nStartPointIndex );
			nFinishPointIndex = nStartPointIndex + 1;
		}
	}
	//
	if ( bCircle && ( keyPointList.size() > 2 ) )
	{
		const int nFirstControlPointIndex = keyPointList[0];
		const int nLastControlPointIndex = keyPointList[keyPointList.size() - 1];
		if ( ( pVSOPointList->size() - nLastControlPointIndex - 1 ) > 0 )
		{
			const float fStep = ( ( *pVSOPointList )[nFirstControlPointIndex].fOpacity - ( *pVSOPointList )[nLastControlPointIndex].fOpacity ) / ( pVSOPointList->size() - nLastControlPointIndex - 1 );
			for ( int nPointIndex = ( nLastControlPointIndex + 1 ); nPointIndex < pVSOPointList->size(); ++nPointIndex )
			{
				( *pVSOPointList )[nPointIndex].fOpacity = ( *pVSOPointList )[nLastControlPointIndex].fOpacity + fStep * ( nPointIndex - nLastControlPointIndex );
			}
			nStartPointIndex = nFinishPointIndex;
			++nFinishPointIndex;
			( *pVSOPointList )[pVSOPointList->size() - 1].fOpacity = ( *pVSOPointList )[0].fOpacity;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::Update( NDb::SVSOInstance *pVSO,
													bool bResampleCurve,
													bool bKeepKeyPoints,
													float fStep,
													float fWidth,
													float fHeight,
													float fOpacity,
													bool bWidth,
													bool bHeight,
													bool bOpacity,
													bool bCircle,
													bool bComplete )
{
	NI_ASSERT( pVSO != 0, "CVSOManager::Update(): Wrong parameter: pVSO == 0" );
	//сохряняем ширины
	SBackupKeyPoints backupKeyPoints;
	if ( bResampleCurve )
	{
		if ( bKeepKeyPoints )
		{
			backupKeyPoints.SaveKeyPoints( pVSO->points );
		}
		pVSO->points.clear();
		SampleCurve( pVSO->controlPoints, &( pVSO->points ), fStep, fWidth, fHeight, fOpacity, bCircle, bComplete );
		if ( bKeepKeyPoints )
		{
			backupKeyPoints.LoadKeyPoints( &( pVSO->points ) );
		}
	}
	if ( bWidth )
	{
		SmoothCurveWidth( &( pVSO->points ), bCircle, bComplete );
	}
	if ( bHeight )
	{
		SmoothCurveHeight( &( pVSO->points ), bCircle, bComplete );
	}
	if ( bOpacity )
	{
		SmoothCurveOpacity( &( pVSO->points ), bCircle, bComplete );
	}
	// set default bank heights for left and right
	for ( CVSOPointList::iterator it = pVSO->points.begin(); it != pVSO->points.end(); ++it )
	{
		it->fRadius = DEFAULT_LEFT_BANK_HEIGHT;
		it->fReserved = DEFAULT_RIGHT_BANK_HEIGHT;
	}
	//
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::MoveEdgeControlPointsOut( NDb::SVSOInstance *pVSO, const CTRect<float> &rRect, float fStepOut, bool bBothEdges, bool bFixedStepOut )
{
	NI_ASSERT( pVSO != 0, "CVSOManager::Update(): Wrong parameter: pVSO == 0" );
	int nFirstIndex = 0;
	int nLastIndex = pVSO->controlPoints.size() - 1;
	if ( nLastIndex > nFirstIndex )
	{
		vector<int> indices;
		indices.push_back( nFirstIndex );
		if ( bBothEdges )
		{
			indices.push_back( nLastIndex );
		}
		for ( vector<int>::const_iterator itIndex = indices.begin(); itIndex < indices.end(); ++itIndex )
		{
			const float fX0 = pVSO->controlPoints[*itIndex].x - rRect.minx;
			const float fXN = rRect.maxx - pVSO->controlPoints[*itIndex].x;
			const float fY0 = pVSO->controlPoints[*itIndex].y - rRect.miny;
			const float fYN = rRect.maxy - pVSO->controlPoints[*itIndex].y;
			float fMin = fX0;
			if ( fXN < fMin )
			{
				fMin = fXN;
			}
			if ( fY0 < fMin )
			{
				fMin = fY0;
			}
			if ( fYN < fMin )
			{
				fMin = fYN;
			}
			if ( fMin > ( -fStepOut ) || bFixedStepOut )
			{
				if ( fX0 == fMin )
				{
					pVSO->controlPoints[*itIndex].x = rRect.minx - fStepOut;
				}
				else if ( fXN == fMin )
				{
					pVSO->controlPoints[*itIndex].x = rRect.maxx + fStepOut;
				}
				else if ( fY0 == fMin )
				{
					pVSO->controlPoints[*itIndex].y = rRect.miny - fStepOut;
				}
				else if ( fYN == fMin )
				{
					pVSO->controlPoints[*itIndex].y = rRect.maxy + fStepOut;
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::GetBoundingPolygon( list<CVec3> *pBoundingPolygon, const CVSOPointList &rVSOPointList, int nPointIndex, EVSOPolygonType vsoPolygonType, float fRelWidth )
{
	NI_ASSERT( pBoundingPolygon != 0, "CVSOManager::GetBoundingPolygon(): Wrong parameter: pBoundingPolygon == 0" );
	NI_ASSERT( ( nPointIndex >= 0 ) && ( nPointIndex < rVSOPointList.size() ),
						 StrFmt( "CVSOManager::GetBoundingPolygon(): Invalid argument: nPointIndex: %d (%d)\n",
										 nPointIndex,
										 rVSOPointList.size() ) );
	pBoundingPolygon->clear();
	//
	switch( vsoPolygonType )
	{
		case PT_NORMALE:
		{
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex].vPos.x + ( rVSOPointList[nPointIndex].vNorm.x * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex].vPos.y + ( rVSOPointList[nPointIndex].vNorm.y * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex + 1].vPos.x + ( rVSOPointList[nPointIndex + 1].vNorm.x * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex + 1].vPos.y + ( rVSOPointList[nPointIndex + 1].vNorm.y * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex + 1].vPos.x,
																					rVSOPointList[nPointIndex + 1].vPos.y,
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex].vPos.x,
																					rVSOPointList[nPointIndex].vPos.y,
																					0.0f ) );
			break;
		}
		case PT_OPNORMALE:
		{
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex].vPos.x,
																					rVSOPointList[nPointIndex].vPos.y,
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex + 1].vPos.x,
																					rVSOPointList[nPointIndex + 1].vPos.y,
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex + 1].vPos.x - ( rVSOPointList[nPointIndex + 1].vNorm.x * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex + 1].vPos.y - ( rVSOPointList[nPointIndex + 1].vNorm.y * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex].vPos.x - ( rVSOPointList[nPointIndex].vNorm.x * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex].vPos.y - ( rVSOPointList[nPointIndex].vNorm.y * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					0.0f ) );
			break;
		}
		case PT_BOTH:
		default:
		{
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex].vPos.x + ( rVSOPointList[nPointIndex].vNorm.x * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex].vPos.y + ( rVSOPointList[nPointIndex].vNorm.y * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex + 1].vPos.x + ( rVSOPointList[nPointIndex + 1].vNorm.x * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex + 1].vPos.y + ( rVSOPointList[nPointIndex + 1].vNorm.y * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex + 1].vPos.x - ( rVSOPointList[nPointIndex + 1].vNorm.x * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex + 1].vPos.y - ( rVSOPointList[nPointIndex + 1].vNorm.y * rVSOPointList[nPointIndex + 1].fWidth * fRelWidth ),
																					0.0f ) );
			pBoundingPolygon->push_back( CVec3( rVSOPointList[nPointIndex].vPos.x - ( rVSOPointList[nPointIndex].vNorm.x * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					rVSOPointList[nPointIndex].vPos.y - ( rVSOPointList[nPointIndex].vNorm.y * rVSOPointList[nPointIndex].fWidth * fRelWidth ),
																					0.0f ) );
			break;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::GetBoundingPolygon( list<CVec3> *pBoundingPolygon, const CVSOPointList &rVSOPointList, EVSOPolygonType vsoPolygonType, float fRelWidth )
{
	NI_ASSERT( pBoundingPolygon != 0, "CVSOManager::GetBoundingPolygon(): Wrong parameter: pBoundingPolygon == 0" );
	pBoundingPolygon->clear();
	//
	for ( CVSOPointList::const_iterator itPoint = rVSOPointList.begin(); itPoint != rVSOPointList.end(); ++itPoint )
	{
		if ( itPoint->bKeyPoint )
		{
			switch( vsoPolygonType )
			{
				case PT_NORMALE:
					pBoundingPolygon->push_back( itPoint->vPos + itPoint->vNorm * itPoint->fWidth * fRelWidth );
					break;
				case PT_OPNORMALE:
					pBoundingPolygon->push_back( itPoint->vPos );
					break;
				case PT_BOTH:
				default:
					pBoundingPolygon->push_back( itPoint->vPos + itPoint->vNorm * itPoint->fWidth * fRelWidth );
					break;
			}
		}
	}
	//
	for ( CVSOPointList::const_iterator itPoint = rVSOPointList.begin(); itPoint != rVSOPointList.end(); ++itPoint )
	{
		if ( itPoint->bKeyPoint )
		{
			switch( vsoPolygonType )
			{
				case PT_NORMALE:
					pBoundingPolygon->push_front( itPoint->vPos );
					break;
				case PT_OPNORMALE:
					pBoundingPolygon->push_front( itPoint->vPos - itPoint->vNorm * itPoint->fWidth * fRelWidth );
					break;
				case PT_BOTH:
				default:
					pBoundingPolygon->push_front( itPoint->vPos - itPoint->vNorm * itPoint->fWidth * fRelWidth );
					break;
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::GetBoundingPolygon( vector<CVec2> *pBoundingPolygon, const CVSOPointList &rVSOPointList, EVSOPolygonType vsoPolygonType, float fRelWidth )
{
	if ( pBoundingPolygon == 0 )
	{
		return;
	}
	pBoundingPolygon->clear();
	const int nPointCount = rVSOPointList.size();
	if ( nPointCount > 0 )
	{
		pBoundingPolygon->resize( nPointCount * 2 );
		//
		CVec2 pos;
		CVec2 norm;
		for ( int nPointIndex = 0; nPointIndex < nPointCount; ++nPointIndex )
		{
			const NDb::SVSOPoint &rVSOPoint = rVSOPointList[nPointIndex];
			pos = GetPointType( rVSOPoint.vPos, static_cast<CVec2*>( 0 ) );
			norm = GetPointType( rVSOPoint.vNorm, static_cast<CVec2*>( 0 ) );
			switch( vsoPolygonType )
			{
				case PT_NORMALE:
					( *pBoundingPolygon )[nPointIndex] = pos + norm * rVSOPoint.fWidth * fRelWidth;
					( *pBoundingPolygon )[ ( nPointCount * 2 ) - nPointIndex - 1] = pos;
					break;
				case PT_OPNORMALE:
					( *pBoundingPolygon )[nPointIndex] = pos;
					( *pBoundingPolygon )[ ( nPointCount * 2 ) - nPointIndex - 1] = pos - norm * rVSOPoint.fWidth * fRelWidth;
					break;
				case PT_BOTH:
				default:
					( *pBoundingPolygon )[nPointIndex] = pos + norm * rVSOPoint.fWidth * fRelWidth;
					( *pBoundingPolygon )[ ( nPointCount * 2 ) - nPointIndex - 1] = pos - norm * rVSOPoint.fWidth * fRelWidth;
					break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::GetCragBoundingPolygon( vector<CVec2> *pBoundingPolygon, const CVSOPointList &rVSOPointList, EVSOPolygonType vsoPolygonType, float fRelWidth, int nVSOID )
{
	if ( pBoundingPolygon == 0 )
	{
		return;
	}
	pBoundingPolygon->clear();
	if ( !EditorScene() || !EditorScene()->GetTerraManager() )
	{
		GetBoundingPolygon( pBoundingPolygon, rVSOPointList, vsoPolygonType, fRelWidth );
		return;
	}
	vector<CVec3> holePointList;
	if ( !EditorScene()->GetTerraManager()->GetCragPrecVerts( &holePointList, nVSOID ) )
	{
		GetBoundingPolygon( pBoundingPolygon, rVSOPointList, vsoPolygonType, fRelWidth );
		return;
	}
	const int nHolePointCount = holePointList.size();
	const int nPointCount = rVSOPointList.size();
	const int nOverallPointCount = nHolePointCount + nPointCount;
	if ( ( nOverallPointCount ) > 0 )
	{
		pBoundingPolygon->resize( nOverallPointCount );
		//
		CVec2 pos;
		CVec2 norm;
		for ( int nPointIndex = 0; nPointIndex < nHolePointCount; ++nPointIndex )
		{
			( *pBoundingPolygon )[nPointIndex] = CVec2( holePointList[nPointIndex].x * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE / VIS_TILE_SIZE, holePointList[nPointIndex].y * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE / VIS_TILE_SIZE );
		}
		for ( int nPointIndex = 0; nPointIndex < nPointCount; ++nPointIndex )
		{
			const NDb::SVSOPoint &rVSOPoint = rVSOPointList[nPointIndex];
			pos = GetPointType( rVSOPoint.vPos, static_cast<CVec2*>( 0 ) );
			norm = GetPointType( rVSOPoint.vNorm, static_cast<CVec2*>( 0 ) );
			switch( vsoPolygonType )
			{
				default:
				case PT_NORMALE:
					( *pBoundingPolygon )[nOverallPointCount - nPointIndex - 1] = pos + norm * rVSOPoint.fWidth * fRelWidth;
					break;
				case PT_OPNORMALE:
					( *pBoundingPolygon )[nOverallPointCount - nPointIndex - 1] = pos - norm * rVSOPoint.fWidth * fRelWidth;
					break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::GetPointsSequence( const SVSOCircle &rCircleBegin, const SVSOCircle &rCircleEnd, int nSegmentsCountBegin, int nSegmentsCountEnd, list<CVec2> *pPointsSequence )
{
	NI_ASSERT( pPointsSequence != 0,
						 "CVSOManager::GetPointsSequence(): Wrong parameter: pPointsSequence == 0" );

	CVec2 vBeginTangentPoint( VNULL2 );
	CVec2 vEndTangentPoint( VNULL2 );

	if ( ( rCircleBegin.classifyRotation != rCircleEnd.classifyRotation ) &&
			 ( fabs( rCircleBegin.r - rCircleEnd.r ) < FP_EPSILON ) )
	{
		CVec2 vAdditionalRadius = GetNormal( rCircleEnd.center - rCircleBegin.center );
		Normalize( &vAdditionalRadius );
		if ( rCircleBegin.classifyRotation == CR_COUNTERCLOCKWISE )
		{
			vAdditionalRadius *= -rCircleBegin.r;
		}
		else
		{
			vAdditionalRadius *= rCircleBegin.r;
		}

		vBeginTangentPoint = rCircleBegin.center + vAdditionalRadius;
		vEndTangentPoint = rCircleEnd.center + vAdditionalRadius;
	}
	else
	{
		SVSOCircle newCircleEnd = rCircleEnd;
		if ( rCircleBegin.classifyRotation == newCircleEnd.classifyRotation )
		{
			newCircleEnd.r += rCircleBegin.r;
		}
		else
		{
			newCircleEnd.r -= rCircleBegin.r;
			if ( newCircleEnd.r < 0 )
			{
				newCircleEnd.classifyRotation = GetNegativeClassifyRotation( newCircleEnd.classifyRotation );
				newCircleEnd.r *= ( -1 );
			}
		}
		
		CVec2 vTangentPoint;
		if ( !newCircleEnd.GetTangentPoint( rCircleBegin.center, &vTangentPoint ) )
		{
			return false;
		}
		
		CVec2 vAdditionalRadius = vTangentPoint - newCircleEnd.center;
		if ( newCircleEnd.classifyRotation != rCircleEnd.classifyRotation )
		{
			vAdditionalRadius *= ( -1 );
		}
		vAdditionalRadius *= rCircleEnd.r / newCircleEnd.r;

		vBeginTangentPoint = rCircleBegin.center + vAdditionalRadius - ( vTangentPoint - newCircleEnd.center );
		vEndTangentPoint = rCircleEnd.center + vAdditionalRadius;
	}

	rCircleBegin.GetPointsSequence( vBeginTangentPoint, nSegmentsCountBegin, pPointsSequence );
	list<CVec2> endPointsSequence;
	rCircleEnd.GetPointsSequence( vEndTangentPoint, nSegmentsCountEnd, &endPointsSequence );
	// в обратном порядке
	for ( list<CVec2>::const_iterator itPoint = endPointsSequence.begin(); itPoint!= endPointsSequence.end(); ++itPoint )
	{
		pPointsSequence->push_front( *itPoint );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::GetPointsSequence( const CVec2 &vBegin0, const CVec2 &vEnd0, float fRadius0, int nSegmentsCount0, bool bBegin0,
																		 const CVec2 &vBegin1, const CVec2 &vEnd1, float fRadius1, int nSegmentsCount1, bool bBegin1,
																		 list<CVec2> *pPointsSequence )
{
	NI_ASSERT( pPointsSequence != 0,
						 "CVSOManager::GetPointsSequence(): Wrong parameter: pPointsSequence == 0" );

	SVSOCircle circle00;
	SVSOCircle circle01;
	SVSOCircle circle10;
	SVSOCircle circle11;

	circle00.CreateFromDirection( vBegin0, vEnd0, fRadius0, CR_CLOCKWISE, bBegin0 );
	circle01.CreateFromDirection( vBegin0, vEnd0, fRadius0, CR_COUNTERCLOCKWISE, bBegin0 );
	circle10.CreateFromDirection( vBegin1, vEnd1, fRadius1, CR_CLOCKWISE, bBegin1 );
	circle11.CreateFromDirection( vBegin1, vEnd1, fRadius1, CR_COUNTERCLOCKWISE, bBegin1 );

	list<CVec2> road0010points;
	list<CVec2> road0011points;
	list<CVec2> road0110points;
	list<CVec2> road0111points;

	float fPerimeter0010 = -1.0f;
	float fPerimeter0011 = -1.0f;
	float fPerimeter0110 = -1.0f;
	float fPerimeter0111 = -1.0f;

	if ( GetPointsSequence( circle00, circle10, nSegmentsCount0, nSegmentsCount1, &road0010points ) )
	{
		fPerimeter0010 = GetPolygonPerimeter( road0010points );
	}
	if ( GetPointsSequence( circle00, circle11, nSegmentsCount0, nSegmentsCount1, &road0011points ) )
	{
		fPerimeter0011 = GetPolygonPerimeter( road0011points );
	}
	if ( GetPointsSequence( circle01, circle10, nSegmentsCount0, nSegmentsCount1, &road0110points ) )
	{
		fPerimeter0110 = GetPolygonPerimeter( road0110points );
	}
	if ( GetPointsSequence( circle01, circle11, nSegmentsCount0, nSegmentsCount1, &road0111points ) )
	{
		fPerimeter0111 = GetPolygonPerimeter( road0111points );
	}
	
	int nIndex = -1;
	float nMinPerimeter = fPerimeter0010 + fPerimeter0011 + fPerimeter0110 + fPerimeter0111 + 1.0f;
	
	if ( ( fPerimeter0010 >= 0 ) && ( fPerimeter0010 < nMinPerimeter ) )
	{
		nMinPerimeter = fPerimeter0010;
		nIndex = 0;
	}
	if ( ( fPerimeter0011 >= 0 ) && ( fPerimeter0011 < nMinPerimeter ) )
	{
		nMinPerimeter = fPerimeter0011;
		nIndex = 1;
	}
	if ( ( fPerimeter0110 >= 0 ) && ( fPerimeter0110 < nMinPerimeter ) )
	{
		nMinPerimeter = fPerimeter0110;
		nIndex = 2;
	}
	if ( ( fPerimeter0111 >= 0 ) && ( fPerimeter0111 < nMinPerimeter ) )
	{
		nIndex = 3;
	}

	if ( nIndex < 0 )
	{
		return false;
	}
	if ( nIndex == 0 )
	{
		pPointsSequence->insert( pPointsSequence->end(), road0010points.begin(), road0010points.end() );
	}
	else if ( nIndex == 1 )
	{
		pPointsSequence->insert( pPointsSequence->end(), road0011points.begin(), road0011points.end() );
	}
	else if ( nIndex == 2 )
	{
		pPointsSequence->insert( pPointsSequence->end(), road0110points.begin(), road0110points.end() );
	}
	else
	{
		pPointsSequence->insert( pPointsSequence->end(), road0111points.begin(), road0111points.end() );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::FindPath( const CVec2 &vBegin0, const CVec2 &vEnd0, bool bBegin0,
														const CVec2 &vBegin1, const CVec2 &vEnd1, bool bBegin1,
														float fRadius, int nSegmentsCount, float fMinEdgeLength, float fDistance, float fDisturbance,
														list<CVec2> *pPointsSequence, const vector<vector<CVec2> > &rLockedPolygons, list<CVec2> *pUsedPoints,
														int nDepth )
{
	if ( nDepth < 0 )
	{
		return false;
	}
	//list<CVec2> currentPath0;
	if ( !GetPointsSequence( vBegin0, vEnd0, fRadius, nSegmentsCount, bBegin0, vBegin1, vEnd1, fRadius, nSegmentsCount, bBegin1, pPointsSequence ) )
	{
		return false;
	}
	RandomizeEdges<list<CVec2>, CVec2>( ( *pPointsSequence ), 10, fDistance, CTPoint<float>( 0.0f, fDisturbance ), pPointsSequence, fMinEdgeLength, 32 * 16 * AI_TILE_SIZE, false );
	UniquePolygon<list<CVec2>, CVec2>( pPointsSequence, MINIMAL_POINT_DISTANCE );
	return true;

	/**
	list<CVec2>::const_iterator itCurrentPath0 = currentPath0.begin();
	list<CVec2>::const_iterator itCurrentPath1 = currentPath0.begin();
	++itCurrentPath1;
	while ( itCurrentPath1 != currentPath0.end() )
	{
		for ( vector<vector<CVec2> >::const_iterator itLockedPolygon = rLockedPolygons.begin(); itLockedPolygon != rLockedPolygons.end(); ++itLockedPolygon )
		{
			if ( itLockedPolygon->size() < 2 )
			{
				continue;
			}
			
			vector<CVec2>::const_iterator itCurrentPoint0;
			vector<CVec2>::const_iterator itCurrentPoint1;
			float fCrossPoint;
			EClassifyIntersection classifyIntersection = ClassifyCross( ( *itLockedPolygon ), ( *itCurrentPath0 ), ( *itCurrentPath1 ), &itCurrentPoint0, &itCurrentPoint1, &fCrossPoint );
			if ( classifyIntersection == CI_SKEW_CROSS )
			{
				bool bBeginUsed = false;
				bool bEndUsed = false;
				for ( list<CVec2>::const_iterator itUsedPoint = pUsedPoints->begin(); itUsedPoint != pUsedPoints->end(); ++itUsedPoint )
				{
					if ( ( *itUsedPoint ) == ( *itCurrentPoint0 ) )
					{
						bBeginUsed = true;
					}
					if ( ( *itUsedPoint ) == ( *itCurrentPoint1 ) )
					{
						bEndUsed = true;
					}
					if ( bBeginUsed && bEndUsed )
					{
						return false;
					}
				}

				CVec2 v0;
				CVec2 v1;
				CVec2 v2;
				if ( ( fCrossPoint < 0.5f ) && ( !bBeginUsed ) )
				{
					if ( itCurrentPoint0 == itLockedPolygon->begin() )
					{
						v0 = ( *( itLockedPolygon->rbegin() ) );
					}
					else
					{
						--itCurrentPoint0;
						v0 = ( *itCurrentPoint0 );
						++itCurrentPoint0;
					}
					v1 = ( *itCurrentPoint0 );
					v2 = ( *itCurrentPoint1 );
				}
				else
				{
					v0 = ( *itCurrentPoint0 );
					v1 = ( *itCurrentPoint1 );
					if ( itCurrentPoint1 == ( --( itLockedPolygon->end() ) ) )
					{
						v2 = ( *( itLockedPolygon->begin() ) );
					}
					else
					{
						++itCurrentPoint1;
						v2 = ( *itCurrentPoint1 );
						--itCurrentPoint1;
					}
				}
				pUsedPoints->push_front( v1 );

				CVec2 v01 = ( v0 - v1 );
				CVec2 v21 = ( v2 - v1 );
				Normalize( &v01 );
				Normalize( &v21 );
				CVec2 vNormal = ( v01 + v21 );
				Normalize( &vNormal );
				EClassifyRotation classifyRotation0 = ClassifyRotation( *itLockedPolygon );
				EClassifyRotation classifyRotation1 = ClassifyRotation( v0, v1, v2 );
				if ( classifyRotation0 == classifyRotation1 )
				{
					vNormal *= ( -1 );
				}
				CVec2 vNewBegin = v1 + vNormal * fRadius;
				CVec2 vNewEnd0 = vNewBegin + GetNormal( vNormal );
				CVec2 vNewEnd1 = vNewBegin - GetNormal( vNormal );

				GetClassifyEdgeInnerSpace( classifyRotation0 );
				if ( ClassifyEdge( ( *itCurrentPath0 ), ( *itCurrentPath1 ), v1 ) == CE_RIGHT )
				{
					CVec2 vTemp = vNewEnd0;
					vNewEnd0 = vNewEnd1;
					vNewEnd1 = vTemp;
				}
				currentPath0.clear();
				list<CVec2> currentPath1;				
				if ( FindPath( vBegin0, vEnd0, bBegin0,
											 vNewBegin, vNewEnd0, true,
											 fRadius, nSegmentsCount, fMinEdgeLength,
											 &currentPath0, rLockedPolygons, pUsedPoints,
											 nDepth - 1 ) &&
						 FindPath( vNewBegin, vNewEnd1, true,
											 vBegin1, vEnd1, bBegin1,
											 fRadius, nSegmentsCount, fMinEdgeLength,
											 &currentPath1, rLockedPolygons, pUsedPoints,
											 nDepth - 1 ) )
				{
					//слить оболочки
					pPointsSequence->insert( pPointsSequence->end(), currentPath0.begin(), currentPath0.end() );
					currentPath1.erase( currentPath1.begin() );
					pPointsSequence->insert( pPointsSequence->end(), currentPath1.begin(), currentPath1.end() );
					//UniquePolygon<list<CVec2>, CVec2>( pPointsSequence, RMGC_MINIMAL_VIS_POINT_DISTANCE );
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		++itCurrentPath0;
		++itCurrentPath1;
	}
	
	pPointsSequence->insert( pPointsSequence->end(), currentPath0.begin(), currentPath0.end() );
	//UniquePolygon<list<CVec2>, CVec2>( pPointsSequence, RMGC_MINIMAL_VIS_POINT_DISTANCE );
	return true;
	/**/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//первый VSO продолжается на 2 controlPoints, край уводится в 0
bool CVSOManager::Merge( NDb::SVSOInstance *pVSO0, bool bVSO0Begin,
												 NDb::SVSOInstance *pVSO1, bool bVSO1Begin )
{
	SBackupKeyPoints backupKeyPoints0;
	SBackupKeyPoints backupKeyPoints1;
	backupKeyPoints0.SaveKeyPoints( pVSO0->points );
	backupKeyPoints1.SaveKeyPoints( pVSO1->points );

	CVec3 vPointToAdd0 = VNULL3;
	CVec3 vPointToAdd1 = VNULL3;
	float fWidthToAdd0 = DEFAULT_WIDTH;
	float fWidthToAdd1 = DEFAULT_WIDTH;
	
	if ( bVSO1Begin )
	{
		backupKeyPoints1.SetFrontOpacity( 0.0f );

		if ( pVSO1->controlPoints.size() < 3 )
		{
			vPointToAdd0 = ( pVSO1->controlPoints.front() + pVSO1->controlPoints.back() ) / 2.0f;
			vPointToAdd1 = pVSO1->controlPoints.back();

			fWidthToAdd0= ( pVSO1->points.front().fWidth + pVSO1->points.back().fWidth ) / 2.0f;
			fWidthToAdd1= pVSO1->points.back().fWidth;
		}
		else
		{
			vPointToAdd0 = *( pVSO1->controlPoints.begin() + 1 );
			vPointToAdd1 = *( pVSO1->controlPoints.begin() + 2 );
			
			bool bFirst = true;
			bool bWidthToAdd0 = false;
			bool bWidthToAdd1 = false;
			for ( CVSOPointList::iterator itPoint = pVSO1->points.begin(); itPoint != pVSO1->points.end(); ++itPoint )
			{
				if ( itPoint->bKeyPoint )
				{
					if ( bFirst )
					{
						bFirst = false;
					}
					else if ( !bWidthToAdd0 )
					{
						fWidthToAdd0 = itPoint->fWidth;
						bWidthToAdd0 = true;
					}
					else if ( !bWidthToAdd1 )
					{
						fWidthToAdd1= itPoint->fWidth;
						bWidthToAdd1 = true;
					}
					if ( bWidthToAdd0 && bWidthToAdd1 )
					{
						break;
					}
				}
			}
		}
	}
	else
	{
		backupKeyPoints1.SetBackOpacity( 0.0f );
		
		if ( pVSO1->controlPoints.size() < 3 )
		{
			vPointToAdd0 = ( pVSO1->controlPoints.front() + pVSO1->controlPoints.back() ) / 2.0f;
			vPointToAdd1 = pVSO1->controlPoints.front();

			fWidthToAdd0= ( pVSO1->points.front().fWidth + pVSO1->points.back().fWidth ) / 2.0f;
			fWidthToAdd1= pVSO1->points.front().fWidth;
		}
		else
		{
			vPointToAdd0 = *( pVSO1->controlPoints.end() - 2 );
			vPointToAdd1 = *( pVSO1->controlPoints.end() - 3 );
			
			bool bFirst = true;
			bool bWidthToAdd0 = false;
			bool bWidthToAdd1 = false;
			for ( CVSOPointList::const_iterator itPoint = pVSO1->points.end(); itPoint != pVSO1->points.begin(); )
			{
				--itPoint;
				if ( itPoint->bKeyPoint )
				{
					if ( bFirst )
					{
						bFirst = false;
					}
					else if ( !bWidthToAdd0 )
					{
						fWidthToAdd0 = itPoint->fWidth;
						bWidthToAdd0 = true;
					}
					else if ( !bWidthToAdd1 )
					{
						fWidthToAdd1= itPoint->fWidth;
						bWidthToAdd1 = true;
					}
					if ( bWidthToAdd0 && bWidthToAdd1 )
					{
						break;
					}
				}
			}
		}
	}

	if ( bVSO0Begin )
	{
		backupKeyPoints0.InsertToFront( fWidthToAdd0, DEFAULT_HEIGHT, pVSO0->points.front().fOpacity );
		backupKeyPoints0.InsertToFront( fWidthToAdd1, DEFAULT_HEIGHT, 0.0f );
		// push_front отсутствует у vector
		pVSO0->controlPoints.insert( pVSO0->controlPoints.begin(), vPointToAdd0 );
		pVSO0->controlPoints.insert( pVSO0->controlPoints.begin(), vPointToAdd1 );
	}
	else
	{
		backupKeyPoints0.InsertToBack( fWidthToAdd0, DEFAULT_HEIGHT, pVSO0->points.back().fOpacity );
		backupKeyPoints0.InsertToBack( fWidthToAdd1, DEFAULT_HEIGHT, 0.0f );
		pVSO0->controlPoints.push_back( vPointToAdd0 );
		pVSO0->controlPoints.push_back( vPointToAdd1 );
	}
	//	
	Update( pVSO0, true, true, DEFAULT_STEP, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_OPACITY, true, false, true, false, false );
	backupKeyPoints0.LoadKeyPoints( &( pVSO0->points ) );
	Update( pVSO0, true, true, DEFAULT_STEP, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_OPACITY, true, false, true, false, false );
	//
	Update( pVSO1, true, true, DEFAULT_STEP, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_OPACITY, true, false, true, false, false );
	backupKeyPoints1.LoadKeyPoints( &( pVSO1->points ) );
	Update( pVSO1, true, true, DEFAULT_STEP, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_OPACITY, true, false, true, false, false );
	//
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//вернуть первую не нулевую высоту или высоту на конце ( true )
float CVSOManager::GetEdgeHeght( const CVSOPointList &rVSOPointList, bool bBegin, bool bFirst )
{
	float fHeight = 0.0f;
	bool bFirstFound = false;
	for ( int nPointIndex = 0; nPointIndex < rVSOPointList.size(); ++nPointIndex )
	{
		CVec3 vPos;
		if ( bBegin )
		{
			vPos = rVSOPointList[nPointIndex].vPos;
		}
		else
		{
			vPos = rVSOPointList[rVSOPointList.size() - nPointIndex - 1].vPos;
		}
		fHeight = GetTerrainHeight( vPos.x, vPos.y );
		//fHeight = GetTerrainHeight( vPos.x, vPos.y, false );
		if ( fHeight != 0.0f )
		{
			if ( bFirstFound || bFirst )
			{
				break;
			}
			bFirstFound = true;
		}
	}
	return fHeight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::GetVSOSelection( SVSOSelection *pVSOSelection,
																	 const CVec3 &rvPos,
																	 const CVec3 &rvOrigin,
																	 const CVec3 &rvDirection,
																	 const NDb::SVSOInstance &rVSO,
																	 const CVSOSelectionParamList &rVSOSelectionParamList )
{
	NI_ASSERT( pVSOSelection != 0, "CVSOManager::GetVSOSelection(): Wrong parameter: pVSOSelection == 0" );
	//
	const CVec3 vTerrainPos = CVec3( rvPos.x, rvPos.y, 0.0f );
	//
	for ( CVSOSelectionParamList::const_iterator itVSOSelectionParam = rVSOSelectionParamList.begin(); itVSOSelectionParam != rVSOSelectionParamList.end(); ++itVSOSelectionParam )
	{
		if ( itVSOSelectionParam->fRadius > 0.0f )
		{
			switch( itVSOSelectionParam->eSelectionType )
			{
				case SVSOSelection::ST_CONTROL:
				{
					for ( int nControlPointsIndex = 0; nControlPointsIndex < rVSO.controlPoints.size(); ++nControlPointsIndex )
					{
						pVSOSelection->vDifference = vTerrainPos - rVSO.controlPoints[nControlPointsIndex];
						pVSOSelection->vDifference.z = 0.0f;
						if ( fabs( pVSOSelection->vDifference ) <= itVSOSelectionParam->fRadius )
						{
							pVSOSelection->eSelectionType = CVSOManager::SVSOSelection::ST_CONTROL;
							pVSOSelection->nIndex = nControlPointsIndex;
							return true;
						}
					}
				}
				case SVSOSelection::ST_CENTER:
				{
					for ( int nPointIndex = 0; nPointIndex < rVSO.points.size(); ++nPointIndex )
					{
						if ( rVSO.points[nPointIndex].bKeyPoint )
						{
							CVec3 vCenterPoint = rVSO.points[nPointIndex].vPos;
							vCenterPoint.z = 0.0f;
							UpdateTerrainHeight( &vCenterPoint );
							vCenterPoint.z += rVSO.points[nPointIndex].vPos.z;
							if ( GetDistanceTo3DLine( vCenterPoint, rvOrigin, rvDirection ) <= itVSOSelectionParam->fRadius )
							{
								pVSOSelection->vDifference = rVSO.points[nPointIndex].vPos;
								pVSOSelection->eSelectionType = CVSOManager::SVSOSelection::ST_CENTER;
								pVSOSelection->nIndex = nPointIndex;
								pVSOSelection->fOpacity = rVSO.points[nPointIndex].fOpacity;
								return true;
							}
						}
					}
				}
				case SVSOSelection::ST_NORMALE:
				{
					for ( int nPointIndex = 0; nPointIndex < rVSO.points.size(); ++nPointIndex )
					{
						if ( rVSO.points[nPointIndex].bKeyPoint )
						{
							pVSOSelection->vDifference = vTerrainPos - ( rVSO.points[nPointIndex].vPos + rVSO.points[nPointIndex].vNorm * rVSO.points[nPointIndex].fWidth );
							pVSOSelection->vDifference.z = 0.0f;
							if ( fabs( pVSOSelection->vDifference ) <= itVSOSelectionParam->fRadius )
							{
								pVSOSelection->eSelectionType = CVSOManager::SVSOSelection::ST_NORMALE;
								pVSOSelection->nIndex = nPointIndex;
								pVSOSelection->fOpacity = rVSO.points[nPointIndex].fOpacity;
								return true;
							}
						}
					}
				}
				case SVSOSelection::ST_OPNORMALE:
				{
					for ( int nPointIndex = 0; nPointIndex < rVSO.points.size(); ++nPointIndex )
					{
						if ( rVSO.points[nPointIndex].bKeyPoint )
						{
							pVSOSelection->vDifference = vTerrainPos - ( rVSO.points[nPointIndex].vPos - rVSO.points[nPointIndex].vNorm * rVSO.points[nPointIndex].fWidth );
							pVSOSelection->vDifference.z = 0.0f;
							if ( fabs( pVSOSelection->vDifference ) <= itVSOSelectionParam->fRadius )
							{
								pVSOSelection->eSelectionType = CVSOManager::SVSOSelection::ST_OPNORMALE;
								pVSOSelection->nIndex = nPointIndex;
								pVSOSelection->fOpacity = rVSO.points[nPointIndex].fOpacity;
								return true;
							}
						}
					}
				}
				default:
					break;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVSOManager::DrawVSO( SVSODrawParams *pDrawParams )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		CVSOPointList *pPoints = 0;
		vector<CVec3> *pControlPoints = 0;
		//
		switch ( pDrawParams->eDrawer )
		{
		case SVSODrawParams::BAD_DRAWER:
			break;
		case SVSODrawParams::VSO_IS_SELECT:
		case SVSODrawParams::VSO_IS_EDIT:
		case SVSODrawParams::ADV_CLIPBOARD:
			{
				if ( pDrawParams->pSelectedVSO )
				{
					pPoints = &( pDrawParams->pSelectedVSO->points );
					pControlPoints = &( pDrawParams->pSelectedVSO->controlPoints );
				}
			}
			break;
		case SVSODrawParams::VSO_IS_ADD:
			{
				if ( pDrawParams->pVSO )
				{
					pPoints = &( pDrawParams->pVSO->points );
					pControlPoints = &( pDrawParams->pVSO->controlPoints );
				}
			}
			break;
		}

		if ( ( pPoints != 0 ) && ( pControlPoints != 0 ) )
		{
			list<CVec3> centerPointList;
			list<CVec3> upperCenterPointList;
			list<CVec3> normalePointList;
			list<CVec3> opNormalePointList;
			for ( int nPointIndex = 0; nPointIndex < pPoints->size(); ++nPointIndex )
			{
				CVec3 vCenterPoint = ( *pPoints )[nPointIndex].vPos;
				UpdateTerrainHeight( &vCenterPoint );
				//
				CVec3 vUpperCenterPoint = vCenterPoint;
				vUpperCenterPoint.z += ( *pPoints )[nPointIndex].vPos.z;
				//
				CVec3 vNormalePoint = ( *pPoints )[nPointIndex].vPos + ( *pPoints )[nPointIndex].vNorm * ( *pPoints )[nPointIndex].fWidth;
				UpdateTerrainHeight( &vNormalePoint );
				//
				CVec3 vOpNormalePoint = ( *pPoints )[nPointIndex].vPos - ( *pPoints )[nPointIndex].vNorm * ( *pPoints )[nPointIndex].fWidth;
				UpdateTerrainHeight( &vOpNormalePoint );
				//
				if ( ( *pPoints )[nPointIndex].bKeyPoint )
				{
					if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_CENTER ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vCenterPoint, vUpperCenterPoint, CENTER_LINE_COLOR, false );
						pDrawParams->pSceneDrawTool->DrawCircle( vUpperCenterPoint, CENTER_POINT_RADIUS, CENTER_POINT_PARTS, CENTER_POINT_COLOR, false );
					}
					//
					if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) && pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vNormalePoint, vOpNormalePoint, NORMALE_LINE_COLOR, false );
					}
					else if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vCenterPoint, vNormalePoint, NORMALE_LINE_COLOR, false );
					}
					else if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vCenterPoint, vOpNormalePoint, NORMALE_LINE_COLOR, false );
					}
					//
					if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawCircle( vNormalePoint, NORMALE_POINT_RADIUS, NORMALE_POINT_PARTS, NORMALE_POINT_COLOR, false );
					}
					if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawCircle( vOpNormalePoint, NORMALE_POINT_RADIUS, NORMALE_POINT_PARTS, NORMALE_POINT_COLOR, false );
					}
				}
				else
				{
					const DWORD nAlpha = DWORD( ( *pPoints )[nPointIndex].fOpacity * 0xFF ) & 0xFF;
					DWORD nColor = DEFAULT_LINE_COLOR;
					UpdateAlphaARGBColor( &nColor, nAlpha );
					if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_CENTER ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vCenterPoint, vUpperCenterPoint, nColor, false );
					}
					if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) && pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vNormalePoint, vOpNormalePoint, nColor, false );
					}
					else if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vCenterPoint, vNormalePoint, nColor, false );
					}
					else if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
					{
						pDrawParams->pSceneDrawTool->DrawLine( vCenterPoint, vOpNormalePoint, nColor, false );
					}
				}
				centerPointList.push_back( vCenterPoint );
				upperCenterPointList.push_back( vUpperCenterPoint );
				normalePointList.push_back( vNormalePoint );
				opNormalePointList.push_back( vOpNormalePoint );
			}
			pDrawParams->pSceneDrawTool->DrawPolyline( centerPointList, DEFAULT_LINE_COLOR, false, false );
			if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_CENTER ) )
			{
				pDrawParams->pSceneDrawTool->DrawPolyline( upperCenterPointList, CENTER_LINE_COLOR, false, false );
			}
			if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) )
			{
				pDrawParams->pSceneDrawTool->DrawPolyline( normalePointList, NORMALE_LINE_COLOR, false, false );
			}
			if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
			{
				pDrawParams->pSceneDrawTool->DrawPolyline( opNormalePointList, NORMALE_LINE_COLOR, false, false );
			}
			//
			if ( pDrawParams->CanEditPoints( CVSOManager::SVSOSelection::ST_CONTROL ) || 
				( pDrawParams->eDrawer == SVSODrawParams::VSO_IS_ADD ) )
			{
				list<CVec3> pointList;
				for ( int nControlPointIndex = 0; nControlPointIndex < pControlPoints->size(); ++nControlPointIndex )
				{
					CVec3 vControlPoint = ( *pControlPoints )[nControlPointIndex];
					UpdateTerrainHeight( &vControlPoint );
					//
					pDrawParams->pSceneDrawTool->DrawCircle( vControlPoint, CONTROL_POINT_RADIUS, CONTROL_POINT_PARTS, CONTROL_POINT_COLOR, false );
					pointList.push_back( vControlPoint );
				}
				pDrawParams->pSceneDrawTool->DrawPolyline( pointList, CONTROL_LINE_COLOR, pDrawParams->bIsClose, false );
			}
			//
			if ( pDrawParams->pCurrSelection && pDrawParams->pCurrSelection->IsValid() )
			{
				if ( pDrawParams->pCurrSelection->IsControlPointType() )
				{
					if ( ( pDrawParams->pCurrSelection->nIndex ) >= 0 && 
						( pDrawParams->pCurrSelection->nIndex < pControlPoints->size() ) )
					{
						float fRadius = CONTROL_POINT_RADIUS - 1.0f;
						CVec3 vControlPoint = ( *pControlPoints )[pDrawParams->pCurrSelection->nIndex];
						UpdateTerrainHeight( &vControlPoint );
						while( fRadius > 0.0f )
						{
							pDrawParams->pSceneDrawTool->DrawCircle( vControlPoint, fRadius, CONTROL_POINT_PARTS, CONTROL_POINT_COLOR, false );
							fRadius -= 2.0f;
						}
					}
				}
				else if ( pDrawParams->pCurrSelection->IsCenterPointType() || pDrawParams->pCurrSelection->IsNormalePointType() )
				{
					if ( ( pDrawParams->pCurrSelection->nIndex ) >= 0 && ( pDrawParams->pCurrSelection->nIndex < pPoints->size() ) )
					{		
						float fRadius = CENTER_POINT_RADIUS - 1.0f;
						if ( pDrawParams->pCurrSelection->eSelectionType == CVSOManager::SVSOSelection::ST_CENTER )
						{
							CVec3 vCenterPoint = ( *pPoints )[pDrawParams->pCurrSelection->nIndex].vPos;
							UpdateTerrainHeight( &vCenterPoint );
							vCenterPoint.z += ( *pPoints )[pDrawParams->pCurrSelection->nIndex].vPos.z;
							//
							while( fRadius > 0.0f )
							{
								pDrawParams->pSceneDrawTool->DrawCircle( vCenterPoint, fRadius, CENTER_POINT_PARTS, CENTER_POINT_COLOR, false );
								fRadius -= 2.0f;
							}
						}
						else if ( pDrawParams->pCurrSelection->eSelectionType == CVSOManager::SVSOSelection::ST_NORMALE )
						{
							CVec3 vNormalePoint = ( *pPoints )[pDrawParams->pCurrSelection->nIndex].vPos + ( *pPoints )[pDrawParams->pCurrSelection->nIndex].vNorm * ( *pPoints )[pDrawParams->pCurrSelection->nIndex].fWidth;
							UpdateTerrainHeight( &vNormalePoint );
							//
							while( fRadius > 0.0f )
							{
								pDrawParams->pSceneDrawTool->DrawCircle( vNormalePoint, fRadius, NORMALE_POINT_PARTS, NORMALE_POINT_COLOR, false );
								fRadius -= 2.0f;
							}
						}
						else if ( pDrawParams->pCurrSelection->eSelectionType == CVSOManager::SVSOSelection::ST_OPNORMALE )
						{
							CVec3 vOpNormalePoint = ( *pPoints )[pDrawParams->pCurrSelection->nIndex].vPos - ( *pPoints )[pDrawParams->pCurrSelection->nIndex].vNorm * ( *pPoints )[pDrawParams->pCurrSelection->nIndex].fWidth;
							UpdateTerrainHeight( &vOpNormalePoint );
							while( fRadius > 0.0f )
							{
								pDrawParams->pSceneDrawTool->DrawCircle( vOpNormalePoint, fRadius, NORMALE_POINT_PARTS, NORMALE_POINT_COLOR, false );
								fRadius -= 2.0f;
							}
						}
					}
				}
			}
		}
		//
		if ( pDrawParams->bIsDrawSceneDrawTool )
		{
			pDrawParams->pSceneDrawTool->Draw();
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CVSOManager::UpdateZ( NDb::SVSOInstance *pVSO )
{
	NI_ASSERT( pVSO != 0, "CVSOManager::UpdateZ(): Wrong parameter: pVSO " );
	
	bool bResult = true;
	for ( vector<CVec3>::iterator itControlPoint = pVSO->controlPoints.begin(); itControlPoint != pVSO->controlPoints.end(); ++itControlPoint )
	{
		UpdateTerrainHeight( &( *itControlPoint ) );
	}
	for ( CVSOPointList::iterator itPoint = pVSO->points.begin(); itPoint != pVSO->points.end(); ++itPoint )
	{
		UpdateTerrainHeight( &( itPoint->vPos ) );
	}
	return bResult;
}
/**/
