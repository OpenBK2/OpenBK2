void PostLoad( bool bInEditor )
{
}

const bool IsAviation() const
{
	return eType == NDb::RT_BOMBERS || 
		eType == NDb::RT_GROUND_ATTACK_PLANES || 
		eType == NDb::RT_RECON || 
		eType == NDb::RT_FIGHTERS ||
		( ( eType == NDb::RT_PARATROOPS || eType == NDb::RT_ELITE_INFANTRY ) && HasPlanes() );
}

const bool HasPlanes() const
{
	for( vector<SReinforcementEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it )
	{
		const SReinforcementEntry &entry = *it;
		if ( !entry.pMechUnit )
			continue;
		if ( entry.pMechUnit->IsAviation() )
			return true;
	}
	return false;
}
