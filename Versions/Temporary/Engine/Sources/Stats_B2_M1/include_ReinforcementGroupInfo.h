int GetGroupById( const int scriptID ) const
{
	for ( vector< SReinforcementGroupInfoEntry >::const_iterator it = infos.begin(); it != infos.end(); ++it )
	{
		int i = 0; 
		while( i < it->groupsVector.data.size() && it->groupsVector.data[i] != scriptID )
			++i;
		if ( i < it->groupsVector.data.size() )
			return it->nGroupID;
	}
	return -1;

/*	for( hash_map< int, vector<int> >::const_iterator it = groups.begin(); it != groups.end(); ++it )
	{
		int i = 0;
		while ( i < it->second.size() && it->second[i] != scriptID )
			++i;

		if ( i < it->second.size() )
			return it->first;
	}

	return -1;
	*/
}
