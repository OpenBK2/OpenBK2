void PostLoad( bool bInEditor ) override
{
	for ( std::vector<SUIScreenEntry>::iterator it = screens.begin(); it != screens.end(); ++it )
	{
		it->PostLoad( bInEditor );
	}
}
