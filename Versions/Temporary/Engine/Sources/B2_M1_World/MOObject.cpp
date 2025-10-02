#include "StdAfx.h"
#include "..\misc\2darray.h"
#include "../System/Commands.h"
#include "..\zlib\zconf.h"
#include "..\3dmotor\dbscene.h"
#include "MOObject.h"
#include "../SceneB2/AnimMutators.h"
#include "../SceneB2/WindController.h"
#include "../Sound/SoundScene.h"

static bool bDrawGrass = true;
static bool bAnimateTrees = true;

void RemoveAttachedSound( WORD *pwSound )
{
	Singleton<ISoundScene>()->RemoveSoundFromMap( *pwSound );
	*pwSound = 0;
}

bool CMOObject::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	const float fNewHP = pUpdate->info.fHitPoints / GetStats()->fMaxHP;

	bAnimateOnDeath = false;			// Default

	if ( GetStats()->pvisualObject == 0 ) 
		return false;
	const NDb::SModel *pModel = GetModel( ChooseVisObjForHP( fNewHP ), eSeason );
	if ( !pModel )
		return false;

	CVec3 vPos;
	CQuat qRot;
	GetPlacementFromUpdate( &vPos, &qRot, pUpdate );
	SetPlacement( vPos, qRot );

	ESceneObjAnimMode eAnimMode = OBJ_ANIM_MODE_DEFAULT;

	SetModel( pModel );

	if ( const NDb::SObjectBaseRPGStats *pStats = dynamic_cast<const NDb::SObjectBaseRPGStats*>( GetStats() ) ) 
	{
		if ( const NDb::SObjectRPGStats *pObjectStats = dynamic_cast<const NDb::SObjectRPGStats*>( pStats ) )
		{
			if ( pObjectStats->bHideForPerformance && !bDrawGrass )
				return true;
		}

		if ( pStats->bCanFall ) 
		{
			eAnimMode = OBJ_ANIM_MODE_FORCE_NON_ANIMATED;
			bAnimateOnDeath = true;
		}
		if ( pStats->eGameType == NDb::SGVOGT_FLORA && bAnimateTrees && Scene()->GetWindController()->GetWindIntensity() > FP_EPSILON )	
		{
			eAnimMode = OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC;
			bAnimateOnDeath = false;
		}
	}

	if ( GetStats()->eGameType == NDb::SGVOGT_FLORA )
	{
		eAnimMode = GetStats()->eGameType == NDb::SGVOGT_FLORA ? OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC : eAnimMode;
	}

	Scene()->AddObject( nUniqueID, pModel, vPos, qRot, CVec3(1, 1, 1), eAnimMode, 0 );
	
	return true;
}

bool CMOObject::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	if ( CMapObj::Create(nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor) )
	{
		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );
		RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, pAnimator );
		//
		if ( const NDb::SObjectRPGStats *pStats = dynamic_cast<const NDb::SObjectRPGStats*>(GetStats()) )
		{			
			if ( pStats->eGameType == NDb::SGVOGT_FLORA && bAnimateTrees && Scene()->GetWindController()->GetWindIntensity() > FP_EPSILON )	
			{						//It's a tree! And the "animated trees" are on. Add wind mutator.
				if ( pAnimator )
				{
					CPtr<ITreeWindMutator> pMutator = MakeObject<ITreeWindMutator>( ITreeWindMutator::typeID );
					pMutator->Setup( pAnimator, GetCenter(), pStats->specificJoints );
					pAnimator->SetSpecialMutator( pMutator );
				}
			}
			//
			if ( !bInEditor )
			{
				ISoundScene *pSoundScene = Singleton<ISoundScene>();
				const NDb::SComplexSoundDesc* pAmbientSoundDesc = pStats->GetAmbientSoundDesc( eSeason, eDayTime );
				if ( pAmbientSoundDesc )
					wAmbientSound = pSoundScene->AddSoundToMap( pAmbientSoundDesc, GetCenter() );
				if ( pStats->pCycledSound )
					wCycledSound = pSoundScene->AddSoundToMap( pStats->pCycledSound, GetCenter() );
				if ( pStats->cycledSoundTimed.size() > eDayTime && pStats->cycledSoundTimed[eDayTime] )
					wCycledSoundTimed = pSoundScene->AddSoundToMap( pStats->cycledSoundTimed[eDayTime], GetCenter() );
			}
		}
		//
		return true;
	}
	return false;
}

void CMOObject::GetStatus( SObjectStatus *pStatus ) const
{
	CMapObj::GetStatus( pStatus );
}

IClientUpdatableProcess* CMOObject::AIUpdateRPGStats( const SAINotifyRPGStats &stats, interface IClientAckManager *pAckManager, NDb::ESeason eSeason ) 
{ 
	const float fNewHP = stats.fHitPoints / GetStats()->fMaxHP;
	CommonUpdateHP( fNewHP, stats, Scene(), eSeason );
	if ( fNewHP <= 0 )
	{
		RemoveAttachedSound( &wAmbientSound );
		RemoveAttachedSound( &wCycledSound );
		RemoveAttachedSound( &wCycledSoundTimed );
	}
	return 0;
}

void CMOObject::AIUpdateFall( const SAITreeBrokenUpdate *pUpdate )
{
	Scene()->RemoveAllAttached( GetID(), ESSOT_LIGHT );
	RemoveAttachedSound( &wAmbientSound );
	RemoveAttachedSound( &wCycledSound );
	RemoveAttachedSound( &wCycledSoundTimed );
	if ( bAnimateOnDeath )			// Recreate an animated model in the object's place
	{
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );

		const NDb::SModel *pModel = GetModelDesc();
		Scene()->RemoveObject( GetID() );
		Scene()->AddObject( GetID(), pModel, vPos, qRot, CVec3(1, 1, 1), OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC, 0 );
	}

	NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );
	if ( pAnimator == 0 )
		return;
	//NI_ASSERT( pAnimator != 0, "Unable to kill tree - no animator availible" );
	Scene()->RemoveObjectPickability( GetID() );
	if ( pAnimator != 0 )
	{
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		float fAngle = PI / 2.0f - atan( pUpdate->fTg );
		if ( const NDb::SObjectRPGStats *pStats = dynamic_cast<const NDb::SObjectRPGStats *>(GetStats()) )
		{
			CPtr<ITreeFallingMutator> pMutator = MakeObject<ITreeFallingMutator>( ITreeFallingMutator::typeID );
			int nEffectID = OBJECT_ID_GENERATE;
			if ( pStats->pSeasonedFallEffect )
				nEffectID = PlayComplexSeasonedEffect( nEffectID, pStats->pSeasonedFallEffect, pUpdate->nUpdateTime, vPos, eSeason );
			else if ( pStats->pFallEffect != 0 )
				nEffectID = PlayComplexEffect( nEffectID, pStats->pFallEffect, pUpdate->nUpdateTime, vPos );
			pMutator->Setup( pAnimator, pUpdate->vDir, fAngle, qRot, pStats->specificJoints, nEffectID, vPos, pStats->nObjectHeight * 0.66f, pStats->fFallCycles, pStats->nFallDuration, pUpdate->nUpdateTime );
		}
	}
}

void CMOObject::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		pActions->SetAction( NDb::USER_ACTION_ATTACK );
	}
}

void CMOObject::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		if ( GetHP() == 0.0f )
			pActions->SetAction( NDb::USER_ACTION_ATTACK );
	}
}

int CMOObject::operator&( IBinSaver &saver )
{
	saver.Add( 1, (CMapObj*)this );
	saver.Add( 3, &bAnimateOnDeath );
	saver.Add( 4, &wAmbientSound );
	saver.Add( 5, &wCycledSound );
	saver.Add( 6, &wAmbientSound );
	saver.Add( 7, &wCycledSound );

	return 0;
}

START_REGISTER(CMOObject)
REGISTER_VAR_EX( "misc_draw_grass", NGlobal::VarBoolHandler, &bDrawGrass, true, STORAGE_USER )
REGISTER_VAR_EX( "misc_animations_trees", NGlobal::VarBoolHandler, &bAnimateTrees, true, STORAGE_USER )
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x100A7484, CMOObject );

