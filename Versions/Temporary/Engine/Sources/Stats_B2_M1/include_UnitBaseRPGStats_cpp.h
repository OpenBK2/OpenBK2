namespace NDb
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SUnitBaseRPGStats::CountPrimaryGuns( const int nUniqueID, const int nPlatform )
{
	nPrimaryGun = -1;

	Zero( nAmmos );
	for ( int i = 0; i < GetGunsSize( nUniqueID, nPlatform ); ++i )
	{
		const SBaseGunRPGStats &gun = GetGun( nUniqueID, nPlatform, i );
		if ( nPrimaryGun == -1 && gun.bIsPrimary )
		{
			nPrimaryGun = i;
			nPrimaryPlatform = nPlatform;
		}

		nAmmos[gun.bIsPrimary ? 0 : 1] += gun.nAmmo;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SUnitBaseRPGStats::CountShellTypes( const int nUniqueID, const int nPlatform )
{
	// count shell etypes
	int nDamagetypes[3] = { 0, 0, 0 };
	for ( int i = 0; i < GetGunsSize( nUniqueID, nPlatform ); ++i )
	{
		const SBaseGunRPGStats &gun = GetGun( nUniqueID, nPlatform, i );
		if ( gun.pWeapon )
		{
			for ( vector<SWeaponRPGStats::SShell>::const_iterator shell = gun.pWeapon->shells.begin(); shell != gun.pWeapon->shells.end(); ++shell )
				nDamagetypes[shell->eDamageType] = 1;
		}
	}
	// set actions
	/*if ( nDamagetypes[0] + nDamagetypes[1] + nDamagetypes[2] > 1 )
	{
		if ( nDamagetypes[DAMAGE_HEALTH] != 0 ) 
			availUserActions.SetAction( USER_ACTION_USE_SHELL_DAMAGE );
		if ( nDamagetypes[DAMAGE_MORALE] != 0 ) 
			availUserActions.SetAction( USER_ACTION_USE_SHELL_AGIT );
		if ( nDamagetypes[DAMAGE_FOG] != 0 ) 
			availUserActions.SetAction( USER_ACTION_USE_SHELL_SMOKE );
	}
	else
	{
		RemoveCommand( ACTION_COMMAND_CHANGE_SHELLTYPE );
		availUserActions.RemoveAction( USER_ACTION_USE_SHELL_DAMAGE );
		availUserActions.RemoveAction( USER_ACTION_USE_SHELL_AGIT );
		availUserActions.RemoveAction( USER_ACTION_USE_SHELL_SMOKE );
	}*/
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SUnitBaseRPGStats::ToAIUnits( bool bInEditor )
{
	if ( eGameType != SGVOGT_TANK_PIT )
		eGameType = SGVOGT_UNIT;
	SHPObjectRPGStats::ToAIUnits( bInEditor );
	etype = NDb::ReMapRPGType( eDBtype );
	//
	if ( !bInEditor )
	{
		fSight *= 1.0f;
		// киломерты/час <=> точки/тик
		fSpeed *= float( ( 1000.0 * 32.0f ) / ( 3600.0 * 1000.0 ) );
		//
		fCamouflage /= 100.0f;
	}
	// сек. <=> тик
	nUninstallRotate = fUninstallRotate * 1000.0f;
	nUninstallTransport = fUninstallTransport * 1000.0f;
	FOR_EACH_VAL( aabb_as, ToAIUnits, bInEditor );
	FOR_EACH_VAL( aabb_ds, ToAIUnits, bInEditor );
	// convert AI price
	if ( !bInEditor )
	{
		const string szVarName = StrFmt( "AIPrice.%x", int(etype) );
		fPrice *= NGlobal::GetVar( szVarName.c_str(), 1.0f );
	}

	//PATHFINDING{
	//boundCircle.bIsRound = false;
	//PATHFINDING}

	if ( vAABBHalfSize.y / vAABBHalfSize.x > 1.5 )
		boundCircle.bIsRound = false;
	else
	{
		boundCircle.bIsRound = true;
		boundCircle.fRadius = fabs( vAABBHalfSize );
	}
	nBoundTileRadius = IsInfantry() ? 0 : int(ceil( (2 * vAABBHalfSize.x) / float(32.0f) )) / 2;
	// transfer animation information to animdescs
	if ( bInEditor == false )
	{
		aabb_as.clear();
		aabb_ds.clear();
		animdescs.clear();
		animdescs.resize( NDb::__ANIMATION_TYPE_COUNTER );
		const SSkeleton *pSkeleton = 0;
		for ( int i = 0; i < pvisualObject->models.size(); ++i )
		{
			if ( pvisualObject->models[i].eSeason == SEASON_SUMMER )
			{
				pSkeleton = pvisualObject->models[i].pModel->pSkeleton;
				break;
			}
		}
		if ( pSkeleton == 0 && !pvisualObject->models.empty() )
			pSkeleton = pvisualObject->models[0].pModel->pSkeleton;
		//
		if ( pSkeleton )
		{
			for ( int i = 0; i < pSkeleton->animations.size(); ++i )
			{
				const SAnimB2 *pAnim = checked_cast_ptr<const SAnimB2 *>( pSkeleton->animations[i] );
				vector<SAnimDesc>::iterator pos = animdescs[pAnim->eType].anims.insert( animdescs[pAnim->eType].anims.end(), SAnimDesc() );
				pos->nLength = pAnim->nLength;
				pos->nAction = pAnim->nAction;
				if ( IsInfantry() )
					pos->pAnimation = pAnim;
				else
					pos->nFrameIndex = i;
				pos->nAABB_A = -1;
				if ( pAnim->aabb_a.vHalfSize != VNULL3 )
				{
					SAABBDesc desc;
					desc.vCenter = CVec2( pAnim->aabb_a.vCenter.x, pAnim->aabb_a.vCenter.y );
					desc.vHalfSize = CVec2( pAnim->aabb_a.vHalfSize.x, pAnim->aabb_a.vHalfSize.y );
					aabb_as.push_back( desc );
					pos->nAABB_A = aabb_as.size() - 1;
				}
				pos->nAABB_D = -1;
				if ( pAnim->aabb_d.vHalfSize != VNULL3 )
				{
					SAABBDesc desc;
					desc.vCenter = CVec2( pAnim->aabb_d.vCenter.x, pAnim->aabb_d.vCenter.y );
					desc.vHalfSize = CVec2( pAnim->aabb_d.vHalfSize.x, pAnim->aabb_d.vHalfSize.y );
					aabb_ds.push_back( desc );
					pos->nAABB_D = aabb_ds.size() - 1;
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
