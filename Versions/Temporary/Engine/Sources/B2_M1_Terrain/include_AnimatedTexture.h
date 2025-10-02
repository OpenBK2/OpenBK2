void PostLoad( bool bInEditor )
{
	if ( nUseFrames == 0 ) 
	{
		nUseFrames = nNumFramesX * nNumFramesY;
	}
}
