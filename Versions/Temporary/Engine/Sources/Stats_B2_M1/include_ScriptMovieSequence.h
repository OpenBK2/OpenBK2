bool IsValid() const
{
	return ( (posKeys.size() > 0) && (followKeys.size() > 0) );
}


float GetLength() const
{
	float fLen = 0;

	for ( std::vector<SScriptMovieKeyPos>::const_iterator itPosKey = posKeys.begin();
																									 itPosKey != posKeys.end(); ++itPosKey )
	{
		fLen = (std::max)( fLen, itPosKey->fStartTime );
	}
	for ( std::vector<SScriptMovieKeyFollow>::const_iterator itFollowKey = followKeys.begin();
																									 itFollowKey != followKeys.end(); ++itFollowKey )
	{
		fLen = (std::max)( fLen, itFollowKey->fStartTime );
	}

	return fLen;
}
