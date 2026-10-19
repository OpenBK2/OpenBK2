
void ReMapCommands( CUserCommands &ai, CUserActions &user );

inline void AddValue( CUserCommands &array, EActionCommand eCommand )
{
	array.SetData( eCommand );
}

inline void RemValue( CUserCommands &array, EActionCommand eCommand )
{
	array.RemoveData( eCommand );
}

void ToAIUnits( bool bInEditor );

void PostLoad( bool bInEditor ) override
{
	ToAIUnits( bInEditor );
}
