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
		SGetClientInfoFunc( CCustomLobby *_pThis ) : pThis( _pThis ) { }
		const bool operator()( const int nClientID, struct SCustomLobbyClientInfo *pInfo ) const 
			{ return pThis->GetClientInfo( nClientID, pInfo ); }
	};

	CObj<IStatisticsCollector> pStatisticsCollector;
	//
	const bool GetClientInfo( const int nClientID, struct SCustomLobbyClientInfo *pInfo ) const;
protected:
	virtual const int GetLobbyID() const { return nLobbyID; }

	virtual void ClientEnterToLobby( const int nClientID );
	virtual void ClientLeaveLobby( const int nClientID );

	virtual void GetLobbyClients( class CGetLobbyClientsPacket *pPacket );
	virtual int CreateGame( const int nMaxPlayers );
	virtual void GameDead( const int nGameID );
	virtual bool PlayerCanKickPlayer() const { return true; }
	virtual bool PlayerCanSeeGamesList() const { return true; }
	virtual bool PlayerCanUpdateGameInfo() const { return true; }
	virtual bool PlayerNeedSpecificGameInfo() const { return true; }
public:
	CCustomLobby() { }
	CCustomLobby( class CClients *pClients, const string &szCfgFile );

	virtual bool Segment();

	friend struct SGetClientInfoFunc;
};


