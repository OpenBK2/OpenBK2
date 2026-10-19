#include "stdafx.h"

#include "ActionCommand.h"
#include "ActionsRemap.h"
#include "Misc/2Darray.h"

#include <fmt/format.h>
#include <zconf.h>

PCConstructorInfo &ConstructorInfo()
{
	static CConstructorInfo *pInfo = 0;
	return pInfo;
}

template <int N>
struct SUserActionsChecker;

template <>
struct SUserActionsChecker<0>
{
};

static SUserActionsChecker<int( NDb::USER_ACTION_UNKNOWN )> check_USER_ACTION_UNKNOWN_is_null;

void NDb::SUnitActions::ReMapCommands( CUserCommands &ai, CUserActions &user )
{
	user.Clear();
	// The loop walks every bit of the command mask, so the index is held as an
	// int and named as a command once per step. This is the translation from
	// one mask's enumeration to the other's, and the only place that needs to.
	for ( int i = 0; i != 128; ++i )
	{
		const EActionCommand eCommand = EActionCommand( i );
		if ( ai.GetData( eCommand ) != 0 )
		{
			const NDb::EUserAction nUserAction = GetActionByCommand( eCommand );
			if ( nUserAction != NDb::USER_ACTION_UNKNOWN )
			{
				user.SetAction( nUserAction );
			}
			//			if ( nUserAction == USER_ACTION_ENGINEER_PLACE_MINE_AP )
			//				user.SetAction( USER_ACTION_ENGINEER_PLACE_MINE_AT );
		}
	}
}

const NDb::SComplexSoundDesc *ChooseAcknowledgement( const NDb::SAckSetRPGStats *pStats, const NDb::EUnitAckType type )
{
	if ( pStats == 0 || type < 0 || type >= pStats->types.size() )
	{
		return 0;
	}
	return pStats->types[type].pAck;
}

void NDb::SUnitActions::ToAIUnits( bool bInEditor )
{
	if ( bInEditor )
	{
		return;
	}
	// Add Special Abilities
	for ( int i = 0; i < specialAbilities.size(); ++i )
	{
		if ( specialAbilities[i] == 0 )
		{
			continue;
		}
		// GetCommandByAbility answers EActionCommand( -1 ) for an ability no
		// command stands for, which is why this keeps the sentinel comparison.
		const EActionCommand eData = GetCommandByAbility( specialAbilities[i]->eName );
		if ( EActionCommand( -1 ) != eData )
		{
			availCommands.SetData( eData );
		}
	}

	AddValue( availCommands, ACTION_COMMAND_STOP );
	AddValue( availCommands, ACTION_COMMAND_GUARD );
	AddValue( availCommands, ACTION_COMMAND_WAIT );
	AddValue( availCommands, ACTION_COMMAND_MOVE_TO_GRID );
	if ( availCommands.GetData( ACTION_COMMAND_MOVE_TO ) != 0 )
	{
		AddValue( availExposures, ACTION_COMMAND_FOLLOW );
	}
	AddValue( availExposures, ACTION_COMMAND_THROW_GRENADE );

	AddValue( availCommands, ACTION_COMMAND_INSTALL );
	AddValue( availCommands, ACTION_COMMAND_UNINSTALL );

	if ( availCommands.GetData( ACTION_COMMAND_ENTRENCH_BEGIN ) )
	{
		AddValue( availCommands, ACTION_COMMAND_ENTRENCH_END );
	}

	if ( availCommands.GetData( ACTION_COMMAND_BUILD_FENCE_BEGIN ) )
	{
		AddValue( availCommands, ACTION_COMMAND_BUILD_FENCE_END );
	}

	if ( availCommands.GetData( ACTION_COMMAND_ENTRENCH_SELF ) )
	{
		AddValue( availExposures, ACTION_COMMAND_ENTRENCH_SELF );
	}

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
	{
		return false;
	}
	NI_ASSERT( ( nCmd >= 0 && nCmd < pActions->availCommands.GetSize() ) || ( nCmd >= 1000 ),
	           fmt::format( "Wrong command ( {} )\n", nCmd ) );
	// nCmd arrives as an int because a value of 1000 or more is a script
	// command rather than an EActionCommand, and those are answered above
	// without consulting the mask. Below that, and inside the mask's own
	// bounds, it is a command and is named as one.
	return nCmd >= 1000 ? true
	                    : ( nCmd < GetActions()->availCommands.GetSize()
	                            ? GetActions()->availCommands.GetData( EActionCommand( nCmd ) )
	                            : false );
}

void NDb::SUnitBaseRPGStats::GetUserActions( CUserActions *pActions ) const
{
	this->pActions->availUserActions.GetActions( pActions );
}

const bool NDb::SUnitBaseRPGStats::HasUserAction( const NDb::EUserAction eAction ) const
{
	return pActions->availUserActions.HasAction( eAction );
}

const CUserActions *NDb::SUnitBaseRPGStats::GetUserActions( bool bActionsBy ) const
{
	return bActionsBy ? &( pActions->availUserActions ) : &( pActions->availUserExposures );
}
