#include "stdafx.h"

#include "AIUnit.h"
#include "GlobalWarFog.h"
#include "Building.h"
#include "CommonStates.h"
#include "CommonUnit.h"
#include "Entrenchment.h"
#include "ExecutorContainer.h"
#include "FakeObjects.h"
#include "Formation.h"
#include "GroupLogic.h"
#include "Guns.h"
#include "NewUpdater.h"
#include "ObjectProfile.h"
#include "ShootEstimator.h"
#include "StaticObjectsIters.h"
#include "UnitsIterators2.h"
#include "Commands.h"
#include "Common_RTS_AI/CommonPathFinder.h"
#include "Common_RTS_AI/PathFinder.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "Stats_B2_M1/StatusUpdates.h"
#include "System/Commands.h"

#include "AILogic_export.h"

#include <cstdint>

extern CDiplomacy theDipl;
extern CEventUpdater updater;
extern CGlobalWarFog theWarFog;
extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;
extern CExecutorContainer theExecutorContainer;
extern bool g_bNewLock;

static int g_nDefaultCommandsLimit = -1;

BASIC_REGISTER_CLASS( AILOGIC, CCommonUnit );

const float CCommonUnit::GetMaxDamage( CCommonUnit *pTarget ) const
{
	float fMaxDamage = 0;
	for ( int i = 0; i < GetNGuns(); ++i )
	{
		if ( GetGun( i )->CanBreach( pTarget ) && GetGun( i )->GetDamage() > fMaxDamage )
			fMaxDamage = GetGun( i )->GetDamage();
	}

	return fMaxDamage;
}

CBasicGun* CCommonUnit::ChooseGunForStatObjWOTime( CStaticObject *pObj )
{
	NTimer::STime time;
	return ChooseGunForStatObj( pObj, &time );
}

void CCommonUnit::Init( const CVec3 &_vCenter, const uint16_t _wDirection, ICollisionsCollector *pCollisionsCollector )
{
	NI_VERIFY( GetUniqueID() != -1, "Unique id is not set", SetUniqueIdForUnits( ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID) );

	bSelectable = true;
	lastBehTime = 0;
	vBattlePos = CVec2( -1.0f, -1.0f );
	fDesirableSpeed = -1.0f;
	fMinFollowingSpeed = float(1e10);

	bCanBeFrozenByState = false;
	bCanBeFrozenByScan = false;
	nextFreezeScan = 0;
	fPrice = 0;

	vOldPlacement = _vCenter;
	qStart.FromAngleAxis( ToRadian( float( _wDirection ) / 65536.0f * 360.0f ), 0, 0, 1 );
	MakeOrientation( &qStart, DWORDToVec3( GetNormale( CVec2( vOldPlacement.x, vOldPlacement.y ) ) ) );

	qFinish = qStart;

	CBasePathUnit::Init( _vCenter, _wDirection, GetAIMap(), pCollisionsCollector, Singleton<CCommonPathFinder>() ); 
	CGroupUnit::Init();
	CQueueUnit::Init();
	nCommandLeft = g_nDefaultCommandsLimit;
}

const bool CCommonUnit::CanGoToPoint( const CVec2 &point ) const
{
	if ( IsInFormation() )
	{
		CFormation *pFormation = GetFormation();
		return
			!pFormation->IsInWaitingState() ||
			pFormation->GetState()->IsAttackingState() ||
			fabs2( pFormation->GetCenterPlain() - point ) < sqr( (float)SConsts::RADIUS_OF_FORMATION );
	}
	else if ( GetState()->GetName() == EUSN_REST )
		return fabs2( checked_cast<CCommonRestState*>( GetState() )->GetGuardPoint() - point ) < sqr( SConsts::GUARD_STATE_RADIUS );

	return true;
}

void CCommonUnit::SetSelectable( bool _bSelectable, bool bSendToWorld )
{
	bSelectable  = _bSelectable;
	if ( !bSendToWorld )
		return;
	updater.AddUpdate( 0, ACTION_NOTIFY_SELECTABLE_CHANGED, this, IsSelectable() );
}

const uint8_t CCommonUnit::GetParty() const
{
	return theDipl.GetNParty( GetPlayer() );
}

void CCommonUnit::Lock( const CBasicGun *pGun ) 
{ 
	pLockingGun = const_cast<CBasicGun*>(pGun);
}

void CCommonUnit::Unlock( const CBasicGun *pGun ) 
{ 
	if ( pLockingGun != 0 && !IsValidObj( pLockingGun ) ) 
		pLockingGun = 0; 
	else if ( pGun == pLockingGun ) 
		pLockingGun = 0; 
}

bool CCommonUnit::IsLocked( const CBasicGun *pGun ) const 
{ 
	return IsValidObj( pLockingGun ) && pLockingGun != pGun; 
}

bool CCommonUnit::CanShootToUnitWoMove( class CAIUnit *_pTarget )
{
	// можно ли пробить армор и вообще попасть
	bool bCan = false;
	int nGun = GetNGuns();
	for ( int i=0; i<nGun; ++i )
	{
		CBasicGun *pGun = GetGun( i );
		if ( pGun->CanShootToUnitWOMove( _pTarget ) )
		{
			bCan = true;
			break;
		}
	}
	return bCan;
}

void CCommonUnit::SetDesirableSpeed( const float _fDesirableSpeed )
{
	fDesirableSpeed = _fDesirableSpeed;
}

void CCommonUnit::UnsetDesirableSpeed()
{
	fDesirableSpeed = -1.0f;
}

float CCommonUnit::GetDesirableSpeed() const
{
	return fDesirableSpeed;
}

void CCommonUnit::AdjustWithDesirableSpeed( float *pfMaxSpeed ) const
{
	if ( fDesirableSpeed != -1.0f && !IsIdle() )
		*pfMaxSpeed = (std::min)( fDesirableSpeed, *pfMaxSpeed );
}

void CCommonUnit::SetFollowState( CCommonUnit *_pFollowedUnit )
{
	SendAcknowledgement( ACK_POSITIVE );
	pFollowedUnit = _pFollowedUnit;

	CVec2 vFromHeader = GetCenterPlain() - pFollowedUnit->GetCenterPlain();
	Normalize( &vFromHeader );
	vFollowShift = vFromHeader * SConsts::FOLLOW_STOP_RADIUS * 0.75f;
}

void CCommonUnit::UnsetFollowState()
{
	pFollowedUnit = 0;
}

bool CCommonUnit::IsInFollowState()
{
	if ( !IsValidObj( pFollowedUnit ) )
		pFollowedUnit = 0;

	return pFollowedUnit != 0;
}

class CCommonUnit* CCommonUnit::GetFollowedUnit() const
{
	return pFollowedUnit;
}

void CCommonUnit::FollowingByYou( CCommonUnit *pFollowingUnit )
{
	float fDist = fabs( pFollowingUnit->GetCenter() - GetCenter() );
	float fDesirableSpeed = GetSpeedForFollowing();

	if ( fDist > SConsts::FOLLOW_WAIT_RADIUS )
		fDesirableSpeed = 0.0f;
	else if ( fDist > SConsts::FOLLOW_STOP_RADIUS )
		fDesirableSpeed = (std::min)( fDesirableSpeed, pFollowingUnit->GetSpeedForFollowing() );

	fMinFollowingSpeed = (std::min)( fDesirableSpeed, fMinFollowingSpeed );
}

void CCommonUnit::Segment()
{
	if ( !GetState()->IsAttackingState() )//|| !theWarFog.IsTileVisible( AICellsTiles::GetTile( GetState()->GetPurposePoint() ), GetParty() ) )
	{
		if ( fDesirableSpeed == -1.0f )
			SetDesirableSpeed( fMinFollowingSpeed );
		else
			SetDesirableSpeed( (std::min)( fDesirableSpeed, fMinFollowingSpeed ) );
	}

	fMinFollowingSpeed = 1e10;
	CQueueUnit::Segment();
}

void CCommonUnit::FreezeSegment()
{
	if ( bCanBeFrozenByState && nextFreezeScan < curTime )
	{
		nextFreezeScan = curTime + NRandom::Random( 1500, 3000 ); RecordRandomCall();

		if ( !IsIdle() )
			bCanBeFrozenByScan = false;
		else
		{
			CUnitsIter<0,3> iter( GetParty(), EDI_ENEMY, GetCenterPlain(), GetTargetScanRadius() );
			bCanBeFrozenByScan = iter.IsFinished();
		}

		if ( !CanBeFrozen() )
			theGroupLogic.RegisterSegments( this, false, false );
	}
}

bool CCommonUnit::CanBeFrozen() const
{
	return bCanBeFrozenByScan && bCanBeFrozenByState;
}

void CCommonUnit::SetShootEstimator( IShootEstimator *_pShootEstimator )
{
	pShootEstimator = _pShootEstimator;
}

void CCommonUnit::ResetShootEstimator( CAIUnit *pCurEnemy, const bool bDamageUpdated, const uint32_t wForbidden )
{
	pShootEstimator->Reset( pCurEnemy, bDamageUpdated, wForbidden );
}

void CCommonUnit::AddUnitToShootEstimator( CAIUnit *pUnit )
{
	pShootEstimator->AddUnit( pUnit );
}

CAIUnit* CCommonUnit::GetBestShootEstimatedUnit() const
{
	return pShootEstimator->GetBestUnit();
}

CBasicGun* CCommonUnit::GetBestShootEstimatedGun() const
{
	return pShootEstimator->GetBestGun();
}

const int CCommonUnit::GetNumOfBestShootEstimatedGun() const
{
	return pShootEstimator->GetNumberOfBestGun();
}

void CCommonUnit::SetTruck( CAIUnit *pUnit )
{
	pTruck = pUnit;
}

CAIUnit* CCommonUnit::GetTruck() const
{
	return pTruck;
}

void CCommonUnit::FreezeByState( const bool bFreeze )
{
	if ( !IsInFollowState() )
	{
		bCanBeFrozenByState = bFreeze;

		if ( !bFreeze )
			theGroupLogic.RegisterSegments( this, false, false );
	}
}

bool CCommonUnit::IsFrozenByState() const
{
	return bCanBeFrozenByState;
}

const float CCommonUnit::GetSpeedForFollowing()
{
	return GetMaxSpeedHere( GetCenterPlain(), true );
}

void CCommonUnit::CheckForDestroyedObjects( const CVec2 &center ) const
{
	const SUnitProfile unitProfile = GetUnitProfile();
	const EAIClasses aiClass = GetAIPassabilityClass();
	const CVec2 vCenter = unitProfile.GetCenter();

	for ( CStObjCircleIter<false> stObjsIter( unitProfile.GetCenter(), unitProfile.GetRadius() + 200 ); !stObjsIter.IsFinished(); stObjsIter.Iterate() )
	{
		CExistingObject *pObj = *stObjsIter;
		const EStaticObjType eType = pObj->GetObjectType();
		if ( pObj->IsAlive() && eType != ESOT_BRIDGE_SPAN && eType != ESOT_MINE && eType != ESOT_ENTR_PART && eType != ESOT_FLAG && eType != ESOT_CANT_CRUSH )
		{
			if ( pObj->CanUnitGoThrough( aiClass ) )
			{
				const CVec3 &vObjPos = pObj->GetCenter();
				bool bIntersected = false;
				if ( g_bNewLock == 0 )
					bIntersected = unitProfile.IsPointInside( CVec2( vObjPos.x, vObjPos.y ) );
				else
				{
					CObjectProfile *pProfile = pObj->GetPassProfile();
					if ( pProfile )
						bIntersected = pProfile->IsWeakIntersected( GetUnitRect() );
				}

				if ( bIntersected )
				{
					if ( pObj->CanFall() )
					{
						CVec2 vFallDir = CVec2(pObj->GetCenter().x,pObj->GetCenter().y) - vCenter;
						vFallDir /= fabs( vFallDir );
						pObj->AnimateFalling( vFallDir );
						pObj->SetTrampled();
						pObj->Delete();
						updater.AddUpdate( 0, ACTION_NOTIFY_SILENT_DEATH, pObj, -1 );
					}
					else if ( pObj->GetObjectType() == ESOT_FAKE_CORPSE )
					{
						pObj->SetTrampled();
						pObj->Delete();
					}
					else if ( !pObj->IsAlive() )
					{
						updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pObj, -1 );
						pObj->SetTrampled();
						pObj->Delete();
					}
					else
					{
						updater.AddUpdate( 0, ACTION_NOTIFY_SILENT_DEATH, pObj, -1 );

						const SStaticObjectRPGStats *pStats = dynamic_cast<const SStaticObjectRPGStats*>(pObj->GetStats());
						if ( !pStats || !pStats->bLeaveCorpse )
							pObj->Delete();
						else
							CFakeCorpseStaticObject::CreateFakeCorpseStaticObject( pObj );
					}
					pObj->SetHitPoints( 0 );
				}
			}
		}
	}
}

const bool CCommonUnit::IterateUnits( const CVec2 &vCenter, const float fRadius, const bool bOnlyMech, const SIterateUnitsCallback &callback ) const
{
	for ( CUnitsIter<0,0> iter( 0, ANY_PARTY, vCenter, fRadius, bOnlyMech ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pAIUnit = *iter;
		if ( !pAIUnit || !pAIUnit->IsAlive() )
			continue;

		CBasePathUnit *pUnit = pAIUnit;
		if ( pUnit->GetUniqueID() != GetUniqueID() )
		{
			if ( !callback.Iterate( pUnit ) )
				return false;
		}
	}
	return true;
}

IStaticPath* CCommonUnit::GetPathToBuilding( CBuilding *pBuilding, int *pnEntrance )
{
	float fMinLen = 1e10;
	IStaticPath *pBestPath = 0;
	*pnEntrance = 0;

	for ( int i = 0; i < pBuilding->GetNEntrancePoints(); ++i )
	{
		const CVec2 vEntr = pBuilding->GetEntrancePoint( i );

		if ( !GetTerrain()->IsLocked( GetAIMap()->GetTile( vEntr ), EAC_HUMAN ) )
		{
			// здесь - не CPtr!!! Нужно, чтобы при выходе из функции он не удалялся
			IStaticPath *pPath = CreateStaticPathToPoint( vEntr, VNULL2, this, false, GetAIMap() );
			// чтобы удалять путь
			CPtr<IStaticPath> pGarbage;

			if ( pPath && pBuilding->IsGoodPointForRunIn( pPath->GetFinishPoint(), i ) )
			{
				const float fDist = fabs2( pPath->GetFinishPoint() - GetCenterPlain() );
				if ( fDist < fMinLen )
				{
					fMinLen = fDist;
					pGarbage = pBestPath;
					pBestPath = pPath;
					*pnEntrance = i;
				}
				else
					pGarbage = pPath;
			}
			else
				pGarbage = pPath;
		}
	}

	return pBestPath;
}

IStaticPath* CCommonUnit::GetPathToEntrenchment( CEntrenchment *pEntrenchment )
{
	SRect rect;
	pEntrenchment->GetBoundRect( &rect );
	const CSegment rectSegment( rect.center - rect.dir * rect.lengthBack, rect.center + rect.dir * rect.lengthAhead );

	CVec2 finishPoint;
	rectSegment.GetClosestPoint( GetCenterPlain(), &finishPoint );

	CVec2 toRectCenter( rect.center - finishPoint );
	Normalize( &toRectCenter );
	// немного сдвинуть к центру окопа
	finishPoint += toRectCenter * SConsts::TILE_SIZE;

	return CreateStaticPathToPoint( finishPoint, VNULL2, this, false, GetAIMap() );
}

void CCommonUnit::UpdatePlacement( const CVec3 &_vOldPosition, const uint16_t _wOldDirection, const bool bNeedUpdate )
{
	qFinish.FromAngleAxis( ToRadian( float( GetFrontDirection() ) / 65536.0f * 360.0f ), 0, 0, 1 );
	MakeOrientation( &qFinish, DWORDToVec3( GetNormale( GetCenterPlain() ) ) );
	if ( bNeedUpdate )
	{
		vOldPlacement = _vOldPosition;

		qStart.FromAngleAxis( ToRadian( float( _wOldDirection ) / 65536.0f * 360.0f ), 0, 0, 1 );
		MakeOrientation( &qStart, DWORDToVec3( GetNormale( CVec2( vOldPlacement.x, vOldPlacement.y ) ) ) );

		if ( bNeedUpdate )
			updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
	}
	else
	{
		vOldPlacement = GetCenter();
		qStart = qFinish;
	}
}

const uint32_t CCommonUnit::GetNormale( const CVec2 &vCenter ) const
{
	return GetHeights()->GetNormal( vCenter.x, vCenter.y );
}

void CCommonUnit::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{ 
	NI_ASSERT( timeDiff <= SConsts::AI_SEGMENT_DURATION, StrFmt( "wrong segment time %i", timeDiff ) );
	const float t = Clamp( (float)timeDiff/(float)(SConsts::AI_SEGMENT_DURATION), 0.0f, 1.0f );
	//DebugTrace( "CCommonUnit::GetPlacement t = %2.3f", t );

	const CVec3 vPosition( vOldPlacement * t + GetCenter() * ( 1.0f - t ) );
	pPlacement->center.x = vPosition.x;
	pPlacement->center.y = vPosition.y;
	pPlacement->z = vPosition.z;

	pPlacement->dir = GetFrontDirection();
	pPlacement->dwNormal = GetNormale( pPlacement->center );

	pPlacement->bNewFormat = true;
	pPlacement->rotation.Interpolate( qStart, qFinish, 1.0f - t );
	pPlacement->vPlacement = vPosition;
	
	pPlacement->fSpeed = GetSpeed();
	pPlacement->fWaterCoeff = 0.0f;
	pPlacement->nObjUniqueID = GetUniqueID();

	const SVector tile = AICellsTiles::GetTile( pPlacement->center );
	pPlacement->cSoil = GetTerrain()->GetSoilType( GetCenterTile() );
}

void CCommonUnit::UpdateTile()
{
	WarFogChanged();
}

void CCommonUnit::SetBehaviourMoving( const SBehaviour::EMoving _moving )
{
	if ( !IsInfantry() || IsFormation() )
	{
		if ( _moving == SBehaviour::EMHoldPos && beh.moving != SBehaviour::EMHoldPos )
			updater.AddUpdate( CreateStatusUpdate( EUS_STAND_GROUND, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
		else if ( _moving != SBehaviour::EMHoldPos && beh.moving == SBehaviour::EMHoldPos )
			updater.AddUpdate( CreateStatusUpdate( EUS_STAND_GROUND, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
	}

	beh.moving = _moving;
}

void CCommonUnit::ProduceEventByAction( const NDb::EUnitSpecialAbility eAbility, const NDb::ESpecialAbilityParam action, class CAICommand * pCommand )
{
	const NDb::SUnitSpecialAblityDesc *pDesc = GetUnitAbilityDesc( eAbility );
	if ( !pDesc )
		return;

	switch( action )
	{
	case PARAM_ABILITY_ON: // switch ability on ( single use )
		{
			SExecutorEventParam param( EID_ABILITY_ACTIVATE, 0, GetUniqueId() );
			CExecutorEventSpecialAbilityActivate event( param, eAbility, pCommand, pDesc );
			theExecutorContainer.RaiseEvent( event );
		}	
		break;
	case PARAM_ABILITY_OFF: // switch ability off ( single use )
		{
			SExecutorEventParam param( EID_ABILITY_DEACTIVATE, 0, GetUniqueId() );
			CExecutorEventSpecialAbilityActivate event( param, eAbility, 0, pDesc );
			theExecutorContainer.RaiseEvent( event );
		}
		break;
	case PARAM_ABILITY_AUTOCAST_ON: // switch on autocast mode
		{
			SExecutorEventParam param( EID_ABILITY_ACTIVATE_AUTOCAST, 0, GetUniqueId() );
			CExecutorEventSpecialAbilityActivate event( param, eAbility, 0, pDesc );
			theExecutorContainer.RaiseEvent( event );
		}
		break;
	case PARAM_ABILITY_AUTOCAST_OFF: // switch autocast mode off
		{
			SExecutorEventParam param( EID_ABILITY_DEACTIVATE_AUTOCAST, 0, GetUniqueId() );
			CExecutorEventSpecialAbilityActivate event( param, eAbility, 0, pDesc );
			theExecutorContainer.RaiseEvent( event );
		}
		break;
	}
}

const bool CCommonUnit::TryExecuteCommand( CAICommand *pCommand, const bool bPlaceInQueue, bool bOnlyThisUnitCommand )
{
	if ( theDipl.IsNetGame() && !pCommand->IsFromAI() )
	{
		if ( nCommandLeft == 0 )
		{
			CQueueUnit::DelCmdQueue( GetUniqueIdQU() );
			std::vector<int> availEnemies;
			for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
			{
				if ( theDipl.IsPlayerExist( i ) && theDipl.GetDiplStatus( GetPlayer(), i ) == EDI_ENEMY )
					availEnemies.push_back( i );
			}
			if ( !availEnemies.empty() )
			{
				nCommandLeft = g_nDefaultCommandsLimit;
				ChangePlayer( availEnemies[NRandom::Random( 0, availEnemies.size()-1 )] ); RecordRandomCall();
			}
			return false;
		}
		if ( nCommandLeft > 0 )
			--nCommandLeft;
	}
	UnitCommand( pCommand, bPlaceInQueue, bOnlyThisUnitCommand );
	return true;
}

START_REGISTER( CommandsLimit )
REGISTER_VAR_EX( "commands_limit", NGlobal::VarIntHandler, &g_nDefaultCommandsLimit, -1, STORAGE_NONE );
FINISH_REGISTER


