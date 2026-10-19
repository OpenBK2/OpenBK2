// Which action puts a squad into a given formation. It has to be a mapping and
// not USER_ACTION_FORMATION_0 + etype, which is what this file and the two
// GetActions callers in B2_M1_World used to compute: the two enumerations do
// not run in step. EUserAction declares the five formation actions in the order
// 0, 3, 2, 1, 4 (UserActions.cll), while EFormationMoveType runs DEFAULT,
// MOVEMENT, DEFENSIVE, OFFENSIVE, SNEAK, so the arithmetic offered the
// offensive action to a squad that has the movement formation and the movement
// action to one that has the offensive formation. The names below are what the
// rest of the game agrees on: CWorldClient::ActionChangeFormation1 commands
// MOVEMENT, and the UI's movement_ARSetForcedAction record names
// USER_ACTION_FORMATION_1.
static EUserAction GetFormationUserAction( const SFormation::EFormationMoveType eFormation )
{
	switch ( eFormation )
	{
		case SFormation::DEFAULT:
			return USER_ACTION_FORMATION_0;
		case SFormation::MOVEMENT:
			return USER_ACTION_FORMATION_1;
		case SFormation::DEFENSIVE:
			return USER_ACTION_FORMATION_2;
		case SFormation::OFFENSIVE:
			return USER_ACTION_FORMATION_3;
		case SFormation::SNEAK:
			return USER_ACTION_FORMATION_4;
		default:
			return USER_ACTION_UNKNOWN;
	}
}

void ToAIUnits( bool bInEditor ) override
{
	eGameType = SGVOGT_SQUAD;
	SHPObjectRPGStats::ToAIUnits( bInEditor );
	// check changesByEvent and remove invalid entries
	for ( std::vector<SFormation>::iterator itFormation = formations.begin(); itFormation != formations.end();
	      ++itFormation )
	{
		for ( std::vector<int>::iterator itChange = itFormation->changesByEvent.begin();
		      itChange != itFormation->changesByEvent.end(); )
		{
			if ( *itChange >= formations.size() )
			{
				itChange = itFormation->changesByEvent.erase( itChange );
			}
			else
			{
				++itChange;
			}
		}
	}
	// check for orders and resize it to fit members size for each formation
	for ( std::vector<SSquadRPGStats::SFormation>::iterator it = formations.begin(); it != formations.end(); ++it )
	{
		const int nOldSize = it->order.size();
		for ( std::vector<CDBPtr<SInfantryRPGStats>>::iterator itMember = members.begin(); itMember != members.end(); )
		{
			if ( ( *itMember ) == 0 )
			{
				itMember = members.erase( itMember );
			}
			else
			{
				++itMember;
			}
		}
		it->order.resize( members.size() );
		if ( nOldSize < members.size() )
		{
			for ( int i = nOldSize; i < members.size(); ++i )
			{
				it->order[i].vPos.Set( 0, 0 );
				it->order[i].fDir = 0;
			}
		}
		//
		it->ToAIUnits( bInEditor );
		// set available user actions
		availUserActions.SetAction( GetFormationUserAction( it->etype ) );
	}
	// each squad can follow and can be followed by
	availUserActions.SetAction( USER_ACTION_FOLLOW );
	availUserExposures.SetAction( USER_ACTION_FOLLOW );
	//
	if ( members.size() == 1 )
	{
		availUserActions.RemoveAction( USER_ACTION_DISBAND_SQUAD );
		availUserActions.SetAction( USER_ACTION_FORM_SQUAD );
	}
	else if ( members.size() > 1 )
	{
		availUserActions.SetAction( USER_ACTION_DISBAND_SQUAD );
		availUserActions.SetAction( USER_ACTION_FORM_SQUAD );
	}
	else if ( members.empty() )
	{
		availUserActions.Clear();
	}
}

//
const CUserActions *GetUserActions( bool bActionsBy ) const override
{
	return bActionsBy ? &availUserActions : &availUserExposures;
}

//
const bool HasCommand( const int nCmd ) const
{
	//{CRAP - until availActions has the same structure, as in UnitBase
	return false;
	//}CRAP
}

//
