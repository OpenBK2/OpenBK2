#include "stdafx.h"

#include "MOBridge.h"
#include "Misc/Win32Random.h"
#include "Stats_B2_M1/DBAnimB2.h"
#include "3Dmotor/GAnimation.hpp"

#include <fmt/format.h>

bool CMOBridge::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	const float fNewHP = pUpdate->info.fHitPoints / GetStats()->fMaxHP;
	pStats = checked_cast<const NDb::SBridgeRPGStats*>( GetStats() );
	nFrameIndex = pUpdate->info.nFrameIndex;
	nRandomSpan = -1;
	int nDamagedState = 0;
	const NDb::SVisObj *pVO = GetVisObjForHP( fNewHP, &nDamagedState );
	const NDb::SModel *pModel = GetModel( pVO, eSeason );

	NI_ASSERT( pModel != 0, fmt::format( "Wrong vis for bridge \"{}\"", GetStats()->GetDBID().ToString() ) );
	if ( !pModel )
		return false;

	CVec3 vPos;
	CQuat qRot;
	GetPlacementFromUpdate( &vPos, &qRot, pUpdate );
	SetPlacement( vPos, qRot );

	bool bAnimated = pVO->bForceAnimated;

	// Check if there are any non-death animations
	for ( int i = 0; i < pModel->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr< const NDb::SAnimB2 * >( pModel->animations[ i ] );
		if ( pAnim && pAnim->eType != NDb::ANIMATION_DEATH )
		{
			bAnimated = true;
			break;
		}
	}

	Scene()->AddObject( nUniqueID, pModel, vPos, qRot, CVec3(1, 1, 1), 
		                  bAnimated ? OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC : OBJ_ANIM_MODE_FORCE_NON_ANIMATED, 0, false );
	SetModel( pModel );

  return true;
}

bool CMOBridge::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	if ( CMapObj::Create(nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor) )
	{
		bDestroyed = GetHP() <= 0.0f;
		if ( bDestroyed )
			PlayDeathAnimation( 0, true );
		else
			RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, Scene()->GetAnimator( GetID() ) );
		return true;
	}
	return false;
}

void CMOBridge::GetStatus( SObjectStatus *pStatus ) const
{
	CMapObj::GetStatus( pStatus );
}

const NDb::SBridgeRPGStats::SElementRPGStats& CMOBridge::GetElement() const
{
	return nFrameIndex == 0 ? pStats->end : pStats->center;
}

int CMOBridge::GetDamagedState( float fHP ) const 
{
	int nResult = -1;
	float fCurrHP = 1.0f;
	const NDb::SBridgeRPGStats::SElementRPGStats &element = GetElement();
	for ( int i = 0; i < element.damageStates.size(); ++i )
	{
		const NDb::SBridgeRPGStats::SElementRPGStats::SBridgeDamageState &state = element.damageStates[i];
		if ( state.fDamageHP >= fHP && state.fDamageHP <= fCurrHP )
		{
			fCurrHP = state.fDamageHP;
			nResult = i;
		}
	}
	return nResult;
}

const NDb::SVisObj* CMOBridge::GetVisObjForHP( float fHP, int *pnDamagedState )
{
	*pnDamagedState = GetDamagedState( fHP );
	const std::vector< CDBPtr<NDb::SVisObj> > &objects = *pnDamagedState == -1 ? GetElement().visualObjects : GetElement().damageStates[*pnDamagedState].visObjects;
	if ( nRandomSpan == -1 )
	{
		if ( objects.size() == 1 )
			nRandomSpan = 0;
		else
			nRandomSpan = NWin32Random::Random( objects.size() - 1 );
	}
	return objects[nRandomSpan];
}

void CMOBridge::PlayDeathAnimation( const NTimer::STime timeStart, const bool bInstant )
{
	CVec3 vPos, vScale;
	CQuat qRot;
	GetPlacement( &vPos, &qRot, &vScale );
	Scene()->RemoveObject( GetID() );
	Scene()->AddObject( GetID(), GetModelDesc(), vPos, qRot, vScale, OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC, 0 );

	int nDamagedState = 0;
	const NDb::SVisObj *pVO = GetVisObjForHP( 0, &nDamagedState );
	NI_ASSERT( pVO, fmt::format( "No VisObject for destroyed Bridge state (object {})", GetID() ) );
	if ( !pVO )
		return;
	const NDb::SModel *pModel = GetModel( pVO, eSeason );
	NI_ASSERT( pModel, fmt::format( "No Model for destroyed Bridge state (object {})", GetID() ) );
	if ( !pModel )
		return;
	CDBPtr<NDb::SSkeleton> pSkeleton = pModel->pSkeleton;
	NI_ASSERT( pSkeleton, fmt::format( "No Skeleton for destroyed Bridge state (object {})", GetID() ) );
	if ( !pSkeleton )
		return;
	std::vector< const NDb::SAnimB2* > deathAnims;
	deathAnims.reserve( pSkeleton->animations.size() );
	for ( int i = 0; i < pSkeleton->animations.size(); ++i )
	{
		CDBPtr<NDb::SAnimB2> pAnim = checked_cast<const NDb::SAnimB2*>( pSkeleton->animations[i].GetPtr() );
		if ( pAnim->eType == NDb::ANIMATION_DEATH )
			deathAnims.push_back( pAnim );
	}
	if ( deathAnims.size() > 0 )	
	{
		int nRandomAnim = deathAnims.size() == 1 ? 0 : NWin32Random::Random( deathAnims.size() );
		if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() ) )
			AddAnimation( deathAnims[nRandomAnim], timeStart, pAnimator, false, bInstant ? FP_MAX_VALUE : 1.0f );
	}
}

IClientUpdatableProcess* CMOBridge::AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason )
{
	const float fNewHP = stats.fHitPoints / GetStats()->fMaxHP;
	const bool bAlive = fNewHP > 0;
	const float fOldHP = GetHP();
	CommonUpdateHP( fNewHP, stats, Scene(), eSeason );
	const int nStage = GetDamagedState( fNewHP );
	if ( fOldHP < fNewHP ) // repair process
	{
		if ( fNewHP == 1.0f )
		{
			bDestroyed = false;
			int nDamagedState;
			const NDb::SVisObj *pVO = GetVisObjForHP( fNewHP, &nDamagedState );
			const NDb::SModel *pModel = GetModel( pVO, eSeason );
			ChangeModelToDamaged( nDamagedState, pModel, eSeason );
			if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() ) )
				pAnimator->ClearAllAnimations();
		}
		return 0;
	}
	if ( nStage != GetDamagedState( fOldHP ) )
	{
		const NDb::SBridgeRPGStats::SElementRPGStats &element = GetElement();
		const NDb::SBridgeRPGStats::SElementRPGStats::SBridgeDamageState &state = element.damageStates[nStage];
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		if ( state.pSmokeEffect != 0 )
		{
			for ( int i = 0; i < pStats->smokePoints.size(); ++i )
			{
				const NDb::SBridgeRPGStats::SBridgeFirePoint &point = pStats->smokePoints[i];
				CVec3 vEffectPos;
				const CVec3 vPointPos( point.vPos.y - element.vOrigin.x, point.vPos.x - element.vOrigin.y, 0 );
				qRot.Rotate( &vEffectPos, vPointPos ); 
				vEffectPos += vPos;
				const float fAngle = point.fDirection * FP_2PI / 65535.0;
				CQuat qEffectRot( fAngle, CVec3( 0, 0, 1.0f ) );
				qEffectRot = qRot * qEffectRot * CQuat( -FP_PI * 0.5f, CVec3( 1.0f, 0, 0 ) );
				SHMatrix mEffectPos;
				AI2Vis( &vEffectPos );
				MakeMatrix( &mEffectPos, vEffectPos, qEffectRot );
				PlayComplexEffect( OBJECT_ID_FORGET, state.pSmokeEffect, stats.time, mEffectPos );
			}
		}
		nRandomSpan = -1;
		int nDamagedState = 0;
		const NDb::SVisObj *pVO = GetVisObjForHP( fNewHP, &nDamagedState );
		ChangeModelToDamaged( nDamagedState, GetModel( pVO, eSeason ), eSeason );
	}
	if ( !bAlive )
	{
		bDestroyed = true;
		// Substitute for an animated model
		PlayDeathAnimation( stats.time + NWin32Random::Random( 2000 ), false );
	}
	return 0;
}

void CMOBridge::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		if ( bDestroyed )
			pActions->SetAction( NDb::USER_ACTION_ENGINEER_REPAIR );
		else
		{
			pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
			pActions->SetAction( NDb::USER_ACTION_ATTACK );
		}
	}
}

void CMOBridge::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		if ( bDestroyed )
		{
			pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
			pActions->SetAction( NDb::USER_ACTION_ATTACK );
		}
	}
}

NDb::EUserAction CMOBridge::GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const
{
	if ( bAltMode && pActionsWith->HasAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN ) )
		return NDb::USER_ACTION_MOVE_LIKE_TERRAIN;
		
	return CMapObj::GetBestAutoAction( actionsBy, pActionsWith, bAltMode );
}

bool CMOBridge::IsPlaceMapCommandAck( NDb::EUserAction eUserAction ) const
{
	return eUserAction == NDb::USER_ACTION_MOVE || eUserAction == NDb::USER_ACTION_MOVE_LIKE_TERRAIN;
}

void CMOBridge::FinalizeDeath( NDb::ESeason eSeason )
{
	nRandomSpan = -1;
	int nDamagedState = 0;
	const NDb::SVisObj *pDeadVO = GetVisObjForHP( 0, &nDamagedState );
	ChangeModelToDamaged( nDamagedState, GetModel( pDeadVO, eSeason ), eSeason );
}

int CMOBridge::operator&( IBinSaver &saver )
{
	saver.Add( 1, checked_cast<CMapObj*>(this) );
	saver.Add( 2, &pStats );
	saver.Add( 3, &nFrameIndex );
	saver.Add( 4, &nRandomSpan );
	saver.Add( 5, &bDestroyed );
	return 0;
}

REGISTER_SAVELOAD_CLASS( B2_M1_WORLD, 0x100A7480, CMOBridge );

