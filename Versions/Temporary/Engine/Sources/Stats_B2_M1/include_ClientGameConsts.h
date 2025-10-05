void PostLoad( bool bInEditor )
{
	for ( std::vector<SAckParameter>::iterator it = acksParameters.begin(); it != acksParameters.end(); ++it )
		it->PostLoad( bInEditor );
}
