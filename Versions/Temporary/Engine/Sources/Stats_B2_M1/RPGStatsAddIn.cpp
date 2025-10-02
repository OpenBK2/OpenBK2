#include "StdAfx.h"

#include "misc/2darray.h"
#include "actioncommand.h"
#include "ActionsRemap.h"

#include <zconf.h>

PCConstructorInfo & ConstructorInfo()
{
	static CConstructorInfo *pInfo = 0;
	return pInfo;
};

template<int N> struct SUserActionsChecker;
template<> struct SUserActionsChecker<0> { };

static SUserActionsChecker<int(NDb::USER_ACTION_UNKNOWN)> check_USER_ACTION_UNKNOWN_is_null;

void NDb::SUnitActions::ReMapCommands( CUserCommands &ai, CUserActions &user )
{
	user.Clear();
	for ( int i = 0; i != 128; ++i ) 
	{
		if ( ai.GetData(i) != 0 )
		{
			const NDb::EUserAction nUserAction = GetActionByCommand( (EActionCommand)i );
			if ( nUserAction != NDb::USER_ACTION_UNKNOWN ) 
			{
				user.SetAction( nUserAction );
			}
//			if ( nUserAction == USER_ACTION_ENGINEER_PLACE_MINE_AP )
//				user.SetAction( USER_ACTION_ENGINEER_PLACE_MINE_AT );
		}
	}
}

const NDb::SComplexSoundDesc * ChooseAcknowledgement( const NDb::SAckSetRPGStats *pStats, const NDb::EUnitAckType type )
{
	if ( pStats == 0 || type < 0 || type >= pStats->types.size() ) 
		return 0;
	return pStats->types[type].pAck;
}

void NDb::SUnitActions::ToAIUnits( bool bInEditor )
{
	if ( bInEditor )
		return;
	// Add Special Abilities
	for ( int i = 0; i < specialAbilities.size(); ++i )
	{
		if ( specialAbilities[i] == 0 )
			continue;
		const int nData = GetCommandByAbility( specialAbilities[i]->eName );
		if ( -1 != nData )
		{
			availCommands.SetData( nData );
		}
	}

	AddValue( availCommands, ACTION_COMMAND_STOP );
	AddValue( availCommands, ACTION_COMMAND_GUARD );
	AddValue( availCommands, ACTION_COMMAND_WAIT );
	AddValue( availCommands, ACTION_COMMAND_MOVE_TO_GRID );
	if ( availCommands.GetData( ACTION_COMMAND_MOVE_TO ) != 0 )
		AddValue( availExposures, ACTION_COMMAND_FOLLOW );
	AddValue( availExposures, ACTION_COMMAND_THROW_GRENADE );

	AddValue( availCommands, ACTION_COMMAND_INSTALL );
	AddValue( availCommands, ACTION_COMMAND_UNINSTALL );

	if ( availCommands.GetData( ACTION_COMMAND_ENTRENCH_BEGIN ) )
		AddValue( availCommands, ACTION_COMMAND_ENTRENCH_END );

	if ( availCommands.GetData( ACTION_COMMAND_BUILD_FENCE_BEGIN ) )
		AddValue( availCommands, ACTION_COMMAND_BUILD_FENCE_END );

	if ( availCommands.GetData( ACTION_COMMAND_ENTRENCH_SELF ) )
		AddValue( availExposures, ACTION_COMMAND_ENTRENCH_SELF );

	// remap commands
	ReMapCommands( availCommands, availUserActions );
	ReMapCommands( availExposures, availUserExposures );

	availUserExposures.SetAction( USER_ACTION_ATTACK );
	availUserExposures.SetAction( USER_ACTION_UNKNOWN );
	//
	availUserActions.SetAction( USER_ACTION_UNKNOWN );
	availUserActions.SetAction( USER_ACTION_MOVE_TO_GRID );
}

const bool NDb::SUnitBaseRPGStats::HasCommand( const int nCmd ) const
{
	if ( -1 == nCmd ) 
		return false;
	NI_ASSERT( (nCmd >= 0 && nCmd < pActions->availCommands.GetSize()) || (nCmd >= 1000), StrFmt( "Wrong command ( %d )\n", nCmd ) );
	return nCmd >= 1000 ? true : ( nCmd < GetActions()->availCommands.GetSize() ? GetActions()->availCommands.GetData(nCmd) : false );
}
void NDb::SUnitBaseRPGStats::GetUserActions( CUserActions *pActions ) const
{ 
	this->pActions->availUserActions.GetActions( pActions ); 
}
const bool NDb::SUnitBaseRPGStats::HasUserAction( const int nAction ) const 
{ 
	return pActions->availUserActions.HasAction( nAction ); 
}
const CUserActions* NDb::SUnitBaseRPGStats::GetUserActions( bool bActionsBy ) const
{ 
	return bActionsBy ? &(pActions->availUserActions) : &(pActions->availUserExposures); 
}


