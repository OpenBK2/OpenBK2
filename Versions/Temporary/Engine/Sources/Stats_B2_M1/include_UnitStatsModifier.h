void Apply( const SUnitStatsModifier &mod, const bool bForward )
{
	weaponPiercing.Apply( mod.weaponPiercing, bForward );
	weaponArea.Apply( mod.weaponArea, bForward );
	weaponTrackDamageProb.Apply( mod.weaponTrackDamageProb, bForward );
	weaponRelaxTime.Apply( mod.weaponRelaxTime, bForward );
	camouflage.Apply( mod.camouflage, bForward );
	weaponDispersion.Apply( mod.weaponDispersion, bForward );
	weaponDamage.Apply( mod.weaponDamage, bForward );
	speed.Apply( mod.speed, bForward );
	weaponArea2.Apply( mod.weaponArea2, bForward );
	weaponAimTime.Apply( mod.weaponAimTime, bForward );
	weaponShellSpeed.Apply( mod.weaponShellSpeed, bForward );
	rotateSpeed.Apply( mod.rotateSpeed, bForward );
	sightRange.Apply( mod.sightRange, bForward );
	durability.Apply( mod.durability, bForward );
	smallAABBCoeff.Apply( mod.smallAABBCoeff, bForward );
	sightPower.Apply( mod.sightPower, bForward );
	cover.Apply( mod.cover, bForward );
}
