void PostLoad( bool bInEditor )
{
}

const bool IsAviation() const
{
	bool isMixedReinf = ( eType == NDb::RT_EXTRA_MIXED_1 || eType == NDb::RT_EXTRA_MIXED_2 || eType == NDb::RT_EXTRA_MIXED_3 || eType == NDb::RT_EXTRA_MIXED_4 || eType == NDb::RT_EXTRA_MIXED_5 ||
							  eType == NDb::RT_EXTRA_MAXLVL_MIXED_1 || eType == NDb::RT_EXTRA_MAXLVL_MIXED_2 || eType == NDb::RT_EXTRA_MAXLVL_MIXED_3 || eType == NDb::RT_EXTRA_MAXLVL_MIXED_4 || eType == NDb::RT_EXTRA_MAXLVL_MIXED_5 );
	return eType == NDb::RT_BOMBERS || 
		eType == NDb::RT_GROUND_ATTACK_PLANES || 
		eType == NDb::RT_RECON || 
		eType == NDb::RT_FIGHTERS ||
		eType == NDb::RT_EXTRA_AIR_1 ||
		eType == NDb::RT_EXTRA_AIR_2 ||
		eType == NDb::RT_EXTRA_AIR_3 ||
		eType == NDb::RT_EXTRA_AIR_4 ||
		eType == NDb::RT_EXTRA_AIR_5 ||
		eType == NDb::RT_EXTRA_MAXLVL_AIR_1 ||
		eType == NDb::RT_EXTRA_MAXLVL_AIR_2 ||
		eType == NDb::RT_EXTRA_MAXLVL_AIR_3 ||
		eType == NDb::RT_EXTRA_MAXLVL_AIR_4 ||
		eType == NDb::RT_EXTRA_MAXLVL_AIR_5 ||
		( ( eType == NDb::RT_PARATROOPS || eType == NDb::RT_ELITE_INFANTRY || isMixedReinf ) && HasPlanes() );
}

const bool HasPlanes() const
{
	for( std::vector<SReinforcementEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it )
	{
		const SReinforcementEntry &entry = *it;
		if ( !entry.pMechUnit )
			continue;
		if ( entry.pMechUnit->IsAviation() )
			return true;
	}
	return false;
}
