void Combine( const SUnitStatsModifier &mod )
{
	weaponPiercing.Combine( mod.weaponPiercing );
	weaponArea.Combine( mod.weaponArea );
	weaponTrackDamageProb.Combine( mod.weaponTrackDamageProb );
	weaponRelaxTime.Combine( mod.weaponRelaxTime );
	camouflage.Combine( mod.camouflage );
	weaponDispersion.Combine( mod.weaponDispersion );
	weaponDamage.Combine( mod.weaponDamage );
	speed.Combine( mod.speed );
	weaponArea2.Combine( mod.weaponArea2 );
	weaponAimTime.Combine( mod.weaponAimTime );
	weaponShellSpeed.Combine( mod.weaponShellSpeed );
	rotateSpeed.Combine( mod.rotateSpeed );
	sightRange.Combine( mod.sightRange );
	durability.Combine( mod.durability );
	smallAABBCoeff.Combine( mod.smallAABBCoeff );
	sightPower.Combine( mod.sightPower );
	cover.Combine( mod.cover );
}
