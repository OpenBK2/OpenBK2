virtual const char *GetName() const
{
	return "AIExpLevel";
}

virtual const char *GetParentName() const
{
	return szTypeName.c_str();
}

// This handwritten field is outside the generated constructor's initializers.
enum EUnitRPGType eType
{
};

// преобразовать из человеческих единиц в AI
void ToAIUnits( bool bInEditor ) {}

void PostLoad( bool bInEditor ) override
{
	//eType = NDb::ReMapRPGType( eDBType );
}
