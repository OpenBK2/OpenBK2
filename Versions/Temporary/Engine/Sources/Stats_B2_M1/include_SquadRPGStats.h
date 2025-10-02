virtual void ToAIUnits( bool bInEditor )
{
	eGameType = SGVOGT_SQUAD;
	SHPObjectRPGStats::ToAIUnits( bInEditor );
	// check changesByEvent and remove invalid entries
	for ( vector<SFormation>::iterator itFormation = formations.begin(); itFormation != formations.end(); ++itFormation )
	{
		for ( vector<int>::iterator itChange = itFormation->changesByEvent.begin(); itChange != itFormation->changesByEvent.end(); )
		{
			if ( *itChange >= formations.size() )
				itChange = itFormation->changesByEvent.erase( itChange );
			else
				++itChange;
		}
	}
	// check for orders and resize it to fit members size for each formation
	for ( vector<SSquadRPGStats::SFormation>::iterator it = formations.begin(); it != formations.end(); ++it ) 
	{
		const int nOldSize = it->order.size();
		for ( vector< CDBPtr<SInfantryRPGStats> >::iterator itMember = members.begin(); itMember != members.end(); )
		{
			if ( (*itMember) == 0 )
				itMember = members.erase( itMember );
			else
				++itMember;
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
		availUserActions.SetAction( USER_ACTION_FORMATION_0 + it->etype );
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
		availUserActions.Clear();
}
//
virtual const CUserActions* GetUserActions( bool bActionsBy ) const 
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
