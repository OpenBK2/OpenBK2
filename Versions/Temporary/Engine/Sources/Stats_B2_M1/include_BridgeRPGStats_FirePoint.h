bool HasFireEffect() const 
{ 
	return !szFireEffect.empty(); 
} 

bool ToAIUnits( bool bInEditor )
{
	if ( bInEditor )
		return true;
	//
	fDirection = ToRadian( fDirection );
	fVerticalAngle = ToRadian( fVerticalAngle );
	//
	return true;
}
