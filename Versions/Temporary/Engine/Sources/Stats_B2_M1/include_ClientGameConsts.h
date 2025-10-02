void PostLoad( bool bInEditor )
{
	for ( vector<SAckParameter>::iterator it = acksParameters.begin(); it != acksParameters.end(); ++it )
		it->PostLoad( bInEditor );
}
