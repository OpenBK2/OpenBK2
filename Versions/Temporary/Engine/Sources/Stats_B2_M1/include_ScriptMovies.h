bool IsValidSequence( int nSeqIndex ) const
{
	NI_VERIFY( (nSeqIndex >= 0) && (nSeqIndex < scriptMovieSequences.size()), "Invalid sequence index!", return false );

	return ( scriptMovieSequences[nSeqIndex].IsValid() );
}


bool HasValidSequence() const
{
	for ( int i = 0; i < scriptMovieSequences.size(); ++i )
	{
		if ( IsValidSequence(i) )
		{
			return true;
		}
	}

	return false;
}



