void PostLoad( bool bInEditor )
{
	for ( std::vector<SOptionEntryState>::iterator it = states.begin(); it != states.end(); ++it )
		it->PostLoad( bInEditor );
}
