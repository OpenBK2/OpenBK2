#pragma once

// ************************************************************************************************************************ //
// **
// ** user commands (128 bits)
// **
// **
// **
// ************************************************************************************************************************ //

class CUserCommands
{
	BYTE actions[16];
public:
	int GetSize() const {  return ARRAY_SIZE(actions) * 8; }
	void Clear() { memset( actions, 0, ARRAY_SIZE(actions) ); }

	void RemoveData( const int nBit )
	{
		NI_ASSERT( (nBit >= 0) && (nBit <= 127), StrFmt("Invalid action %d must be in [0..127]", nBit) );
		const int nIndex = nBit / 8;
		actions[nIndex] &= ~( 1UL << (nBit - nIndex*8) );
	}
	void SetData( const int nBit )
	{
		NI_ASSERT( (nBit >= 0) && (nBit <= 127), StrFmt("Invalid action %d must be in [0..127]", nBit) );
		const int nIndex = nBit / 8;
		actions[nIndex] |= ( 1UL << (nBit - nIndex*8) );
	}
	const bool GetData( const int nBit ) const
	{
		NI_ASSERT( (nBit >= 0) && (nBit <= 127), StrFmt("Invalid action %d must be in [0..127]", nBit) );
		const int nIndex = nBit / 8;
		return actions[nIndex] & ( 1UL << (nBit - nIndex*8) );
	}

};

// ************************************************************************************************************************ //
// **
// ** user actions (new!!!, 128 bits)
// **
// **
// **
// ************************************************************************************************************************ //

class CUserActions
{
	DWORD actions[4];
public:
	CUserActions() { Clear(); }
	CUserActions( const DWORD _actions[4] ) { actions[0] = _actions[0]; actions[1] = _actions[1]; actions[2] = _actions[2]; actions[3] = _actions[3]; }
	CUserActions( const CUserActions &userActions ) { actions[0] = userActions.actions[0]; actions[1] = userActions.actions[1]; actions[2] = userActions.actions[2]; actions[3] = userActions.actions[3]; }
	//
	const CUserActions& operator=( const DWORD _actions[2] ) { actions[0] = _actions[0]; actions[1] = _actions[1]; actions[2] = _actions[2]; actions[3] = _actions[3]; return *this; }
	const CUserActions& operator=( const CUserActions &_actions ) { return this->operator=( _actions.actions ); }
	//
	bool operator==( const DWORD _actions[4] ) const { return (_actions[0] == actions[0]) && (_actions[1] == actions[1]) && (_actions[2] == actions[2]) && (_actions[3] == actions[3]); }
	bool operator==( const CUserActions &_actions ) const { return this->operator==( _actions.actions ); }
	bool operator!=( const DWORD _actions[4] ) const { return (_actions[0] != actions[0]) || (_actions[1] != actions[1]) || (_actions[2] != actions[2]) || (_actions[3] != actions[3]); }
	bool operator!=( const CUserActions &_actions ) const { return this->operator!=( _actions.actions ); }
	//
	bool operator>( const DWORD _actions[4] ) const
	{
		return ( actions[0] == _actions[0] ? ( actions[1] == _actions[1] ? ( actions[2] == _actions[2] ? actions[3] > _actions[3] : actions[2] > _actions[2] ) : actions[1] > _actions[1] ) : actions[0] > _actions[0] );
	}
	bool operator>( const CUserActions &_actions ) const { return ( this->operator>( _actions.actions ) ); }
	//
	void operator|=( const CUserActions &ua ) { actions[0] |= ua.actions[0]; actions[1] |= ua.actions[1]; actions[2] |= ua.actions[2]; actions[3] |= ua.actions[3]; }
	int operator&=( const CUserActions &ua ) { actions[0] &= ua.actions[0]; actions[1] &= ua.actions[1]; actions[2] &= ua.actions[2]; actions[3] &= ua.actions[3]; return 0; }

	const CUserActions operator~() const { CUserActions ua; ua.actions[0] = ~actions[0]; ua.actions[1] = ~actions[1]; ua.actions[2] = ~actions[2]; ua.actions[3] = ~actions[3]; return ua; }
	//
	void Clear() { actions[0] = actions[1] = actions[2] = actions[3] = 0; }
	bool IsEmpty() const { return ( actions[0] | actions[1] | actions[2] | actions[3] ) == 0; }
	//
	const bool HasAction( const int nAction ) const
	{
		NI_ASSERT( (nAction >= 0) && (nAction <= 127), StrFmt("Invalid action %d must be in [0..127]", nAction) );
		const int nIndex = nAction / 32;
		return actions[nIndex] & ( 1UL << (nAction - nIndex*32) );
	}
	void SetAction( const int nAction )
	{
		NI_ASSERT( (nAction >= 0) && (nAction <= 127), StrFmt("Invalid action %d must be in [0..127]", nAction) );
		const int nIndex = nAction / 32;
		actions[nIndex] |= ( 1UL << (nAction - nIndex*32) );
	}
	void RemoveAction( const int nAction )
	{
		NI_ASSERT( (nAction >= 0) && (nAction <= 127), StrFmt("Invalid action %d must be in [0..127]", nAction) );
		const int nIndex = nAction / 32;
		actions[nIndex] &= ~( 1UL << (nAction - nIndex*32) );
	}
	const int GetAnyAction() const
	{
		for ( int i = 0; i < 4; i++ )
		{
			if ( actions[i] )
				return GetLSB( actions[i] )+ i*32;
		}

		return 0;
	}
	//
	DWORD GetActions( const int nIndex ) const { return actions[nIndex]; }
	void GetActions( DWORD *_actions ) const { _actions[0] = actions[0]; _actions[1] = actions[1]; _actions[2] = actions[2]; _actions[3] = actions[3]; }
	void GetActions( CUserActions *pActions ) const { GetActions( pActions->GetBuffer() ); }
	void SetActions( const DWORD _actions[4] ) { actions[0] = _actions[0]; actions[1] = _actions[1]; actions[2] = _actions[2]; actions[3] = _actions[3]; }
	DWORD* GetBuffer() { return &(actions[0]); }
	//
	void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
};


