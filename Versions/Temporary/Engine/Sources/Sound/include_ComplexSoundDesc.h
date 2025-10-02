

const SSoundStats * GetRandomSound() const
{
	const float fRand = 1.0f * rand() / RAND_MAX;
	float fSum = 0;
	for ( int i = 0; i < sounds.size(); ++i )
	{
		fSum += sounds[i].fProbability;
		if ( fSum > fRand ) 
			return &sounds[i];
	}
	return 0;
} 
virtual void PostLoad( bool bInEditor )
{
	if ( bInEditor )
		return;
	float fDenominator = 0;
	for ( vector<SSoundStats>::const_iterator it = sounds.begin(); it != sounds.end(); ++it )
		fDenominator += it->fProbability;
	fDenominator = 1.0f / fDenominator;
	for ( vector<SSoundStats>::iterator it = sounds.begin(); it != sounds.end(); ++it )
		it->fProbability *= fDenominator;
}
