namespace NDb
{

void SMechUnitRPGStats::ToAIUnits( bool bInEditor )
{
	SUnitBaseRPGStats::ToAIUnits( bInEditor );
	if ( !bInEditor )
	{
		// градусы/сек <=> угол 65536/тик
		fRotateSpeed *= float( (65536.0 / 360.0) / 1000.0 );
		fTiltSpeed *= float( (65536.0 / 360.0) / 1000.0 );
		fTiltAcceleration *= float( (65536.0 / 360.0) / 1000.0 / 1000.0 );
		// метры <=> AI точки
		fTurnRadius *= float( 32.0f );
		fMaxHeight *= float( 32.0f );
		// move entrance point in front direction outside bounding box
		if ( vEntrancePoint == VNULL2 )
			vEntrancePoint.y = vAABBCenter.y + vAABBHalfSize.y;
	}

	// armor
	FOR_EACH_ARR_VAL( armors, ToAIUnits, armors.size(), bInEditor ); 
	// проинициализировать min/max Armor
	if ( !armors.empty() ) 
	{
		nMinArmor = armors[0].nMin;
		nMaxArmor = armors[0].nMax;
		for ( int i = 1; i < 4; ++i )
		{
			nMinArmor = Min( nMinArmor, armors[i].nMin );
			nMaxArmor = Max( nMaxArmor, armors[i].nMax );
		}
	}
	//
	FOR_EACH_VAL( platforms, ToAIUnits, bInEditor );
	//
	if ( !bInEditor )
	{
		if ( nPriority == 0 )
			nPriority = 1;
	}
	//
	nPrimaryGun = -1;
	nPrimaryPlatform = -1;
	for ( int i = 0; i < platforms.size(); ++i )
		CountPrimaryGuns( -1, i );
	//
	wDivingAngle = fDivingAngle * 65535.0f / 360.0f;
	wClimbingAngle = fClimbAngle * 65535.0f / 360.0f;
	wTiltAngle = fTiltAngle * 65535.0f / 360.0f;
	// make action flags, based on available shells
	for ( int i = 0; i < platforms.size(); ++i )
		CountShellTypes( -1, i );

	for ( int i = 0; i < allowedPlaneManuvers.size(); ++i )
		manuverMap[int(allowedPlaneManuvers[i])] = true;
}

}
