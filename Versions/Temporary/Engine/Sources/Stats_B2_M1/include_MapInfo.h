virtual void PostLoad( bool bInEditor )
{
	// check if players has diplomacy sides != -1
	// if they all do, alternate diplomacy
	bool bAllNonDefault = true;
	for ( int i = 0; i < players.size(); ++i )
	{
		if ( players[i].nDiplomacySide == -1 )
			bAllNonDefault = false;
	}
	if ( bAllNonDefault )
	{
		diplomacies.resize( players.size() );
		for ( int i = 0; i < players.size(); ++i )
			diplomacies[i] = players[i].nDiplomacySide;
	}
	// check all objects has correct player
	const int nNumPlayers = players.size();
	for ( int i = 0; i < objects.size(); ++i )
	{
		if ( objects[i].nPlayer >= nNumPlayers )
		{
			NI_ASSERT( objects[i].nPlayer < nNumPlayers, fmt::format("Object {} (placed at {{{:g}; {:g}}}) on the map \"{}\" has invalid player ({} of {}) - setting neutral!", i, objects[i].vPos.x, objects[i].vPos.y, GetDBID().ToString().c_str(), objects[i].nPlayer, nNumPlayers) );
			objects[i].nPlayer = nNumPlayers - 1;
		}
	}
	//
	for ( std::vector<SMapPlayerInfo>::iterator it = players.begin(); it != players.end(); ++it )
		it->PostLoad( bInEditor );
}
