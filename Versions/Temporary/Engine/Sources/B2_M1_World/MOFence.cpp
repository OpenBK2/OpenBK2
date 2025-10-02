#include "StdAfx.h"
#include "MOFence.h"
#include "../Common_RTS_AI/AIClasses.h"

bool CMOFence::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	const float fNewHP = pUpdate->info.fHitPoints / GetStats()->fMaxHP;
	const NDb::SModel *pModel = GetModel( GetStatsLocal()->GetVisObjByFrameIndex( pUpdate->info.nFrameIndex ), eSeason );
	//ChooseVisObjForHP( fNewHP ), eSeason );

	NI_ASSERT( pModel != 0, StrFmt( "Wrong fence \"%s\" - no model", GetStats()->GetDBID().ToString().c_str() ) );
	if ( !pModel )
		return false;

	CVec3 vPos;
	CQuat qRot;
	GetPlacementFromUpdate( &vPos, &qRot, pUpdate );
	SetPlacement( vPos, qRot );

	nSceneID = Scene()->AddObject( nUniqueID, pModel, vPos, qRot, CVec3(1, 1, 1), OBJ_ANIM_MODE_DEFAULT, 0 );
	SetModel( pModel );
	return true;
}

bool CMOFence::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	if ( CMapObj::Create(nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor) )
	{
		RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, Scene()->GetAnimator(GetID()) );
		return true;
	}
	return false;
}

void CMOFence::GetStatus( SObjectStatus *pStatus ) const
{
	CMapObj::GetStatus( pStatus );
}

IClientUpdatableProcess* CMOFence::AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason )
{ 
	const float fNewHP = stats.fHitPoints / GetStats()->fMaxHP;
	CommonUpdateHP( fNewHP, stats, Scene(), eSeason );
	return 0;
}

void CMOFence::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		if ( GetHP() == 0.0f )
			pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
		pActions->SetAction( NDb::USER_ACTION_ATTACK );
	}
}

void CMOFence::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		if ( GetHP() == 0.0f )
			pActions->SetAction( NDb::USER_ACTION_ATTACK );
	}
}

NDb::EUserAction CMOFence::GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const
{
	if ( bAltMode && pActionsWith->HasAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN ) )
		return NDb::USER_ACTION_MOVE_LIKE_TERRAIN;
		
	NDb::EUserAction eAction = CMapObj::GetBestAutoAction( actionsBy, pActionsWith, bAltMode );

	if ( eAction == NDb::USER_ACTION_UNKNOWN )
	{
		if ( const NDb::SFenceRPGStats *pStats = GetStatsLocal() )
		{
			if ( actionsBy.HasAction( NDb::USER_ACTION_MOVE_TRACK ) )
			{
				if ( (pStats->nAIPassabilityClass & EAC_TRACK) == 0 )
					eAction = NDb::USER_ACTION_MOVE_TRACK;
			}
			if ( actionsBy.HasAction( NDb::USER_ACTION_MOVE_WHELL ) )
			{
				if ( (pStats->nAIPassabilityClass & EAC_WHELL) == 0 )
					eAction = NDb::USER_ACTION_MOVE_WHELL;
			}
			if ( actionsBy.HasAction( NDb::USER_ACTION_MOVE_HUMAN ) )
			{
				if ( (pStats->nAIPassabilityClass & EAC_HUMAN) == 0 )
					eAction = NDb::USER_ACTION_MOVE_HUMAN;
			}
		}
	}
		
	return eAction;
}

int CMOFence::operator&( IBinSaver &saver )
{
	saver.Add( 1, (CMapObj*)this );
	saver.Add( 2, &nSceneID );
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x11121C01, CMOFence );

