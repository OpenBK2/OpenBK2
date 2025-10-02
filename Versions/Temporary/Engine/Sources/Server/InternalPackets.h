#pragma once

#include "../Server_Client_Common/NetPacket.h"

class CGetLobbyClientsListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CGetLobbyClientsListPacket );
public:
	ZDATA
		int nLobbyID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLobbyID); return 0; }

	CGetLobbyClientsListPacket() : nLobbyID( 0 ) { }
	CGetLobbyClientsListPacket( const int _nLobbyID ) : nLobbyID( _nLobbyID ) { }
};

class CShowLobbyGamesPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CShowLobbyGamesPacket );
public:
	ZDATA
		int nLobbyID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLobbyID); return 0; }

	CShowLobbyGamesPacket() : nLobbyID( 0 ) { }
	CShowLobbyGamesPacket( const int _nLobbyID ) : nLobbyID( _nLobbyID ) { }
};


