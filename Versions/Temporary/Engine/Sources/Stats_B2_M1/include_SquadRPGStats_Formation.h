/*enum EFormationMoveType
{
	DEFAULT,
	MOVEMENT,
	DEFENSIVE,
	OFFENSIVE,
	SNEAK,
};*/
bool ToAIUnits( bool bInEditor )
{
	FOR_EACH_VAL( order, ToAIUnits, bInEditor ); 
	if ( changesByEvent.empty() ) 
		changesByEvent.push_back( -1 );
	return true;
} 
