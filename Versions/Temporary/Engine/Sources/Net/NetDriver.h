#pragma once

#include "Net_export.h"

namespace NNet
{
typedef unsigned int APPLICATION_ID;

struct SNetDriverConsts
{
	// время, после которого drop клиента
	int fTimeout;
	// время, после которого удаление сервера из serverslist
	float fServerListTimeout;

	SNetDriverConsts() : fTimeout( 60 ), fServerListTimeout( 20 ) {}
	SNetDriverConsts( const int _nTimeOut ) : fTimeout( _nTimeOut ), fServerListTimeout( 20 ) { }
};

// abstraction from SOCKET
struct ILinksManager : public CObjectBase
{
	virtual bool MakeBroadcastAddr( class CNodeAddress *pRes, int nPort ) const = 0;
	virtual bool IsLocalAddr( const CNodeAddress &test ) const = 0;
	virtual bool Send( const CNodeAddress &dst, CMemoryStream &pkt ) const = 0;
	virtual bool Recv( class CNodeAddress *pSrc, CMemoryStream *pPkt ) const = 0;
	virtual bool GetSelfAddress( class  CNodeAddressSet *pRes ) const = 0;
};

struct IDriver : public CObjectBase
{
	enum EState
	{
		INACTIVE,
		ACTIVE,
		CONNECTING
	};
	enum EReject
	{
		NONE,
		TIMEOUT,
		BANNED,
		WRONG_VERSION,
		MAXPLAYERS_REACHED,
		PASSWORD_FAILED,
		ALREADY_CONNECTED,
		FORBIDDEN
	};
	enum EMessage
	{
		NEW_CLIENT,
		REMOVE_CLIENT,
		DIRECT,
		BROADCAST,
		SERVER_DEAD,
		KICKED,
	};
	enum EServerGameMode
	{
		ESGM_WAIT,						// waiting for players to join
		ESGM_SETTINGS,				// players are determining game parameters, no joining allowed
		ESGM_CLOSEDPLAYING,		// game is in progress, no joining allowed
		ESGM_OPENPLAYING,			// game is in progress, players may still join
		ESGM_DEBRIEFING,			// game is over, stats/info is being shown, no joining allowed
		ESGM_EXITING,					// server is shutting down, remove from server list
	};

	struct SGameInfo
	{
		ZDATA
		std::wstring wszServerName;
		long nHostPort;
		std::wstring wszMapName;
		std::string szGameType;
		int nCurPlayers, nMaxPlayers;
		EServerGameMode eGameMode;

		bool bPasswordRequired;
		
		std::string szModName;
		std::string szModVersion;
		
		CMemoryStream gameSettings;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszServerName); f.Add(3,&nHostPort); f.Add(4,&wszMapName); f.Add(5,&szGameType); f.Add(6,&nCurPlayers); f.Add(7,&nMaxPlayers); f.Add(8,&eGameMode); f.Add(9,&bPasswordRequired); f.Add(10,&szModName); f.Add(11,&szModVersion); f.Add(12,&gameSettings); return 0; }

		SGameInfo() 
			: wszServerName( L"" ), nHostPort( 0 ), wszMapName( L"" ), szGameType( "" ), 
				nCurPlayers( 0 ), nMaxPlayers( 0 ), eGameMode( ESGM_SETTINGS ), bPasswordRequired( false ),
				szModName( "" ), szModVersion( "" ) { }
	};
	//
	virtual void Init( const APPLICATION_ID nApplicationID, const int nGamePort, bool bClientOnly, ILinksManager *pLinksManager ) = 0;
	// get current state (active/inactive/connecting)
	virtual EState GetState() const = 0;
	// get reject reason (then )
	virtual EReject GetRejectReason() const = 0;
	// connect to the game with particular address
	virtual void ConnectGame( const CNodeAddress &addr, const CMemoryStream &pkt ) = 0;
	// start game (server)
	virtual void StartGame() = 0;
	// start sending game info (for server)
	virtual void StartGameInfoSend( const SGameInfo &gameInfo ) = 0;
	// stop sending game info (for server)
	virtual void StopGameInfoSend() = 0;
	// start accepting new players (for server)
	virtual void StartNewPlayerAccept() = 0;
	// stop accepting new players (for server)
	virtual void StopNewPlayerAccept() = 0;
	// get game info (for client)
	virtual bool GetGameInfo( int nIdx, CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, SGameInfo *pGameInfo ) = 0;
	// refresh servers list ( for client )
	virtual void RefreshServersList() = 0;
	// send broadcast message for all
	virtual bool SendBroadcast( const CMemoryStream &_pkt ) = 0;
	// send direct message for client 'nClient'
	virtual bool SendDirect( int nClient, const CMemoryStream &_pkt ) = 0;
	// kick player 'nClient'
	virtual void Kick( int nClient ) = 0;
	// get next message
	virtual bool GetMessage( EMessage *pMsg, int *pClientID, std::vector<int> *pReceived, CMemoryStream *pPkt ) = 0;
	// ping of the client, -1 if client doesn't exist
	virtual const float GetPing( const int nClientID ) = 0;
	// time since last message was received from this client
	virtual const float GetTimeSinceLastRecv( const int nClientID ) = 0;
	// step net driver
	virtual void Step() = 0;
	// nClientID of this host
	virtual int GetSelfClientID() = 0;

	// for debug of lagging net
	virtual void PauseNet() {}
	virtual void UnpauseNet() {}
	virtual void SetLag( const NTimer::STime period ) {}

	virtual const std::string GetIP( const int nClientID ) = 0;
	virtual const int GetPort( const int nClientID ) = 0;
};

NET_EXPORT IDriver* CreateNetDriver( const SNetDriverConsts &consts, bool bIsBroadcast = true );

NET_EXPORT ILinksManager* CreateClientLinksManager();
NET_EXPORT ILinksManager* CreateServerLinksManager( const int nPort );

NET_EXPORT void GetNOPStream( CMemoryStream *pNOP );

}

