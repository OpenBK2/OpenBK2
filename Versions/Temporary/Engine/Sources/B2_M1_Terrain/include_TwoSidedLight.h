void PostLoad( bool bInEditor )
{
	if ( bInEditor )
		return;
	vLightColor -= vAmbientColor;
	vShadeColor -= vShadeAmbientColor;
	if ( !bWhitening ) 
	{
		vLightColor *= 0.25f;
		vAmbientColor *= 0.25f;
		vShadeColor *= 0.25f;
		vShadeAmbientColor *= 0.25f;
	}
} 
