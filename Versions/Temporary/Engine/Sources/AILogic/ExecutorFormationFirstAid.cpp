#include "stdafx.h"

#include "ExecutorFormationFirstAid.h"
#include "GroupLogic.h"
#include "Formation.h"
#include "Commands.h"
#include "FormationStates.h"

#include "AILogic_export.h"

extern CGroupLogic theGroupLogic;

CExecutorFormationFirstAid::CExecutorFormationFirstAid( CFormation *_pUnit )
: CExecutorUnitBase( TID_FIRST_AID, 10, NDb::ABILITY_FIRST_AID ), pFormation( _pUnit )
{
	SetAbilityDesc( pFormation->GetUnitAbilityDesc( NDb::ABILITY_FIRST_AID ) );
}

CCommonUnit *CExecutorFormationFirstAid::GetUnit()
{ 
	return pFormation; 
}

float CExecutorFormationFirstAid::OnAbilityActive()
{
	return 0.0f;
}

float CExecutorFormationFirstAid::OnAbilityOff()
{
	return 0.0f;
}

void CExecutorFormationFirstAid::SwitchingOffStart()
{
	//theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pFormation, false );
}

void CExecutorFormationFirstAid::SwitchOnEnd()
{
}

void CExecutorFormationFirstAid::SwitchOnStart( const class CAICommand *pCommand )
{
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_FIRST_AID, pCommand ? pCommand->ToUnitCmd().nObjectID : 0 ), pFormation, false );
}

bool CExecutorFormationFirstAid::ActivateDuringDisable()
{
	return false;
}

int CExecutorFormationFirstAid::Segment()
{
	// if autocast on - check if units around need healing.
	if ( IsAutocast() && pFormation->GetState()->IsRestState() &&
			 pFormation->IsFree() &&
		 CFormationFirstAidState::IsAnyNeedHealing( pFormation->GetParty(), pFormation->GetCenter() ) )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_FIRST_AID, float(EID_ABILITY_ACTIVATE) ), pFormation, false );
	}
	return CExecutorUnitBase::Segment();
}

bool CExecutorFormationFirstAid::NotifyEvent( const CExecutorEvent &event )
{
	if ( pFormation->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbilityActivate *pEv( static_cast<const CExecutorEventSpecialAbilityActivate *>( &event ) );
	if ( pEv->GetAbility() == GetAbility() )
	{
		return CExecutorUnitBase::NotifyEvent( event );
	}
	return false;
}

bool CExecutorFormationFirstAid::IsExecutorValid() const
{
	return IsValidObj( pFormation );
}

void CExecutorFormationFirstAid::RegisterOnEvents( IExecutorContainer *pContainer )
{
	std::vector<EExecutorEventID> events;
	events.push_back( EID_ABILITY_ACTIVATE );
	events.push_back( EID_ABILITY_DEACTIVATE );
	events.push_back( EID_ABILITY_ACTIVATE_AUTOCAST );
	events.push_back( EID_ABILITY_DEACTIVATE_AUTOCAST );

	RegisterOnUnitEvents( pContainer, events, pFormation->GetUniqueId() );
}

int CExecutorFormationFirstAid::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitBase*>( this ) );
	saver.Add( 2, &pFormation );  
	return 0;
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x11136340, CExecutorFormationFirstAid )

