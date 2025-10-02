#pragma once

#include "GameInfo.h"
#include "Server_Client_Common/NetPacket.h"

/** create game */
class CCreateGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CCreateGamePacket );
public:
	ZDATA
		CPtr<SGameInfo> pInitialGameInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pInitialGameInfo); return 0; }

	CCreateGamePacket() { }
	CCreateGamePacket( const int nClientID ) : CNetPacket( nClientID ), pInitialGameInfo( 0 ) { }
	CCreateGamePacket( const int nClientID, SGameInfo *_pInitialGameInfo ) : 
		CNetPacket( nClientID ), pInitialGameInfo( _pInitialGameInfo ) {}
};

/** hearbeat packet */
class CGameHeartBeatPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameHeartBeatPacket )
public:
	ZDATA
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); return 0; }

	CGameHeartBeatPacket() { }
	CGameHeartBeatPacket( const int nClientID, const int _nGameID )
		: CNetPacket( nClientID ), nGameID( _nGameID ) { }
};

class CGameStartLoadingPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameStartLoadingPacket )
public:
	ZDATA
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); return 0; }

	CGameStartLoadingPacket() {}
	CGameStartLoadingPacket( const int nClientID, const int _nGameID )
		: CNetPacket( nClientID ), nGameID( _nGameID ) {}
};

/** kill the game */
class CKillGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CKillGamePacket )
public:
	ZDATA
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); return 0; }

	CKillGamePacket() { }
	CKillGamePacket( const int nClientID, const int _nGameID )
		: CNetPacket( nClientID ), nGameID( _nGameID ) { }
};

/** connect to game */
class CConnectGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CConnectGamePacket )
public:
	ZDATA
		int nGameID;
		string szPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&szPassword); return 0; }

	CConnectGamePacket() { }
	CConnectGamePacket( const int nClientID, const int _nGameID, const string &_szPassword )
		: CNetPacket( nClientID ), nGameID( _nGameID ), szPassword( _szPassword ) { }
};

/** id of game we connected to */
class CConnectedGameID : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CConnectedGameID );
public:
	ZDATA
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); return 0; }

	CConnectedGameID() { }
	CConnectedGameID( const int nClientID, const int _nGameID )
		: CNetPacket( nClientID ), nGameID( _nGameID ) { }
};

/** leave the game */
class CLeaveGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLeaveGamePacket )
public:
	ZDATA
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); return 0; }

	CLeaveGamePacket() { }
	CLeaveGamePacket( const int nClientID, const int _nGameID )
		: CNetPacket( nClientID ), nGameID( _nGameID ) { }
};

/** connect with game failed */
class CConnectGameFailed : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CConnectGameFailed )
public:
	enum EReason
	{
		ER_UNKNOWN,
		ER_WRONG_PASSWORD,
		ER_MAX_PLAYERS_REACHED,
		ER_GAME_CLOSE_TO_CONNECT,
	};

	ZDATA
		EReason eReason;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eReason); return 0; }

	CConnectGameFailed() : eReason( ER_UNKNOWN ) { }
	CConnectGameFailed( const int nClientID, const EReason _eReason )
		: CNetPacket( nClientID ), eReason( _eReason ) { }
};

/** game was killed */
class CGameKilled : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameKilled );
public:
	ZDATA
		int nGame;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGame); return 0; }

	CGameKilled() { }
	CGameKilled( const int nClientID, const int _nGame )
		: CNetPacket( nClientID ), nGame( _nGame ) { }
};

/** client left the game */
class CGameClientRemoved : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameClientRemoved )
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CGameClientRemoved() { }
	CGameClientRemoved( const int nClient ) : CNetPacket( nClient ) { }
};

/** new client connected */
class CNewGameClient : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CNewGameClient )
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CNewGameClient() { }
	CNewGameClient( const int nClient ) : CNetPacket( nClient ) { }
};

/** kick client from game*/
class CGameKickClient : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameKickClient )
public:
	ZDATA
		int nKicked;
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nKicked); f.Add(3,&nGameID); return 0; }

	CGameKickClient() { }
	CGameKickClient( const int nClientID, const int _nGameID, const int _nKicked )
		: CNetPacket( nClientID ), nKicked( _nKicked ), nGameID( _nGameID ) { }
};

/** this client was kicked */
class CGameClientWasKicked : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameClientWasKicked );
public:
	ZDATA
		int nKicked;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nKicked); return 0; }

	CGameClientWasKicked() { }
	CGameClientWasKicked( const int nClientID, const int _nKicked )
		: CNetPacket( nClientID ), nKicked( _nKicked ) { }
};

/** info to server that game info was updated */
class CUpdateGameInfo : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CUpdateGameInfo )
public:
	ZDATA
	SGameInfo gameInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&gameInfo); return 0; }

	CUpdateGameInfo() { }
	CUpdateGameInfo( const int nClientID, const SGameInfo &_gameInfo )
		: CNetPacket( nClientID ), gameInfo( _gameInfo ) { }
};

/** specific internal info about game, sent to connected users */
class CSpecificGameInfo : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CSpecificGameInfo )
public:
	ZDATA
		int nGameID;
		CPtr<CNetPacket> pInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&pInfo); return 0; }

	CSpecificGameInfo() { }
	CSpecificGameInfo( const int nClientID, const int _nGameID, CNetPacket *_pInfo )
		: CNetPacket( nClientID ), nGameID( _nGameID ), pInfo( _pInfo ) { }
};

/** incremental update of clients in the lobby */
class CLobbyGamesPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLobbyGamesPacket )
public:
	ZDATA
		DWORD dwVersion;
		bool bFullUpdate;
		list<int> removed;
		list<SGameInfo> added;
		list<SGameInfo> changed;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); f.Add(3,&bFullUpdate); f.Add(4,&removed); f.Add(5,&added); f.Add(6,&changed); return 0; }

	CLobbyGamesPacket() { }
	CLobbyGamesPacket( const int nClientID ) : CNetPacket( nClientID ) { }
};

// game room debug support
class CGameTestBroadcastMsg : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameTestBroadcastMsg )
public:
	ZDATA
		int nNumber;
	string szStr;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nNumber); f.Add(3,&szStr); return 0; }

	CGameTestBroadcastMsg() { }
	CGameTestBroadcastMsg( const int _nNumber, const string &_szStr )
		: nNumber( _nNumber ), szStr( _szStr ) { }
};

// game room debug support
class CGameTestDirectMsg : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameTestDirectMsg )
public:
	ZDATA
		int nNumber;
	string szStr;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nNumber); f.Add(3,&szStr); return 0; }

	CGameTestDirectMsg() { }
	CGameTestDirectMsg( const int nClientID, const int _nNumber, const string &_szStr )
		: CNetPacket( nClientID ), nNumber( _nNumber ), szStr( _szStr ) { }
};


// internal packets
/** connection opened, client nServerID ready to receive other gamers connections */
class CNewGameConnectingClient : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CNewGameConnectingClient )
public:
	ZDATA
		int nServerID;
		int nConnection;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nServerID); f.Add(3,&nConnection); return 0; }

	CNewGameConnectingClient() { }
	CNewGameConnectingClient( const int nClientID, const int _nServerID, const int _nConnection )
		: CNetPacket( nClientID ), nServerID( _nServerID ), nConnection( _nConnection ) { }
};

/** answer to connection opened */
class CGameConnectingClientAccepted : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameConnectingClientAccepted )
public:
	ZDATA
		int nConnection;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nConnection); return 0; }

	CGameConnectingClientAccepted() { }
	CGameConnectingClientAccepted( const int nClientID, const int _nConnection )
		: CNetPacket( nClientID ), nConnection( _nConnection ) { }
};

/** info to server, client want to connect to client nClient2Connect through connection nMyConnect */
class CWant2Connect2Client : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CWant2Connect2Client )
public:
	ZDATA
		int nClient2Connect;
		int nMyConnect;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClient2Connect); f.Add(3,&nMyConnect); return 0; }

	CWant2Connect2Client() { }
	CWant2Connect2Client( const int nClientID, const int _nClient2Connect, const int _nMyConnect )
		: CNetPacket( nClientID ), nClient2Connect( _nClient2Connect ), nMyConnect( _nMyConnect ) { }
};

/** answer to connect game, passing list of game clients */
class CAnswerConnectGame : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CAnswerConnectGame )
public:
	ZDATA
		list<int> clients;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&clients); return 0; }

	CAnswerConnectGame() { }
	CAnswerConnectGame( const int nClientID ) : CNetPacket( nClientID ) { }
};

/** info from server about asked client, goes to connection effort */
class CClientGameConnectInfo : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CClientGameConnectInfo )
public:
	ZDATA
		string szIP;
		int nPort;
		int nClient2Connect;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szIP); f.Add(3,&nPort); f.Add(4,&nClient2Connect); return 0; }

	CClientGameConnectInfo() { }
	CClientGameConnectInfo( const int nClientID, const string &_szIP, const int _nPort, const int _nClient2Connect )
		: CNetPacket( nClientID ), szIP( _szIP ), nPort( _nPort ), nClient2Connect( _nClient2Connect ) { }
};

/** info from server about client which want to connect, creates new connection to the client */
class CClientWantToConnect : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CClientWantToConnect )
public:
	ZDATA
		string szIP;
		int nPort;
		int nWantedClient;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szIP); f.Add(3,&nPort); f.Add(4,&nWantedClient); return 0; }

	CClientWantToConnect() { }
	CClientWantToConnect( const int nClientID, const int _nWantedClient, const string &_szIP, const int _nPort )
		: CNetPacket( nClientID ), nWantedClient( _nWantedClient ), szIP( _szIP ), nPort( _nPort ) { }
};

/** Info about client, who just connected to us (through the pAcceptGamersNet connection)*/
class CIndentityPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CIndentityPacket )
public:
	ZDATA
		int nServerID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nServerID); return 0; }

	CIndentityPacket() { }
	CIndentityPacket( const int nClientID, const int _nServerID )
		: CNetPacket( nClientID ), nServerID( _nServerID ) { }
};

class CThroughServerConnectionPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CThroughServerConnectionPacket )
public:
	ZDATA
		int nClientWith;
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClientWith); f.Add(3,&nGameID); return 0; }

	CThroughServerConnectionPacket() { }
	CThroughServerConnectionPacket( const int nClientID, const int _nClientWith, const int _nGameID )
		: CNetPacket( nClientID ), nClientWith( _nClientWith ), nGameID( _nGameID ) { }
};

class CGameClientDead : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGameClientDead )
public:
	ZDATA
		int nDeadClient;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nDeadClient); return 0; }

	CGameClientDead() { }
	CGameClientDead( const int nClientID, const int _nDeadClient )
		: CNetPacket( nClientID ), nDeadClient( _nDeadClient ) { }
};

class CThroughServerGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CThroughServerGamePacket );
public:
	ZDATA
		int nClient;
		CPtr<CNetPacket> pGamePacket;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClient); f.Add(3,&pGamePacket); return 0; }

	CThroughServerGamePacket() { }
	CThroughServerGamePacket( const int nClientID, const int nClientTo, CNetPacket *_pGamePacket )
		: CNetPacket( nClientID ), nClient( nClientTo ), pGamePacket( _pGamePacket ) { }
};

class CPingPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CPingPacket );
public:
	ZDATA
		int nSendTime;
		int nFromID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSendTime); f.Add(3,&nFromID); return 0; }

	CPingPacket() { }
	CPingPacket( const int nClientID, const int _nFromID, const int _nSendTime ) 
		: CNetPacket( nClientID ), nFromID( _nFromID ), nSendTime( _nSendTime ) {}
};


