#include "stdafx.h"

#include "UnitsSegments.h"
#include "AIUnit.h"
//#include "..\Common_RTS_AI\CollisionInternal.h"


extern NTimer::STime curTime;

//*******************************************************************
//*													CFreezeSegments													*
//*******************************************************************

void CFreezeSegments::SetSegmentObject( CCommonUnit *_pUnit )
{
	pUnit = _pUnit;
}

bool CFreezeSegments::Check()
{ 
	return pUnit && pUnit->IsRefValid() && ( pUnit->IsAlive() || pUnit->IsTrain() );
}

const NTimer::STime CFreezeSegments::ProcessSegment()
{
//	MPLog( "%d, freeze %d, (%g,%g,%g), %d", curTime, pUnit->GetUniqueId(), pUnit->GetCenter().x, pUnit->GetCenter().y, pUnit->GetZ(), pUnit->GetDirection() );
	pUnit->FreezeSegment();
//	MPLog( "%d, freeze %d, (%g,%g,%g), %d", curTime, pUnit->GetUniqueId(), pUnit->GetCenter().x, pUnit->GetCenter().y, pUnit->GetZ(), pUnit->GetDirection() );

	RecordRandomCall();
	return curTime + NRandom::Random( 500, 1500 );
}

//*******************************************************************
//*														CStateSegments												*
//*******************************************************************

void CStateSegments::SetSegmentObject( CCommonUnit *_pUnit )
{
	pUnit = _pUnit;
}

bool CStateSegments::Check()
{ 
	bIsValid = pUnit && pUnit->IsRefValid() && ( pUnit->IsAlive() || pUnit->IsTrain() );
	bCheck = bIsValid /*&& !pUnit->CanBeFrozen()*/;

	return bCheck;
}

const NTimer::STime CStateSegments::ProcessSegment()
{
//	MPLog( "%d, state %d, (%g,%g,%g), %d", curTime, pUnit->GetUniqueId(), pUnit->GetCenter().x, pUnit->GetCenter().y, pUnit->GetZ(), pUnit->GetDirection() );
	pUnit->Segment();
//	MPLog( "%d, state %d, (%g,%g,%g), %d", curTime, pUnit->GetUniqueId(), pUnit->GetCenter().x, pUnit->GetCenter().y, pUnit->GetZ(), pUnit->GetDirection() );
	return pUnit->GetNextSegmTime();
}

bool CStateSegments::ShouldBeUnregistered() const
{
	return bIsValid && !bCheck;
}

//*******************************************************************
//*													CFirstPathSegments											*
//*******************************************************************

void CFirstPathSegments::SetSegmentObject( CAIUnit *_pUnit )
{
	pUnit = _pUnit;
}

bool CFirstPathSegments::Check()
{
	return IsValidObj( pUnit );
}

const NTimer::STime CFirstPathSegments::ProcessSegment()
{
	if ( ( pUnit->IsAlive() || pUnit->GetStats()->IsTrain() ) && pUnit->IsFree() )
		pUnit->FirstSegment( SConsts::AI_SEGMENT_DURATION );

	return pUnit->GetNextPathSegmTime();
}

//*******************************************************************
//*													CSecondPathSegments											*
//*******************************************************************

void CSecondPathSegments::SetSegmentObject( CAIUnit *_pUnit )
{
	pUnit = _pUnit;
}

bool CSecondPathSegments::Check()
{
	return IsValidObj( pUnit );
}

const NTimer::STime CSecondPathSegments::ProcessSegment()
{
	if ( pUnit->IsFree() )
	{
		pUnit->SecondSegment( SConsts::AI_SEGMENT_DURATION );

		if ( pUnit->GetStats()->IsAviation() )
			pUnit->Moved();
		else
		{
			if ( pUnit->IsMoving() || ( pUnit->IsTurning() && pUnit->GetTurnSpeed() != 0 ) )
				pUnit->Moved();
			else
				pUnit->Stopped();
		}
	}
	const NTimer::STime timeResult = pUnit->GetNextSecondPathSegmTime();

	return timeResult;
}

//*******************************************************************
//*													CStayTimeSegments												*
//*******************************************************************

void CStayTimeSegments::SetSegmentObject( CAIUnit *_pUnit )
{
	pUnit = _pUnit;
}

bool CStayTimeSegments::Check()
{
	return IsValidObj( pUnit );
}

void CStayTimeSegments::ProcessSegment()
{
	if ( ( pUnit->IsAlive() || pUnit->GetStats()->IsTrain() ) && !pUnit->IsIdle() && !pUnit->GetStats()->IsAviation() )
	{
//		CPathUnit *pPathUnit = pUnit->GetPathUnit();
		const NTimer::STime collStayTime = pUnit->GetCollStayTime();
		if ( collStayTime != 0 )
		{
			const float fProb = ( 3000.0f + NRandom::Random( 0.0f, 800.0f ) ) / float( collStayTime ); RecordRandomCall();
			RecordRandomCall();
			if ( NRandom::Random( 0.0f, 1.0f ) > fProb )
			{
				CPtr<ICollision> pCollison = CreateCollision( pUnit, 0, -1, NCollision::ECN_STOP );
			}
		}

		pUnit->ResetCollStayTime();
		pUnit->ResetCollisionsCount();
	}
}


