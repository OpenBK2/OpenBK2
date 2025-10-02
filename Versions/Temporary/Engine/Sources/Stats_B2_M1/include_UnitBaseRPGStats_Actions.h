
void ReMapCommands( CUserCommands &ai, CUserActions &user );

inline void AddValue( CUserCommands &array, int nBit )
{
	array.SetData( nBit );
}
inline void RemValue( CUserCommands &array, int nBit )
{
	array.RemoveData( nBit );
}

void ToAIUnits( bool bInEditor );
virtual void PostLoad( bool bInEditor )
{
	ToAIUnits( bInEditor );
}

