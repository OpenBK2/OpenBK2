void PostLoad( bool bInEditor )
{
	for ( vector<SOptionsCategory>::iterator it = categories.begin(); it != categories.end(); ++it )
		it->PostLoad( bInEditor );
}
