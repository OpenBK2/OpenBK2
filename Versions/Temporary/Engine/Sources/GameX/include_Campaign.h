void PostLoad( bool bInEditor )
{
	for ( std::vector<SLeaderExpLevel>::iterator it = leaderRanks.begin(); it != leaderRanks.end(); ++it )
		it->PostLoad( bInEditor );
	for ( std::vector<SUIScreenEntry>::iterator it = screens.begin(); it != screens.end(); ++it )
		it->PostLoad( bInEditor );
}
