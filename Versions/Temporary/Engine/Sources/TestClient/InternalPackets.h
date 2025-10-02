#pragma once

#include "../Server_Client_Common/NetPacket.h"

class CEnteredLobby : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CEnteredLobby );
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CEnteredLobby() { }
};

class CTestShowGameClients : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CTestShowGameClients );
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }
};

class CTestSpecGameInfo : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CTestSpecGameInfo );
public:
	ZDATA
		string szMapName;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szMapName); return 0; }

	CTestSpecGameInfo() { }
	CTestSpecGameInfo( const string &_szMapName ) : szMapName( _szMapName ) { }
};


