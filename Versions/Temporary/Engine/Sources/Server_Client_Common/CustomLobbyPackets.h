#pragma once

#include "CustomLobbyClientInfo.h"
#include "NetPacket.h"

/** incremental update of clients in the lobby */
class CCustomLobbyClientsPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CCustomLobbyClientsPacket )
public:
	ZDATA
		/** version of clients list */
		DWORD dwVersion;
		/** is it full update or not? */
		bool bFullUpdate;
		/** removed clinets */
		std::list<int> removed;
		/** added clients */
		std::list<SCustomLobbyClientInfo> added;
		/** changed clients */
		std::list<SCustomLobbyClientInfo> changed;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); f.Add(3,&bFullUpdate); f.Add(4,&removed); f.Add(5,&added); f.Add(6,&changed); return 0; }

	CCustomLobbyClientsPacket() { }
	CCustomLobbyClientsPacket( const int nClientID ) : CNetPacket( nClientID ) { }
};


