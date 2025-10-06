#include "stdafx.h"

#include "Aviation.h"
#include "PlaneStates.h"
#include "PlanePath.h"
#include "GroupLogic.h"
#include "NewUpdater.h"
#include "AckManager.h"
#include "CombatEstimator.h"
#include "Statistics.h"
#include "AIUnitInfoForGeneral.h"
#include "GlobalWarFog.h"
#include "PlanesFormation.h"
#include "ManuverInternal.h"
// for profiling
#include "TimeCounter.h"
#include "AAFeedBacks.h"
#include "AIGeometry.h"
#include "UnitsIterators2.h"
#include "DebugTools/DebugInfoManager.h"


REGISTER_SAVELOAD_CLASS( 0x1108D443, CAviation );
REGISTER_SAVELOAD_CLASS( 0x1508D490, CPlanesFormation );

extern CAAFeedBacks theAAFeedBacks;
extern CDiplomacy theDipl;
extern CStatistics theStatistics;
extern CUnits units;
extern CCombatEstimator theCombatEstimator;
extern CAckManager theAckManager;
extern CEventUpdater updater;
extern CGroupLogic theGroupLogic;
extern NTimer::STime curTime;
extern CGlobalWarFog theWarFog;

extern NAI::CTimeCounter timeCounter;

//*******************************************************************
//*														CAviation															*
//*******************************************************************

CAviation::~CAviation()
{
}

void CAviation::DecFuel( const bool bEconomyMode )
{
	const float fDec = (bEconomyMode ?  SConsts::PLANE_FUEL_DEC_ECONOMY : SConsts::PLANE_FUEL_DEC ) * SConsts::AI_SEGMENT_DURATION / 1000;
	fFuel = (std::max)( fFuel - fDec, 0.0f );
	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
}

float CAviation::GetFuel() const
{
	return fFuel;
}

const SRect & CAviation::GetUnitRect() const
{
	static SRect unitRect;

	const float length = GetStats()->vAABBHalfSize.y * SConsts::BOUND_RECT_FACTOR;
	const float width = GetStats()->vAABBHalfSize.x * SConsts::BOUND_RECT_FACTOR;


	const CVec2 realDirVec( GetDirVector() );
	const CVec2 dirPerp( realDirVec.y, -realDirVec.x );
	const CVec2 vCenterShift( realDirVec * GetStats()->vAABBCenter.y + dirPerp * GetStats()->vAABBCenter.x );

	unitRect.InitRect( GetCenterPlain() + vCenterShift, GetDirVector(), length, width );
	return unitRect;
}

void CAviation::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *_pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
{
	pStats = checked_cast<const SMechUnitRPGStats*>( _pStats );
	// calculate appear height
	timeNextGroundScan = curTime;
	
	const float fAppearHeight = Clamp( pStats->fMaxHeight, SPlanesConsts::GetMinHeight(), SPlanesConsts::MAX_HEIGHT );

	const CVec2 vGoToPoint( center );
	const CVec2 vDirection( GetVectorByDirection( dir ) );
	const CVec2 vAppearPoint( center - vDirection * 100 );
	
	vInitialPoint = center;
	
	CVec2 vUitDir( vGoToPoint - vAppearPoint );
	Normalize( &vUitDir );


	// create formation if needed
	CPtr<CPlanesFormation> pTempFormation = new CPlanesFormation( 0 );
	pTempFormation->Init( pStats, CVec3(vAppearPoint, fAppearHeight), pStats->fTurnRadius, pStats->fTurnRadius * 2, 
		CVec3(vDirection * pStats->fSpeed,0), CVec3(0,0,1.0f), 0, false );
	SetPlanesFormation( pTempFormation, VNULL3 );
	fFuel = pStats->fFuel;


	CPtr<CPlaneInFormationSmoothPath> pPath = new CPlaneInFormationSmoothPath;
	pPath->Init( this );
	SetSmoothPath( pPath );
	pFormation->CreatePointManuver( CVec3(vGoToPoint, fAppearHeight), true );

	CMilitaryCar::Init( center, fAppearHeight, pStats, fHP, dir, player, pCollisionsCollector );

	SetSmoothPath( pPath );
	//pPath->Init( this );
	//SetSmoothPath( pPath );
	//pFormation->CreatePointManuver( CVec3(vGoToPoint, fAppearHeight), true );


	vSpeed = pFormation->GetSpeedB2();
	vPos = pFormation->GetPosB2();
	vNormale = pFormation->GetNormalB2();
	// SendAcknowledgement( ACK_PLANE_TAKING_OFF, true );
}

const class CPlanePreferences & CAviation::GetPreferencesB2() const 
{ 
	return pFormation->GetPreferencesB2(); 
}

const IManuver * CAviation::GetManuver() const 
{ 
	return pFormation->GetManuver(); 
}

IStatesFactory* CAviation::GetStatesFactory() const 
{ 
	return CPlaneStatesFactory::Instance(); 
}

const WORD CAviation::GetDir() const
{
	if ( pFormation )
	{
		const CVec3 vSpeed ( pFormation->GetSpeed( vPlanesShift ) );
		return GetDirectionByVector( vSpeed.x, vSpeed.y );
	}
	return CMilitaryCar::GetDirection();
}

const CVec2 CAviation::GetDirVector() const
{
	static CVec2 dirvec;
	if ( pFormation )
	{
		const CVec3 vSpeed( pFormation->GetSpeedB2() );
		static CVec2 vRet;
		vRet.x = vSpeed.x;
		vRet.y = vSpeed.y;
		return vRet;
	}
	return CMilitaryCar::GetDirectionVector();
}

const float CAviation::GetSpeed() const
{
	static CVec3 vSpeed;
	if ( pFormation )
	{
		vSpeed = pFormation->GetSpeed( vPlanesShift );
		static CVec2 vRet;
		vRet.x = vSpeed.x;
		vRet.y = vSpeed.y;
		return fabs( vRet );
	}
	return CMilitaryCar::GetSpeed();
}

void CAviation::SetPlanesFormation( class CPlanesFormation *_pFormation, const CVec3 &_vShift )
{
	if ( pFormation && !_pFormation )
		SetDirection( GetFrontDirection() );
	pFormation = _pFormation;
	vPlanesShift = _vShift;
}

CPlanesFormation * CAviation::GetPlanesFormation()
{
	return pFormation;
}

void CAviation::SecondSegment( const NTimer::STime timeDiff )
{
	if ( pFormation )
	{
		const CVec3 vSpeed( pFormation->GetSpeedB2() );
		SetDirection( GetDirectionByVector( vSpeed.x, vSpeed.y ) );
		//updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );

		pFormation->AddProcessed();
		if ( pFormation->IsAllProcessed() )
			pFormation->SecondSegment( timeDiff );
	}
	CMilitaryCar::SecondSegment( timeDiff );
}

CVec3 CAviation::GetSpeedB2() const 
{ 
	return pFormation->GetSpeedB2();	
}

CVec3 CAviation::GetPosB2() const 
{ 
	return pFormation->GetPosB2(); 
}

CVec3 CAviation::GetNormalB2() const 
{ 
	return pFormation->GetNormalB2(); 
}

CVec3 CAviation::GetPosNext() const 
{ 
	return pFormation->GetPosNext(); 
}

CVec3 CAviation::GetSpeedNext() const 
{ 
	return pFormation->GetSpeedNext(); 
}

CVec3 CAviation::GetNormalNext() const 
{ 
	return pFormation->GetNormalNext(); 
}

void CAviation::Segment()
{
	vSpeed = pFormation->GetSpeedNext();
	vPos = pFormation->GetPosNext();
	vNormale = pFormation->GetNormalNext();

	CMilitaryCar::Segment();
	if ( !IsAlive() ) 
		return;
	pFormation->AddAlive();

	// ground scan time
	if ( curTime > timeNextGroundScan )
	{
		bool bFound = false;
		// scan ground for enemy ground units
		for ( CUnitsIter<1,3> iter( GetParty(), EDI_ENEMY, GetCenterPlain(), 1000 ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( pUnit->IsRefValid() && pUnit->IsAlive() )
			{
				// if found - ask them to send acknowledgements
				pUnit->SendAcknowledgement( NDb::ACK_BEING_ATTACKED_BY_AVIATION, pUnit->GetPlayer() != theDipl.GetMyNumber() );
				break;
			}
		}

		timeNextGroundScan = curTime + 1000 + NRandom::Random( 0, 2000 );
	}
}

void CAviation::GetSpeed3( CVec3 *pSpeed ) const 
{
	*pSpeed = pFormation->GetSpeedB2();
}

void CAviation::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	//
	pPlacement->bNewFormat = true;
	pPlacement->nObjUniqueID = GetUniqueId();
	const float fSegmDuration = float( int( SConsts::AI_SEGMENT_DURATION ) );
	const float fCoeff = 1.0f * float( timeDiff ) / fSegmDuration;

	const CVec3 vSpeedDiff( GetSpeedNext() - vSpeed );
	const CVec3 vPosDiff( GetPosNext() - vPos );
	const CVec3 vNormaleDiff( GetNormalNext() - vNormale );

	const CVec3 vInterpolatedSpeed( GetSpeedNext() - fCoeff * vSpeedDiff );
	const CVec3 vInterpolatedNormale( GetNormalNext() - fCoeff * vNormaleDiff );
	const CVec3 vInterpolatedPos( GetPosNext() - fCoeff * vPosDiff );

#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_plane_path", 0 ) == 1 )
	{
		{
			CVec3 vSp( vInterpolatedSpeed );
			Normalize( &vSp );
			NDebugInfo::SArrowHead a1( vInterpolatedPos );
			NDebugInfo::SArrowHead a2( vInterpolatedPos + vSp * 500, 50, 5  );
			DebugInfoManager()->DrawLine( GetUniqueId() * 50000, a1, a2, NDebugInfo::RED );
		}
		{
			NDebugInfo::SArrowHead a1( vInterpolatedPos );
			NDebugInfo::SArrowHead a2( vInterpolatedPos + vInterpolatedNormale * 500, 50, 5  );
			DebugInfoManager()->DrawLine( GetUniqueId() * 50001, a1, a2, NDebugInfo::RED );
		}
	}
#endif


	MakeQuatBySpeedAndNormale( &pPlacement->rotation, vInterpolatedSpeed, vInterpolatedNormale );
	pPlacement->vPlacement = vInterpolatedPos;
}

const NTimer::STime CAviation::GetNextSecondPathSegmTime() const
{
	return 0;
}

void CAviation::Die( const bool fromExplosion, const float fDamage )
{
	theStatistics.UnitDead( this );

	pUnitInfoForGeneral->Die();

	// FEEDBACK ABOUT KILLING AVIATION
	
	//EFeedBack eFeed = 0;
	int nParam = 0;
	if ( GetPlayer() == theDipl.GetMyNumber() )
		nParam = 1;
	else if ( GetParty() == theDipl.GetMyParty() )
		nParam = 2;
	else if ( EDI_ENEMY == theDipl.GetDiplStatus( GetParty(), theDipl.GetMyParty() ) )
		nParam = 3;
	
/*	if ( nParam != 0 )
		updater.AddUpdate( EFB_AVIA_KILLED, (nParam << 16) | GetAviationType() );
	*/
	ChangePlayer( theDipl.GetNeutralPlayer() );
	
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_FLY_DEAD), this, false );
	
	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	updater.AddUpdate( 0, ACTION_NOTIFY_DEADPLANE, this, -1 );
	theAckManager.UnitDead( this );
	theAAFeedBacks.PlaneDeleted( this );
}

void CAviation::Disappear()
{
	theAAFeedBacks.PlaneDeleted( this );
	//SetPlanesFormation( 0, VNULL3 );
	RestoreSmoothPath();
	CMilitaryCar::Disappear();
}

const float CAviation::GetSightRadius() const
{
	return GetStats()->fSight * SConsts::TILE_SIZE;
}

void CAviation::GetRPGStats( struct SAINotifyRPGStats *pStats )
{
	CMilitaryCar::GetRPGStats( pStats );
	pStats->fFuel = fFuel;
}


