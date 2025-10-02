bool ToAIUnits( bool bInEditor )
{
	//Vis2AI( &vOrigin );
	//Vis2AI( &vVisOrigin );
	passability.PostLoad( bInEditor );
	visibility.PostLoad( bInEditor );
	return true;
} 
