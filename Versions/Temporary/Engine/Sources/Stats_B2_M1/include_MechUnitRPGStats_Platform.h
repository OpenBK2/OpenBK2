WORD wVerticalRotationSpeed;
WORD wHorizontalRotationSpeed;

bool ToAIUnits( bool bInEditor )
{
	constraint.ToAIUnits( bInEditor );
	constraintVertical.ToAIUnits( bInEditor );
	// (секунды на полный оборот <=> градусы65535/тик)
	if ( fHorizontalRotationSpeed != 0 )
	{
		float fLocalHSpeed = 1.0f / fHorizontalRotationSpeed * 65535.0f / 1000.0f;
		if ( fLocalHSpeed != 0 )
			fLocalHSpeed = (std::max)( 1.0f, fLocalHSpeed );
		wHorizontalRotationSpeed = fLocalHSpeed <= 65535.0f ? WORD( fLocalHSpeed ) : 65535;
		if ( !bInEditor )
			fHorizontalRotationSpeed = fLocalHSpeed;
	}
	else if ( !bInEditor )
	{
		fHorizontalRotationSpeed = 1000;
		wHorizontalRotationSpeed = 0;
	}
	if ( fVerticalRotationSpeed != 0 )
	{
		float fLocalVSpeed = 1.0f / fVerticalRotationSpeed * 65535.0f / 1000.0f;
		if ( fLocalVSpeed != 0 )
			fLocalVSpeed = (std::max)( 1.0f, fLocalVSpeed );
		wVerticalRotationSpeed = fLocalVSpeed <= 65535.0f ? WORD( fLocalVSpeed ) : 65535;
		if ( !bInEditor )
			fVerticalRotationSpeed = fLocalVSpeed;
	}
	else if ( !bInEditor )
	{
		fVerticalRotationSpeed = 1000;
		wVerticalRotationSpeed = 0;
	}
	//
	FOR_EACH_VAL( guns, ToAIUnits, bInEditor );
	//
	return true;
} 

friend struct SMechUnitRPGStats;

