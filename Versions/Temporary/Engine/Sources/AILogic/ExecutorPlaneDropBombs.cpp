
#include "stdafx.h"
#include "Aviation.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "Commands.h"
#include "ExecutorPlaneDropBombs.h"
#include "Diplomacy.h"

#include "AILogic_export.h"

extern CGroupLogic theGroupLogic;
extern CDiplomacy theDipl;



//CExecutorPlaneDropBombsObject


bool CExecutorPlaneDropBombsObject::IsExecutorValidInternal() const
{
	return IsValidObj( pUnit );
}

CExecutorPlaneDropBombsObject::CExecutorPlaneDropBombsObject( CAIUnit *_pUnit )
: CExecutorUnitBase( TID_PLANE_DROP_BOMBS_OBJECT, SConsts::BEH_UPDATE_DURATION/SConsts::AI_SEGMENT_DURATION, ABILITY_DROP_BOMBS ),
pUnit( _pUnit )
{
	if ( theDipl.IsAIPlayer( pUnit->GetPlayer() ) )
	{
		checked_cast_ptr<CAviation*>( pUnit )->SetBombAutocast( true );
		SetAutocast( true );
	}
}

int CExecutorPlaneDropBombsObject::Segment()
{
	bool bBombsOnBoard = false;
	const int nGun = pUnit->GetNGuns();
	for ( int i = 0; i < nGun; ++i )
	{
		CBasicGun *pGun = pUnit ->GetGun( i );
		if ( pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB && 
			   pGun->GetNAmmo() != 0 )
		{
			bBombsOnBoard = true;
			break;
		}
	}

	if ( bBombsOnBoard )
		return CExecutorUnitBase::Segment();
	else
	{
		Disable();
		CExecutorUnitBase::Segment();
		return -1;
	}
}

bool CExecutorPlaneDropBombsObject::NotifyEvent( const CExecutorEvent &event )
{
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbilityActivate *pEv( static_cast<const CExecutorEventSpecialAbilityActivate *>( &event ) );

	const EExecutorEventID eCurID = event.GetParam().eEventID;

	switch( eCurID )
	{
	case EID_ABILITY_DEACTIVATE_AUTOCAST:
		checked_cast_ptr<CAviation*>( pUnit )->SetBombAutocast( false );
		SetAutocast( false );
		return true;

	case EID_ABILITY_ACTIVATE_AUTOCAST:
		checked_cast_ptr<CAviation*>( pUnit )->SetBombAutocast( true );
		SetAutocast( true );
		return true;
	}

	if ( pEv->GetAbility() == GetAbility() )
		return CExecutorUnitBase::NotifyEvent( event );
	return false;
}

void CExecutorPlaneDropBombsObject::RegisterOnEvents( IExecutorContainer *pContainer )
{
	std::vector<EExecutorEventID> events;
	events.push_back( EID_ABILITY_ACTIVATE );
	events.push_back( EID_ABILITY_ACTIVATE_AUTOCAST );
	events.push_back( EID_ABILITY_DEACTIVATE_AUTOCAST );

	RegisterOnUnitEvents( pContainer, events, pUnit->GetUniqueId() );
}

// if 1.0 then ability must be switched off
float CExecutorPlaneDropBombsObject::OnAbilityActive()
{
	return 0.0f;
}

// if 1.0 then ability is ready to on
float CExecutorPlaneDropBombsObject::OnAbilityOff()
{
	return 1.0f;
}

CCommonUnit *CExecutorPlaneDropBombsObject::GetUnit() 
{ 
	return pUnit; 
}

void CExecutorPlaneDropBombsObject::SwitchingOffEnd()
{
}

void CExecutorPlaneDropBombsObject::SwitchingOffStart()
{
}

void CExecutorPlaneDropBombsObject::SwitchOnEnd()
{
}

void CExecutorPlaneDropBombsObject::SwitchOnStart( const class CAICommand *pCommand )
{
	// execute shturmovik patrol command with specific target
	if ( pCommand )
	{
		const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
		if ( cmd.nObjectID != 0 )
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_DROP_BOMBS_TO_TARGET, cmd.nObjectID, 0.0f ), pUnit, false );
		else
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_DROP_BOMBS_TO_POINT, cmd.vPos ), pUnit, false );
	}
	ForceRecharge();
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1115AAC1, CExecutorPlaneDropBombsObject );

