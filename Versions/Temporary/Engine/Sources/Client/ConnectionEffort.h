#pragma once

#include "Server_Client_Common/PacketProcessor.h"

class CNet;

namespace NNet
{
struct ILinksManager;
}

enum EConnEffort
{
	ECE_NONE,
	ECE_SIMPLE,
	ECE_THROUGH_SERVER,
};

class CConnectionEffort : public CPacketProcessor
{
protected:
	void SendBreakThroughPacket( NNet::ILinksManager *pLinks, const std::string &szIP, const int nPort );

public:
	enum EState
	{
		ES_CONNECTING,
		ES_SUCCESS,
		ES_FAILED
	};

	virtual const EState GetState() const = 0;
	virtual const int GetClientServerID() const = 0;

	virtual struct IConnection *CreateConnection() const = 0;
	virtual CConnectionEffort *CreateNextEffort( const int nOurGameID ) = 0;

	// Optional setup information forwarded by the lobby before peer login.
	virtual void AddPeerLocalAddress( const std::string &szIP, int nPort ) {}
};

namespace NEffortsFactory
{
CConnectionEffort *
CreateFirstClientEffort( const int nMyServerID, const int nClientServerID, class CNet *pNet, const int nTimeOut );
CConnectionEffort *CreateFirstServerEffort( const int nMyServerID,
                                            const int nClientServerID,
                                            const std::string &szClientIP,
                                            const int nClientPort,
                                            CNet *pNet,
                                            CNet *pAcceptGamersNet,
                                            NNet::ILinksManager *pLinksManager );

void SetServerInfo( const std::string &szServerIP, const int nServerPort, const int nGameNetVersion );
} // namespace NEffortsFactory
