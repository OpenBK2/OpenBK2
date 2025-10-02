void PostLoad( bool bInEditor )
{	
	// CRAP, to emulate non zero default values
	if ( vParticlesColor == VNULL3 )
		vParticlesColor = CVec3( 0.25, 0.25, 0.25 );
	if ( vDymanicLightsModifications == VNULL3 )
		vDymanicLightsModifications = CVec3( 1, 1, 1 );

	if ( bInEditor )
		return;
	/*
	vLightColor *= 0.25f;
	vAmbientColor *= 0.25f;
	vGroundAmbientColor *= 0.25f;
	*/


	if ( !bWhitening ) 
	{
		vLightColor *= 0.25f;
		vAmbientColor *= 0.25f;
		vShadeColor *= 0.25f;
		vIncidentShadowColor *= 0.25f;
	}
}
