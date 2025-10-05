void PostLoad( bool bInEditor )
{
	// remove empty reinforcements from PlayerInfo
	for ( std::vector<CDBPtr<SReinforcement> >::iterator it = reinforcementTypes.begin(); it != reinforcementTypes.end(); )
	{
		if ( (*it) == 0 )
		{
			it = reinforcementTypes.erase( it );
			NI_ASSERT( false, "Empty reinforcement type in MapPlayerInfo!" )
		}
		else
			++it;
	}
}
