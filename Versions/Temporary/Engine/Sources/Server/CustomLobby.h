#pragma once

#include "GameLobby.h"

struct IStatisticsCollector;

class CCustomLobby : public CGameLobby
{
	OBJECT_NOCOPY_METHODS( CCustomLobby )

	int nLobbyID;
	int nPlayersTotal;
	int nGamesTotal;

	struct SGetClientInfoFunc
	{
		CCustomLobby *pThis;

		SGetClientInfoFunc( CCustomLobby *_pThis )
			: pThis( _pThis )
		{
		}

		const bool operator()( const int nClientID, struct SCustomLobbyClientInfo *pInfo ) const
		{
			return pThis->GetClientInfo( nClientID, pInfo );
		}
	};

	CObj<IStatisticsCollector> pStatisticsCollector;
	//
	const bool GetClientInfo( const int nClientID, struct SCustomLobbyClientInfo *pInfo ) const;

protected:
	const int GetLobbyID() const override
	{
		return nLobbyID;
	}

	void ClientEnterToLobby( const int nClientID ) override;
	void ClientLeaveLobby( const int nClientID ) override;

	void GetLobbyClients( class CGetLobbyClientsPacket *pPacket ) override;
	int CreateGame( const int nMaxPlayers ) override;
	void GameDead( const int nGameID ) override;

	bool PlayerCanKickPlayer() const override
	{
		return true;
	}

	bool PlayerCanSeeGamesList() const override
	{
		return true;
	}

	bool PlayerCanUpdateGameInfo() const override
	{
		return true;
	}

	bool PlayerNeedSpecificGameInfo() const override
	{
		return true;
	}

public:
	CCustomLobby() {}

	CCustomLobby( class CClients *pClients, const std::string &szCfgFile );

	bool Segment() override;

	friend struct SGetClientInfoFunc;
};
