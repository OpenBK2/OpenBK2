#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "..\Stats_B2_M1\AITypes.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// последняя party обязательно должна быть нейтральна ко всем
class CDiplomacy
{
	public: int operator&( IBinSaver &saver ); private:;
	
	// 0, 1 - игровые стороны, 2 - нейтралы
	vector<int> playerParty;
	int nMyNumber;

	vector<int> isPlayerExist;

	bool bNetGame;
public:
	CDiplomacy() : nMyNumber( 0 ), bNetGame( false ) { }
	
	void Load( const vector<int> &playerParty );
	void SetNPlayers( const int nPlayers ) { playerParty.resize( nPlayers + 1, 0 ); playerParty[nPlayers] = 2; }

	void Clear() { }

	const int GetNPlayers() const { return playerParty.size(); }

	const EDiplomacyInfo GetDiplStatus( const BYTE a, const BYTE b ) const
	{ 
		if ( playerParty[a] == 2 || playerParty[b] == 2 )
			return EDI_NEUTRAL;
		else if ( playerParty[a] != playerParty[b] )
			return EDI_ENEMY;
		else
			return EDI_FRIEND;
	}

	const EDiplomacyInfo GetDiplStatusForParties( const BYTE nParty1, const BYTE nParty2 )
	{
		if ( nParty1 == 2 || nParty2 == 2 )
			return EDI_NEUTRAL;
		else
			if ( nParty1 != nParty2 )
				return EDI_ENEMY;
			else
				return EDI_FRIEND;
	}

	const BYTE GetNParty( const BYTE cPlayer ) const { return playerParty[cPlayer]; }
	const BYTE GetMyNumber() const { return nMyNumber; }
	const BYTE GetMyParty() const { return GetNParty( GetMyNumber() ); }
	const bool IsAIPlayer( const BYTE cPlayer ) const { return !bNetGame && cPlayer != GetMyNumber(); }
	const BYTE GetNeutralPlayer() const { return GetNPlayers() - 1; }
	// номер нейтральной стороны
	int GetNeutralParty() const { return 2; }

	void SetParty( const BYTE nPlayer, const BYTE newParty ) { playerParty[nPlayer] = newParty; }
	
	void SetMyNumber( const int nNumber ) { nMyNumber = nNumber; }
	void SetNetGame( bool _bNetGame );
	
	bool IsNetGame() const { return bNetGame; }

	//
	bool IsPlayerExist( const int nPlayer ) const;
	void SetPlayerNotExist( const int nPlayer );
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int ANY_PARTY = EDI_FRIEND | EDI_ENEMY | EDI_NEUTRAL;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
