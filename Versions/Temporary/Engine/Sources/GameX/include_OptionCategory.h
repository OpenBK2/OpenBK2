void PostLoad( bool bInEditor )
{
	for ( std::vector<SOptionEntry>::iterator it = options.begin(); it != options.end(); ++it )
		it->PostLoad( bInEditor );
}
