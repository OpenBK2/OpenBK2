#pragma once

#include "Net/NetDriver.h"
#include "ServerClientInterface.h"

#include "Client_export.h"

namespace NNet
{
class CNodeAddress;
}

class CLIENT_EXPORT CLANClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CLANClient );

	std::list<int> clients;
	CObj<NNet::IDriver> pNetDriver;
	std::list<CPtr<class CNetPacket>> packets;
	bool bTryingConnect;

public:
	CLANClient()
		: bTryingConnect( false )
	{
	}

	~CLANClient() override;

	// On failure returns 0 and, if pszError is given, puts there why (which call
	// failed, on which address, with the system error), for the player to see.
	static CLANClient *
	Create( const int nNetGameVersion, const int nGamePort, const int _nTimeOut, std::string *pszError = 0 );

	class CNetPacket *GetPacket() override;
	void SendPacket( class CNetPacket *pPacket ) override;
	void SendGamePacket( class CNetPacket *pPacket, bool bBroadcast ) override;

	void Segment() override;

	// Access to NetDriver
	bool GetGameInfo(
			int nIdx, NNet::CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, NNet::IDriver::SGameInfo *pGameInfo );
	void ConnectGame( const NNet::CNodeAddress &addr );
	void Kick( int nClient );
	void StartGame();
	void StartGameInfoSend( const NNet::IDriver::SGameInfo &gameInfo );
	void StopGameInfoSend();
	void StartNewPlayerAccept();
	void StopNewPlayerAccept();

	int GetOwnClientID()
	{
		return pNetDriver->GetSelfClientID();
	}
};
