virtual void ToAIUnits( bool bInEditor )
{
	SCommonRPGStats::ToAIUnits( bInEditor );
}
//
float GetMapHP() const { return fMaxHP; }
float GetHP( const float fHPPercentage ) const { return fMaxHP * fHPPercentage; }
virtual int GetArmor( const int n ) const { return ( defences[n].nArmorMin + defences[n].nArmorMax ) / 2; }
virtual int GetMinPossibleArmor( const int n ) const { return defences[n].nArmorMin; }
virtual int GetMaxPossibleArmor( const int n ) const { return defences[n].nArmorMax; }
virtual int GetRandomArmor( const int n ) const 
{ 
	return NRandom::RandomCheck( defences[n].nArmorMin, defences[n].nArmorMax ); 
}
//
virtual const class CUserActions* GetUserActions( bool bActionsBy ) const { return 0; }

