void PostLoad( bool bInEditor )
{
	if ( !bInEditor )
		fPeriod -= float( rand() ) / RAND_MAX * fPeriodVariation;
	fInvPeriod = 1.0f / fPeriod;
}
