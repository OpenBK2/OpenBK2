#pragma once

#include "Client_export.h"


#include "ServerClientInterface.h"
#include "../Net/NetDriver.h"

namespace NNet
{
	class CNodeAddress;
}

class CLIENT_EXPORT CLANClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CLANClient );

	list<int> clients;
	CObj<NNet::IDriver> pNetDriver;
	list< CPtr<class CNetPacket> > packets;
	bool bTryingConnect;

public:
	CLANClient() : bTryingConnect( false ) { }
	~CLANClient();

	static CLANClient *Create( const int nNetGameVersion, const int nGamePort, const int _nTimeOut );

	class CNetPacket* GetPacket();
	void SendPacket( class CNetPacket *pPacket );
	void SendGamePacket( class CNetPacket *pPacket, bool bBroadcast );

	virtual void Segment();

	// Access to NetDriver
	bool GetGameInfo( int nIdx, NNet::CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, NNet::IDriver::SGameInfo *pGameInfo );
	void ConnectGame( const NNet::CNodeAddress &addr );
	void Kick( int nClient );
	void StartGame();
	void StartGameInfoSend( const NNet::IDriver::SGameInfo &gameInfo );
	void StopGameInfoSend();
	void StartNewPlayerAccept();
	void StopNewPlayerAccept();
	int GetOwnClientID() { return pNetDriver->GetSelfClientID(); }
};


