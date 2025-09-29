
virtual const char* GetName() const { return szStatsType.c_str(); }
virtual const char* GetParentName() const { return szParentName.c_str(); }

//
virtual void ToAIUnits( bool bInEditor ) {}
// проверка статсов на корректность
virtual bool Validate() { return true; }
//
virtual void PostLoad( bool bInEditor )
{
	ToAIUnits( bInEditor );
}