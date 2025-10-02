#ifndef __MULTIPLAYER_TEST_NET_H__
#define __MULTIPLAYER_TEST_NET_H__

#pragma ONCE

#include "../Client/ServerClientInterface.h"
#include "../Server_Client_Common/PacketProcessor.h"

class CNetPacket;

// Пустой клиент, в идеале, не должен получать никаких сообщений, иначе выдает ассерты
class CEmptyServerClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CEmptyServerClient );
public:
	CEmptyServerClient() {}
	
	//{
	CNetPacket* GetPacket();
	void SendPacket( CNetPacket *pPacket );

	CNetPacket* GetGamePacket();
	void SendGamePacket( CNetPacket *pPacket, bool bBroadcast );

	void Segment();
	//}
};

/*class CServerClientWrapper : public IServerClient
{
private:
	//{ IServerClient
	void Segment() {}
	//}
};

class CPacketProcessorWrapper : public CPacketProcessorBase
{
private:
	//{ CPacketProcessorBase
	bool Segment() { return true; }
	//}
};

class CMPUITestServerClient : public CServerClientWrapper, public CPacketProcessorWrapper
{
	OBJECT_NOCOPY_METHODS( CMPUITestServerClient );

	struct SPlayer
	{
		int nID;
		wstring wszNick;

		int nGameID;
		bool bCreator;
		
		SPlayer() : nID( -1 ), nGameID( -1 ) {}
	};
	
	list< CPtr<CNetPacket> > serverPackets;
	list< CPtr<CNetPacket> > gamePackets;
	
	SPlayer player;
	list<SPlayer> opponents;
	int nFreeClientID;
	bool bLobby;
	int nFreeGameID;
private:
	void PushPacket( CNetPacket *pPacket );
	void PushGamePacket( CNetPacket *pPacket );

	//{
	// login
	bool OnRegisterPacket( class CRegisterPacket *pPacket );
	bool OnLoginPacket( class CLoginPacket *pPacket );

	// chat
	bool OnSendRoomChatPacket( class CChatPacket *pPacket );
	
	// lobby
	bool OnEnterGameRoomPacket( class CEnterLobbyPacket *pPacket );
	bool OnLeaveGameRoomPacket( class CLeaveLobbyPacket *pPacket );
	
	bool OnGetRoomClientsPacket( class CGetLobbyClientsPacket *pPacket );
	bool OnGetRoomGamesPacket( class CGetLobbyGamesPacket *pPacket );
	
	// game
	bool OnCreateGamePacket( class CCreateGamePacket *pPacket );
	bool OnConnectGamePacket( class CConnectGamePacket *pPacket );
	bool OnLeaveGamePacket( class CLeaveGamePacket *pPacket );
	bool OnKillGamePacket( class CKillGamePacket *pPacket );
	
	// game specific
	bool OnB2GameRoomPlayerPacket( class CB2GameRoomPlayerPacket *pPacket );
	//}
public:
	CMPUITestServerClient();
	
	//{ IServerClient
	CNetPacket* GetPacket();
	void SendPacket( CNetPacket *pPacket );

	CNetPacket* GetGamePacket();
	void SendGamePacket( CNetPacket *pPacket, bool bBroadcast );
	//}
	
	int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "do not serialize" ); return 0; }
};*/

#endif //__MULTIPLAYER_TEST_NET_H__

