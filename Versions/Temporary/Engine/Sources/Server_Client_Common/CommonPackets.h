#pragma once

#include "Server_Client_Common/CommonClientState.h"
#include "ChatPackets.h"

class CNetNewClient : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CNetNewClient );

	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }
};

class CNetRemoveClient : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CNetRemoveClient );
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CNetRemoveClient() { }
};

/**
* Connection to server result
*/
class CConnectServerPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CConnectServerPacket )
public:
	enum EConnectionState { ECS_SUCCESS, ECS_FAILED };
	enum ERejectReason
	{
		ERR_UNKNOWN,
		ERR_CANT_RESOLVE_ADDRESS,
		ERR_CONNECTION_LOST,
		ERR_BAD_NICK,
		ERR_WRONG_CD_KEY,
		ERR_ALREADY_REGISTERED,
		ERR_ALREADY_ONLINE,
		ERR_NOT_REGISTERED,
		ERR_BANNED_NICK,
		ERR_BANNED_CDKEY,
		ERR_WRONG_PASSWORD,
		ERR_KICKED,
		ERR_LOGIN_TIMEOUT,
		ERR_WRONG_NET_VERSION,
		ERR_CRITICAL_BUSY
	};

	ZDATA
		EConnectionState eConnectionState;
		ERejectReason eRejectReason;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eConnectionState); f.Add(3,&eRejectReason); return 0; }

	CConnectServerPacket() { }
	CConnectServerPacket( const EConnectionState _eConnectionState, const ERejectReason _eRejectReason )
		: eConnectionState( _eConnectionState ), eRejectReason( _eRejectReason ), CNetPacket( 0 ) { }
};

/**
* Packet with my id.
*/
class CMyIDPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CMyIDPacket )
public:
	ZDATA
		int nMyID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nMyID); return 0; }

	CMyIDPacket() { }
	CMyIDPacket( const int nClientID )
		: CNetPacket( nClientID ), nMyID( nClientID ) { }
};

class CSystemBroadcastPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CSystemBroadcastPacket )
public:
	ZDATA
		wstring wszText;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszText); return 0; }

	CSystemBroadcastPacket() { }
	CSystemBroadcastPacket( const int nClientID, const wstring &_wszText )
		: CNetPacket( nClientID ), wszText( _wszText ) { }
};

/**
* Packet to enter to lobby
*/
class CEnterLobbyPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CEnterLobbyPacket )
public:
	ZDATA
		int nLobbyID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLobbyID); return 0; }

	CEnterLobbyPacket() { }
	CEnterLobbyPacket( const int nClientID, const int _nLobbyID ) 
		: CNetPacket( nClientID ), nLobbyID( _nLobbyID ) { }
};

/**
* Packet to leave the lobby
*/
class CLeaveLobbyPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLeaveLobbyPacket )
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CLeaveLobbyPacket() { }
	CLeaveLobbyPacket( const int nClientID ) : CNetPacket( nClientID ) { }
};

/** set new client state */
class CCommonClientStatePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CCommonClientStatePacket )
public:
	ZDATA
		ECommonClientState eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); return 0; }

	CCommonClientStatePacket() { }
	CCommonClientStatePacket( const int nClientID, const ECommonClientState _eState )
		: CNetPacket( nClientID ), eState( _eState ) { }
};

/** get lobby clients */
class CGetLobbyClientsPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGetLobbyClientsPacket )
public:
	ZDATA
		/** my latest received version */
		DWORD dwVersion;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); return 0; }

	CGetLobbyClientsPacket() { }
	CGetLobbyClientsPacket( const int nClientID, const DWORD _dwVersion )
		: CNetPacket( nClientID ), dwVersion( _dwVersion ) { }
};

/** get lobby games */
class CGetLobbyGamesPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGetLobbyGamesPacket )
public:
	ZDATA
		/** my latest received version */
		DWORD dwVersion;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); return 0; }

	CGetLobbyGamesPacket() { }
	CGetLobbyGamesPacket( const int nClientID, const DWORD _dwVersion )
		: CNetPacket( nClientID ), dwVersion( _dwVersion ) { }
};


// packets for internal use!

class CDirectPacketToClient : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CDirectPacketToClient )
public:
	ZDATA
		int nClient;
		CPtr<CNetPacket> pPacket;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClient); f.Add(3,&pPacket); return 0; }

	CDirectPacketToClient() { }
	CDirectPacketToClient( const int nClient, const int nClientTo, CNetPacket *_pPacket )
		: CNetPacket( nClient ), nClient( nClientTo ), pPacket( _pPacket ) { }
};

class CTestDirectPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CTestDirectPacket );
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CTestDirectPacket() { }
	CTestDirectPacket( const int nClientID ) : CNetPacket( nClientID ) { }
};





