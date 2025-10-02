bool ToAIUnits( bool bInEditor )
{
	//Vis2AI( &vOrigin );
	//Vis2AI( &vVisOrigin );
	// tiles to world points
	passability.PostLoad( bInEditor );
	visibility.PostLoad( bInEditor );
	//
	return true;
} 
