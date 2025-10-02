
bool ToAIUnits( bool bInEditor )
{
	//Vis2AI( &vFirePlace );
	//Vis2AI( &vAABBCenter );
	//Vis2AI( &vAABBHalfSize );
	//for ( vector<CVec2>::iterator it = fireplaces.begin(); it != fireplaces.end(); ++it )
		//Vis2AI( &(*it) );
	return true;
}

float GetLength() const 
{ 
	return vAABBHalfSize.x*2.0f*AI_TO_VIS;
}
float GetHalfLength() const 
{ 
	return vAABBHalfSize.x*AI_TO_VIS; 
}
/*const CVec2 GetVisFirePlace() const
{
	CVec2 vTemp;
	AI2Vis( &vTemp, vFirePlace );
	return vTemp;
}*/
const CVec2 GetVisAABBCenter() const
{
	CVec2 vTemp;
	AI2Vis( &vTemp, vAABBCenter );
	return vTemp;
}
const CVec3 GetVisAABBHalfSize() const
{
	CVec3 vTemp;
	AI2Vis( &vTemp, vAABBHalfSize );
	return vTemp;
}