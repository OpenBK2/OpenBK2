void ToAIUnits( bool bInEditor )
{
	SCommonRPGStats::ToAIUnits( bInEditor );
	if ( !bInEditor )
	{
		// find max type index
		int nMaxType = 0;
		for ( vector<SAckType>::const_iterator it = types.begin(); it != types.end(); ++it )
			nMaxType = Max( nMaxType, int(it->eAckType) );
		NI_ASSERT( nMaxType + 1 >= types.size(), StrFmt("Wrong acks in \"%s\" set", szParentName.c_str()) );
		// copy acks to the new positions in accordance with it's type
		vector<SAckType> types2( nMaxType + 1 );
		for ( vector<SAckType>::const_iterator it = types.begin(); it != types.end(); ++it )
			types2[it->eAckType] = *it;
		// swap old and new vectors
		swap( types2, types );
	}
}
