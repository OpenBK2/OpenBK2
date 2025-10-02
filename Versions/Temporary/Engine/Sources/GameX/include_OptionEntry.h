void PostLoad( bool bInEditor )
{
	for ( vector<SOptionEntryState>::iterator it = states.begin(); it != states.end(); ++it )
		it->PostLoad( bInEditor );
}
