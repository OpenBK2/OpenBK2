SHMatrix matDirection;
WORD wDirection;
WORD wAngle;
WORD wRotationSpeed;

bool ToAIUnits( bool bInEditor )
{
	//Vis2AI( &vPos );
	//
	CQuat quat = CQuat( -FP_PI2, V3_AXIS_X );
	quat *= CQuat( ToRadian(fDirection), V3_AXIS_Z );
	matDirection.Set( quat );
	//	
	wDirection = WORD( fDirection * 65536.0f / 360.0f );
	wAngle = WORD( fAngle * 32768.0f / 360.0f );
	if ( bInEditor )
	{
		const float fLocalRotationSpeed = 1.0f / fRotationSpeed * 65535.0f / 1000.0f;
		wRotationSpeed = fLocalRotationSpeed <= 65535.0f ? WORD( fLocalRotationSpeed ) : 65535;
	}
	else
	{
		fRotationSpeed = 1.0f / fRotationSpeed * 65535.0f / 1000.0f;
		wRotationSpeed = fRotationSpeed <= 65535.0f ? WORD( fRotationSpeed ) : 65535;
	}
	gun.ToAIUnits( bInEditor );

	return true;
}

virtual const CVec3 GetShootPointPos() const { return vPos; }
