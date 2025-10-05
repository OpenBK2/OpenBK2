void PostLoad( bool bInEditor )
{
	for ( std::vector<SUIScreenEntry>::iterator it = screens.begin(); it != screens.end(); ++it )
		it->PostLoad( bInEditor );
}
