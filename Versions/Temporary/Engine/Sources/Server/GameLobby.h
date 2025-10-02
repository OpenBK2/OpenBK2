#pragma once

#include "UpdatableList.h"
#include "Server_Client_Common/GameInfo.h"
#include "Server_Client_Common/PacketProcessor.h"


class CGameLobby : public CPacketProcessor
{
	CUpdatableList clientsVersions;
	CUpdatableList gamesVersions;

	hash_set<int> lobbyClients;
	CPtr<class CClients> pClients;
	hash_map< int, hash_set<int> > throughServerClients;

	DWORD dwGameTimeOut;
	DWORD dwGameLoadingTimeOut;
	string szCfgFile;

	struct SLobbyGameInfo
	{
		UINT64 nLastGameHeartBeat;
		hash_set<int> clients;
		SGameInfo gameInfo;
		CPtr<class CSpecificGameInfo> pSpecificGameInfo;
	};
	hash_map<int, SLobbyGameInfo> lobbyGames;
	int nGamesCounter;

	struct SGetGameInfoFunc
	{
		CGameLobby *pThis;
		SGetGameInfoFunc( CGameLobby *_pThis ) : pThis( _pThis ) { }
		const bool operator()( const int nGameID, struct SGameInfo *pInfo ) const 
		{ return pThis->GetGameInfo( nGameID, pInfo ); }
	};

	//
	void ClientEntered( const int nID );
	void ClientLeaved( const int nID );
	const bool GetGameInfo( const int nGameID, SGameInfo *pInfo ) const;
	bool GetGameClients( const int nGame, hash_set<int> *pClients );
	void SetClientGameID( const int nClientID, const int nGameID );
	void EraseGameClients( const int nGameID );
	void SetClientLobbyID( const int nClientID, const BYTE cLobbyID );
	void InformThroughServerClients( const int nLeftGameClient );
	void RemoveLobbyClient( const int nClientID ); 
	bool KillGame( const int nGameID );
protected:
	bool IsLobbyClient( const int nID ) const { return lobbyClients.find( nID ) != lobbyClients.end(); }
	class CClients* GetClients() const { return pClients; }

	void GameInfoChanged( const int nGameID );

	template<typename TInPacket, typename TOutPacket, class TGetInfo>
		inline void GetUpdate( TInPacket *pInPacket, TOutPacket *pOutPacket, TGetInfo &GetInfoFunc, bool bClientsUpdate );

	//
	virtual const int GetLobbyID() const = 0;
	virtual void ClientEnterToLobby( const int nClientID ) = 0;
	virtual void ClientLeaveLobby( const int nClientID ) = 0;
	virtual void GetLobbyClients( class CGetLobbyClientsPacket *pPacket ) = 0;
	virtual void GameDead( const int nGameID ) {}
	virtual int CreateGame( const int nMaxPlayers );
	virtual bool PlayerCanKickPlayer() const = 0;
	virtual bool PlayerCanSeeGamesList() const = 0;
	virtual bool PlayerCanUpdateGameInfo() const = 0;
	virtual bool PlayerNeedSpecificGameInfo() const = 0;
public:
	CGameLobby() : nGamesCounter( 0 ) { }
	CGameLobby( class CClients *pClients, const string &szCfgFile );

	virtual bool Segment();
	bool CanBePaused() { return false; }

	virtual void ReloadConfig();

	//
	bool ProcessCommonClientStatePacket( class CCommonClientStatePacket *pPacket );
	bool ProcessGameHeartBeatPacket( class CGameHeartBeatPacket *pPacket );
	bool ProcessGameStartLoadingPacket( class CGameStartLoadingPacket *pPacket );
	bool ProcessConnectGamePacket( class CConnectGamePacket *pPacket );
	bool ProcessLeaveGame( class CLeaveGamePacket *pPacket );
	bool ProcessWant2Connect2Client( class CWant2Connect2Client *pPacket );
	bool ProcessKickClient( class CGameKickClient *pPacket );

	bool ProcessEnterLobby( class CEnterLobbyPacket *pPacket );
	bool ProcessLeaveLobby( class CLeaveLobbyPacket *pPacket );
	bool ProcessRemoveClient( class CNetRemoveClient *pNetRemoveClient );
	bool ProcessCreateGame( class CCreateGamePacket *pPacket );
	bool ProcessKillGame( class CKillGamePacket *pPacket );
	bool ProcessGetLobbyGames( class CGetLobbyGamesPacket *pPacket );
	bool ProcessGetLobbyClients( class CGetLobbyClientsPacket *pPacket );
	bool ProcessUpdateGame( class CUpdateGameInfo *pPacket );
	bool ProcessSpecificGameInfo( class CSpecificGameInfo *pPacket );
	bool ProcessThroughServerConnection( class CThroughServerConnectionPacket *pPacket );

	friend struct SGetGameInfoFunc;

	// for testing
	bool ProcessGetLobbyClientsListPacket( class CGetLobbyClientsListPacket *pPacket );
	// for testing
	bool ProcessShowLobbyGames( class CShowLobbyGamesPacket *pPacket );

};



template<typename TInPacket, typename TOutPacket, class TGetInfo>
inline void CGameLobby::GetUpdate( TInPacket *pInPacket, TOutPacket*, TGetInfo &GetInfoFunc, bool bClientsUpdate )
{
	SUpdateInfo updateInfo;
	if ( bClientsUpdate )
		clientsVersions.GetDiff( 0, pInPacket->dwVersion, &updateInfo );
	//		clientsVersions.GetDiff( pInPacket->nClientID, pInPacket->dwVersion, &updateInfo );
	else
		gamesVersions.GetDiff( -1, pInPacket->dwVersion, &updateInfo );

	TOutPacket *pOutPacket = new TOutPacket( pInPacket->nClientID );
	pOutPacket->dwVersion = updateInfo.dwVersion;
	pOutPacket->bFullUpdate = updateInfo.bFullUpdate;

	for ( list<int>::iterator iter = updateInfo.added.begin(); iter != updateInfo.added.end(); ++iter )
	{
		pOutPacket->added.insert( pOutPacket->added.end() );
		if ( !GetInfoFunc( *iter, &(pOutPacket->added.back()) ) )
			pOutPacket->added.pop_back();
	}

	if ( !pOutPacket->bFullUpdate )
	{
		pOutPacket->removed.splice( pOutPacket->removed.begin(), updateInfo.removed );
		for ( list<int>::iterator iter = updateInfo.changed.begin(); iter != updateInfo.changed.end(); ++iter )
		{
			pOutPacket->changed.insert( pOutPacket->changed.end() );
			if ( !GetInfoFunc( *iter, &(pOutPacket->changed.back()) ) )
				pOutPacket->changed.pop_back();
		}
	}

	PushPacket( pOutPacket );
}


