void PostLoad( bool bInEditor )
{
	if ( !bInEditor )
	{
		fPeriod -= float( rand() ) / static_cast<float>( RAND_MAX ) * fPeriodVariation;
	}
	fInvPeriod = 1.0f / fPeriod;
}
