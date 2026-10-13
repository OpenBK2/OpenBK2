//SParameterModifier() : fMultBonus( 1.0f ), fAddBonus( 0.0f ), nZeroCount( 0 ) {  }
void Apply( const SParameterModifier &param, const bool bForward )
{
	if ( bForward )
	{
		fMultBonus *= param.fMultBonus;
		fAddBonus += param.fAddBonus;
		nZeroCount += param.nZeroCount;
	}
	else
	{
		fMultBonus /= param.fMultBonus;
		fAddBonus -= param.fAddBonus;
		nZeroCount -= param.nZeroCount;
		NI_ASSERT( nZeroCount >= 0, "applied backward effects more than forward" );
	}
}
float Get( const float fInitialValue ) const
{
	return (nZeroCount ? 0.0f : ( fInitialValue + fAddBonus ) * fMultBonus);
}
