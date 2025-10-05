void PostLoad( bool bInEditor )
{
	for ( std::vector<SOptionsCategory>::iterator it = categories.begin(); it != categories.end(); ++it )
		it->PostLoad( bInEditor );
}
