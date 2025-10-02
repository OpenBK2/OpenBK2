void PostLoad( bool bInEditor )
{
	for ( vector<SUIScreenEntry>::iterator it = screens.begin(); it != screens.end(); ++it )
		it->PostLoad( bInEditor );
}
