void Combine( const SParameterModifier &param )
{
	// Rebuild from active sources instead of trying to invert floating-point arithmetic.
	fMultBonus *= param.fMultBonus;
	fAddBonus += param.fAddBonus;
	nZeroCount += param.nZeroCount;
}

float Get( const float fInitialValue ) const
{
	return ( nZeroCount ? 0.0f : ( fInitialValue + fAddBonus ) * fMultBonus );
}
