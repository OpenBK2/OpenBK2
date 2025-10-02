vector< SSlot > aiSlots;

vector<int> aiSlotToSlot;		// index - AI slot, value - index in slot[]

void SBuildingRPGStats::ToAIUnits( bool bInEditor )
{
	eGameType = SGVOGT_BUILDING;
	SObjectBaseRPGStats::ToAIUnits( bInEditor );
	// (c) Alexander Vinnikov's method of visibility
	if ( !bInEditor && bVisibilitySrink )
	{
		if ( passability.GetSizeX() > 2 && passability.GetSizeY() > 2 )
			visibility.SetSizes( passability.GetSizeX() - 2, passability.GetSizeY() - 2 );
		for ( int x = 0; x < visibility.GetSizeX(); ++x )
		{
			for	 ( int y = 0; y < visibility.GetSizeY(); ++y )
			{
				visibility[y][x] = passability[y][x]			&& passability[y][x]		&& passability[y][x] &&
													 passability[y][x+1]		&& passability[y+1][x+1]&& passability[y+2][x+1]	&&
													 passability[y][x+2]		&& passability[y+1][x+2]&& passability[y+2][x+2];
			}
		}
		bUsePassabilityForVisibility = false;
		vVisOrigin = vOrigin - CVec2( 32, 32 );
	}
	//
	FOR_EACH_VAL( entrances, ToAIUnits, bInEditor );

	// cheat: use aiSlots in AILogic
	FOR_EACH_VAL( slots, ToAIUnits, bInEditor );
	int nAISlots = 0;
	for ( int i = 0; i < slots.size(); ++i )
		nAISlots += slots[i].nNumFirePlaces;
	aiSlots.resize( nAISlots );
	aiSlotToSlot.resize( nAISlots );
	int nAISlot = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		for ( int j = 0; j < slots[i].nNumFirePlaces; j++ ) // copy only AIInfo
		{
			aiSlotToSlot[nAISlot] = i;
			aiSlots[nAISlot] = slots[i];
			++nAISlot;
		}
	}
	FOR_EACH_VAL( firePoints, ToAIUnits, bInEditor );
	FOR_EACH_VAL( smokePoints, ToAIUnits, bInEditor );
	FOR_EACH_VAL( dirExplosions, ToAIUnits, bInEditor );
} 
