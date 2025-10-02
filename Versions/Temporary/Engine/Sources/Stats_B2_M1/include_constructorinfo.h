
void GetGunPlatformIndex( const int nUniqueID, const int nPlatform, const int nGun, int *pnPlatform, int *pnGun ) const
{ 
	if ( ConstructorInfo() )
	{
		const vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if (  ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
		{
			*pnPlatform = (*pPlatforms)[nPlatform].nPlatformIndex;
			*pnGun = (*pPlatforms)[nPlatform].gunIndexes[nGun];
			return;
		}
	}
	*pnPlatform = nPlatform;
	*pnGun = nGun;
}

