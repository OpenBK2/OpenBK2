#include "stdafx.h"

#include "BasePathUnit.h"
#include "Tools.h"

const float BOUND_RECT_FACTOR = 1.0f;
const int SPEED_FACTOR = 800;

//*******************************************************************
//*														CBSplne																*
//*******************************************************************

const float CBSpline::DELTA = 0.02f;
const float CBSpline::DELTA_FORWARD = 0.08f;
const int CBSpline::N_OF_ITERATONS = 1 / CBSpline::DELTA;
const int CBSpline::N_ITERS_TO_FORWARD = CBSpline::DELTA_FORWARD / CBSpline::DELTA;

void CBSpline::Init( const CVec2 &p3, const CVec2 &p2, const CVec2 &p1, const CVec2 &p0 )
{
	a = 1.0f/6.0f * ( -p3 + 3 * p2 - 3 * p1 + p0 );
	b = 1.0f/6.0f * ( 3 * p3 - 6 * p2 + 3 * p1 );
	c = 1.0f/6.0f * ( -3 * p3 + 3 * p1 );
	d = 1.0f/6.0f * ( p3 + 4 * p2 + p1 );

	// для построения сплайна
	d3x = a * sqr( DELTA ) * DELTA;
	dx = d3x + b * sqr( DELTA );
	d2x = 2 * ( 2 * d3x + dx );
	dx += c * DELTA;
	d3x *= 6;
	x = d;

	// для просмотра вперёд
	fw_d3x = a * sqr( DELTA_FORWARD ) * DELTA_FORWARD;
	fw_dx = fw_d3x + b * sqr( DELTA_FORWARD );
	fw_d2x = 2 * ( 2 * fw_d3x + fw_dx );
	fw_dx += c * DELTA_FORWARD;
	fw_d3x *= 6;

	t = tForward = 0;
	cntToForward = 0;
	/*
	dx = a * sqr( del ) * del + b * sqr( del ) + c * del;
	d2x = 6 * a * sqr( del ) * del + 2 * b * sqr( del );
	d3x = 6 * a * sqr( del ) * del;
	*/
}

void CBSpline::Iterate()
{
	x += dx;
	dx += d2x;
	d2x += d3x;
	t += DELTA;

	++cntToForward;
	if ( cntToForward == N_ITERS_TO_FORWARD )
	{
		fw_dx += fw_d2x;
		fw_d2x += fw_d3x;
		tForward += DELTA_FORWARD;
		cntToForward = 0;
	}
}

const float CBSpline::GetReverseR() const 
{ 
	const CVec2 first = 3 * a * sqr( t ) + 2 * b * t + c;
	const CVec2 second = 6 * a * t + 2 * b;
	const float tanLen = fabs( first );

	return fabs( first.x * second.y - first.y * second.x ) / ( sqr( tanLen ) * tanLen );
}

const void CBSpline::StartForwardIterating( SForwardIter *pIter )
{
	pIter->t = tForward;
	pIter->x = x;
	pIter->fw_dx = fw_dx;
	pIter->fw_d2x = fw_d2x;
	pIter->fw_d3x = fw_d3x;
}

const void CBSpline::IterateForward( SForwardIter *pIter )
{
	if ( pIter->t != -1 && pIter->t + DELTA_FORWARD <= 1 )
	{
		pIter->t += DELTA_FORWARD;
		pIter->x += pIter->fw_dx;
		pIter->fw_dx += pIter->fw_d2x;
		pIter->fw_d2x += pIter->fw_d3x;
	}
	else
		pIter->t = -1;
}

int CBSpline ::operator&( IBinSaver &saver )
{

	saver.Add( 1, &a );
	saver.Add( 2, &b );
	saver.Add( 3, &c );
	saver.Add( 4, &d );
	saver.Add( 5, &x );
	saver.Add( 6, &dx );
	saver.Add( 7, &d2x );
	saver.Add( 8, &d3x );
	saver.Add( 9, &fw_dx );
	saver.Add( 10, &fw_d2x );
	saver.Add( 11, &fw_d3x );
	saver.Add( 12, &t );
	saver.Add( 13, &tForward );
	saver.Add( 14, &cntToForward );
	return 0;
}

void CBSpline::DumpState() const
{
	Singleton<IConsoleBuffer>()->WriteASCII( 500, StrFmt("spline: x=(%g,%g), dx=(%g,%g), d2x=(%g,%g), d3x=(%g,%g)", x.x, x.y, dx.x, dx.y, d2x.x, d2x.y, d3x.x, d3x.y ), 0, true );
}


//*******************************************************************
//*														CCircle																*
//*******************************************************************

float CCirclePath::Iterate( const float fLength )
{
	const float fDelta = fLength/fRadius;
	fCurrentDelta += fDelta;
	float fResult = 0.0f;

	if ( fCurrentDelta > fDirDiff )  
	{
		fResult = fLength -	( fDirDiff - fCurrentDelta ) * fRadius;
		fCurrentDelta = fDirDiff;
	}

	const float fCurrentDir =  ( bClockWise ) ? ( fStartDir - fCurrentDelta ) : ( fStartDir + fCurrentDelta );
	vDX = CVec2( NMath::Cos( fCurrentDir ), NMath::Sin( fCurrentDir ) );
	if ( !bForwardDir )
		vDX = -vDX;

	const float fCurrentPerp =  ( bClockWise == bForwardDir ) ? ( fCurrentDir + FP_PI2 ) : ( fCurrentDir - FP_PI2 );
	vX = CVec2( fRadius*NMath::Cos( fCurrentPerp ), fRadius*NMath::Sin( fCurrentPerp ) ) + vCenter;

	return fResult;
}

int CCirclePath::operator&( IBinSaver &saver )
{
	saver.Add( 1, &fStartDir );
	saver.Add( 2, &fDirDiff );
	saver.Add( 3, &vCenter );
	saver.Add( 4, &fRadius );
	saver.Add( 5, &bClockWise );
	saver.Add( 6, &fCurrentDelta );
	saver.Add( 7, &vX );
	saver.Add( 8, &vDX );
	saver.Add( 9, &vLastX );
	saver.Add( 10, &vLastDX );
	saver.Add( 11, &bForwardDir );

	return 0;
}

const SRect GetUnitFullSpeedRect( const CBasePathUnit *pUnit, const bool bForInfantry )
{
	const SUnitProfile profile( pUnit->GetUnitProfile() );
	const CVec2 vAABBHalfSize( profile.GetHalfWidth(), profile.GetHalfLength() );

	const float width = vAABBHalfSize.x * BOUND_RECT_FACTOR;// * 1.3f;
	const float lengthBack = vAABBHalfSize.y * BOUND_RECT_FACTOR;
	float lengthAhead;

	if ( !pUnit->IsIdle() )
	{
		if ( bForInfantry )
			lengthAhead = Max( vAABBHalfSize.y * BOUND_RECT_FACTOR + SPEED_FACTOR * pUnit->GetMaxPossibleSpeed(), vAABBHalfSize.y * BOUND_RECT_FACTOR * 1.5f );
		else
			lengthAhead = Max( vAABBHalfSize.y * BOUND_RECT_FACTOR + SPEED_FACTOR * pUnit->GetSpeed(), vAABBHalfSize.y * BOUND_RECT_FACTOR * 1.5f );
	}
	else
		lengthAhead = vAABBHalfSize.y * BOUND_RECT_FACTOR;

	SRect speedRect;
	speedRect.InitRect( pUnit->GetCenterPlain() + pUnit->GetCenterShift(), pUnit->GetDirectionVector(), lengthAhead, lengthBack, width );

	return speedRect;

}

const SRect GetUnitSpeedRect( const CBasePathUnit *pUnit, const bool bForInfantry )
{
	const SUnitProfile profile( pUnit->GetUnitProfile() );
	const CVec2 vAABBHalfSize( profile.GetHalfWidth(), profile.GetHalfLength() );

	const float width = vAABBHalfSize.x * BOUND_RECT_FACTOR;// * 1.3f;
	const float lengthBack = vAABBHalfSize.y * BOUND_RECT_FACTOR;
	float lengthAhead;
	if ( !pUnit->IsIdle() )
	{
		if ( bForInfantry )
			lengthAhead = Max( SPEED_FACTOR * pUnit->GetMaxPossibleSpeed(), vAABBHalfSize.y * 0.5f );
		else
			lengthAhead = Max( SPEED_FACTOR * pUnit->GetSpeed(), vAABBHalfSize.y * 0.5f );
	}
	else
		lengthAhead = 0;

	SRect speedRect;
	speedRect.InitRect( pUnit->GetCenterPlain()  + pUnit->GetCenterShift() + 2 * pUnit->GetDirectionVector() * vAABBHalfSize.y, 
		pUnit->GetDirectionVector(), lengthAhead, lengthBack, width );

	return speedRect;
}

const SRect GetUnitSmallRect( const CBasePathUnit *pUnit )
{
	const SUnitProfile profile( pUnit->GetUnitProfile() );
	const CVec2 vAABBHalfSize( profile.GetHalfWidth(), profile.GetHalfLength() );

	const float fCoeff = ( pUnit->IsInfantry() ) ? 0.5 : 0.8;
	const float fLength = vAABBHalfSize.y * fCoeff;
	const float fWidth = vAABBHalfSize.x * fCoeff;

	SRect smallRect;
	smallRect.InitRect( pUnit->GetCenterPlain() + pUnit->GetCenterShift(), pUnit->GetFrontDirectionVector(), fLength, fWidth );

	return smallRect;
}

const SRect GetUnitNormalRect( const CBasePathUnit *pUnit )
{
	const SUnitProfile profile( pUnit->GetUnitProfile() );
	const CVec2 vAABBHalfSize( profile.GetHalfWidth(), profile.GetHalfLength() );

	const float fLength = vAABBHalfSize.y * BOUND_RECT_FACTOR;
	const float fWidth = vAABBHalfSize.x * BOUND_RECT_FACTOR;
	
	SRect unitRect;
	unitRect.InitRect( pUnit->GetCenterPlain() + pUnit->GetCenterShift(), pUnit->GetFrontDirectionVector(), fLength, fWidth );

	return unitRect;
}

