#include "stdafx.h"
#include "PlanesFormation.h"
#include "ManuverBuilder.h"

#include "NewUpdater.h"

extern CEventUpdater updater;
extern CManuverBuilder theManuverBuilder;
extern NTimer::STime curTime;
int CPlanesFormation::nIDSoFar = 1;
det_map<int, bool> CPlanesFormation::existence;

//*******************************************************************
//*														CPlaneManuverHistory*
//*******************************************************************

IManuver * CPlaneManuverHistory::GetCurManuver() const
{ 
	//NI_ASSERT( !pathHistory.empty(), "unitialized path" );
	if ( pathHistory.empty() )
		return 0;
	return *pathHistory.begin(); 
}

void CPlaneManuverHistory::StartManuver( IManuver *pManuver )
{
	pathHistory.push_front( pManuver );
}

void CPlaneManuverHistory::ClearUnised()
{
	do 
	{
		IManuver *pManuver = pathHistory.back();
		if ( pManuver->IsUsed() )
		{
			pManuver->Used( false );
			return;
		}
		else
			pathHistory.pop_back();
	} while( true );
}

const SFormationMemberInfo & CPlaneManuverHistory::GetValues( const CVec3 &vOffset )
{
	IManuver *pManuver( *pathHistory.begin() );
	SFormationMemberInfo &cacheValue = memberCache[vOffset];
	if ( cacheValue.lastMoveTime != curTime )
	{
		//CRAP{ UNTILL REDO MANUVER TO SFormationMemeberInfo
		cacheValue.vWorldPosition = pManuver->GetPos();
		cacheValue.vSpeed = pManuver->GetSpeed();
		cacheValue.vNormal = pManuver->GetNormale();
		cacheValue.lastMoveTime = curTime;
		//CRAP}
	}
	return cacheValue;
}

void CPlaneManuverHistory::Advance( const NTimer::STime timeDiff )
{
	NI_ASSERT( !pathHistory.empty(), "unitialized" );
	if ( curPos.lastMoveTime != curTime )
	{
		IManuver *pManuver( *pathHistory.begin() );
		bFinished = pManuver->Advance( timeDiff );

		//CRAP{ UNTILL REDO MANUVER TO SFormationMemeberInfo
		curPos.vWorldPosition = pManuver->GetPos();
		curPos.vSpeed = pManuver->GetSpeed();
		curPos.vNormal = pManuver->GetNormale();
		curPos.lastMoveTime = curTime;
		//CRAP}
	}
}

bool CPlaneManuverHistory::IsManuverFinished() const
{
	return bFinished;
}

//*******************************************************************
//*														CPlanesFormation*
//*******************************************************************

void CPlanesFormation::Clear()
{
	nIDSoFar = 1;
	existence.clear();
}

void CPlanesFormation::SetCanViolateHeghtLimits()
{
	preferences.SetCanViolateHeghtLimits();
}

bool CPlanesFormation::IsFormaionExists( int nID )
{
	return existence.find( nID ) != existence.end();
}

void CPlanesFormation::Advance( const NTimer::STime timeDiff )
{
	pathHistory.Advance( timeDiff );
	// update ALL new values here (vNewSpeed, etc)
	vNewPos = pathHistory.GetValues( VNULL3 ).vWorldPosition;
	vNewSpeed = pathHistory.GetValues( VNULL3 ).vSpeed;
	vNewNormal = pathHistory.GetValues( VNULL3 ).vNormal;
}

bool CPlanesFormation::IsManuverFinished() const
{
	return pathHistory.IsManuverFinished();
}

void CPlanesFormation::CreateManuver( const CVec3 &vCenter, int nUniqueID )
{
	pathHistory.StartManuver( theManuverBuilder.CreateManuver( this, vCenter, nUniqueID ) );
}

void CPlanesFormation::CreateManuver( class CPlanesFormation *pEnemy )
{
	pathHistory.StartManuver( theManuverBuilder.CreateManuver( this, pEnemy ) );
}

void CPlanesFormation::CreatePointManuver( const CVec3 &vPos, const bool bToHorisontal )
{
	pathHistory.StartManuver( theManuverBuilder.CreatePointManuver( this, vPos, bToHorisontal ) );
}

const CVec3 & CPlanesFormation::GetSpeed( const CVec3 &vFormationOffset )
{
	NI_ASSERT( !pathHistory.IsEmpty(), "tried to use unitialized path " );
	return pathHistory.GetValues( vFormationOffset ).vSpeed;
}

const CVec3 & CPlanesFormation::GetPoint( const CVec3 &vFormationOffset )
{
	NI_ASSERT( !pathHistory.IsEmpty(), "tried to use unitialized path " );
	return pathHistory.GetValues( vFormationOffset ).vWorldPosition;
}

const CVec3 & CPlanesFormation::GetNormale( const CVec3 &vFormationOffset )
{
	NI_ASSERT( !pathHistory.IsEmpty(), "tried to use unitialized path " );
	return pathHistory.GetValues( vFormationOffset ).vNormal;
}

void CPlanesFormation::AddProcessed() 
{ 
	++nProcessed; 
}

void CPlanesFormation::SetNewPos( const CVec3 &vCenter ) 
{ 
	vNewPos = vCenter;
}

void CPlanesFormation::AddAlive() 
{ 
	++nAlive; 
}

bool CPlanesFormation::IsAllProcessed() const 
{ 
	return nProcessed >= nAlive; 
}

void CPlanesFormation::SecondSegment( const NTimer::STime timeDiff )
{
	vSpeed = vNewSpeed;
	vNormal = vNewNormal;
	vPos= vNewPos;
	
	pathHistory.ClearUnised();
}

void CPlanesFormation::Init( const NDb::SMechUnitRPGStats *pStats, const CVec3 &vCenter, const float fTurnRadiusMin, const float fTurnRadiusMax, const CVec3 &_vSpeed, const CVec3 &_vNormale, const float _fBombPointOffset, bool bCanViolateHeghtLimits )
{
	preferences.Init( pStats, bCanViolateHeghtLimits );
	nAlive = nProcessed = 0;
	fBombPointOffset = _fBombPointOffset;

	vPos = vNewPos = vCenter;
	vNewSpeed = vSpeed = _vSpeed;
	vNormal = vNewNormal = _vNormale;
}

