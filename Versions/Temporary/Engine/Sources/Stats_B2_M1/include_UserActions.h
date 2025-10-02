const SUserActions& operator=( const DWORD _actions[2] ) { actions[0] = _actions[0]; actions[1] = _actions[1]; return *this; }
const SUserActions& operator=( const SUserActions &_actions ) { return this->operator=( _actions.actions ); }
//
bool operator==( const DWORD _actions[2] ) const { return (_actions[0] == actions[0]) && (_actions[1] == actions[1]); }
bool operator==( const SUserActions &_actions ) const { return this->operator==( _actions.actions ); }
bool operator!=( const DWORD _actions[2] ) const { return (_actions[0] != actions[0]) || (_actions[1] != actions[1]); }
bool operator!=( const SUserActions &_actions ) const { return this->operator!=( _actions.actions ); }
//
void operator|=( const SUserActions &ua ) { actions[0] |= ua.actions[0]; actions[1] |= ua.actions[1]; }
int operator&=( const SUserActions &ua ) { actions[0] &= ua.actions[0]; actions[1] &= ua.actions[1]; }
//
void Clear() { actions[0] = actions[1] = 0; }
bool IsEmpty() const { return ( actions[0] | actions[1] ) == 0; }
//
const bool HasAction( const int nAction ) const
{
	NI_ASSERT( (nAction >= 0) && (nAction <= 63), StrFmt("Invalid action %d must be in [0..63]", nAction) );
	const int nIndex = nAction >> 5;
	return actions[nIndex] & ( 1UL << (nAction - nIndex*32) );
}
void SetAction( const int nAction )
{
	NI_ASSERT( (nAction >= 0) && (nAction <= 63), StrFmt("Invalid action %d must be in [0..63]", nAction) );
	const int nIndex = nAction >> 5;
	actions[nIndex] |= ( 1UL << (nAction - nIndex*32) );
}
void RemoveAction( const int nAction )
{
	NI_ASSERT( (nAction >= 0) && (nAction <= 63), StrFmt("Invalid action %d must be in [0..63]", nAction) );
	const int nIndex = nAction >> 5;
	actions[nIndex] &= ~( 1UL << (nAction - nIndex*32) );
}
//
DWORD GetActions( const int nIndex ) const { return actions[nIndex]; }
void GetActions( DWORD *_actions ) const { _actions[0] = actions[0]; _actions[1] = actions[1]; }
void GetActions( SUserActions *pActions ) const { GetActions( pActions->GetBuffer() ); }
void SetActions( const DWORD _actions[2] ) { actions[0] = _actions[0]; actions[1] = _actions[1]; }
DWORD* GetBuffer() { return &(actions[0]); } 
