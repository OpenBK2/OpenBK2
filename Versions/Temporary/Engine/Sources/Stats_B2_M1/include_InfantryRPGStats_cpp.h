namespace NDb
{

void SInfantryRPGStats::ToAIUnits( bool bInEditor )
{
	SUnitBaseRPGStats::ToAIUnits( bInEditor );
	//
	nPriority = 0;
	//
	nPrimaryGun = -1;
	nPrimaryPlatform = -1;
	CountPrimaryGuns( -1, 0 );
	// make action flags, based on available shells
	CountShellTypes( -1, 0 );

	FOR_EACH_VAL( guns, ToAIUnits, bInEditor );
}

}

