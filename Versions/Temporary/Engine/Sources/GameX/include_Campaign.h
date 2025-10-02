void PostLoad( bool bInEditor )
{
	for ( vector<SLeaderExpLevel>::iterator it = leaderRanks.begin(); it != leaderRanks.end(); ++it )
		it->PostLoad( bInEditor );
	for ( vector<SUIScreenEntry>::iterator it = screens.begin(); it != screens.end(); ++it )
		it->PostLoad( bInEditor );
}
