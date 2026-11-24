#pragma once

#include "ConnectionEffort.h"
#include "Net/NetAddress.h"
#include "PeerSetupPacket.h"

#include <cstdint>
#include <vector>

class CNet;

namespace NNet
{
class CGameLinksManager;
class CNodeAddress;
} // namespace NNet

class CSimpleClientEffort : public CConnectionEffort
{
	OBJECT_NOCOPY_METHODS( CSimpleClientEffort )
	friend struct CPlayGameProcessorTestAccess;

	int nMyServerID;
	int nClientServerID;
	int nGameNetVersion;
	CPtr<CNet> pNet;
	CPtr<CNet> pConnet2PlayersNet;

	enum ESubState
	{
		ECONNECT_SERVER,
		EWAIT_ADDRESSES,
		EWAIT_ATTEMPT,
		ECONNECT_CLIENT
	};

	ESubState eSubState;

	EState eState;
	CPtr<class CConnectServerProcessor> pConnectServerProcessor;
	CPtr<NNet::CGameLinksManager> pLinksManager;
	NNet::CNodeAddress serverAddress;

	uint32_t dwStartConnectClientTime;
	uint32_t dwPredBreakThroughSend;
	std::string szClientIP;
	int nClientPort;
	int nTimeOut;
	std::vector<NNet::CNodeAddress> peerLocalAddresses;
	std::vector<NNet::CNodeAddress> peerTargets;
	size_t nNextPeerTarget = 0;
	uint32_t dwPeerAttemptStart = 0;
	int nOurGameID = 0;
	uint32_t nSetupID = 0;
	int nAttempt = 0;
	int nAttemptLimit = 2;
	bool bRecovery = false;

	void StartPeerConnections();

public:
	CSimpleClientEffort() {}

	CSimpleClientEffort( const int nMyServerID,
	                     const int nClientServerID,
	                     CNet *pNet,
	                     const std::string &szServerIP,
	                     const int nServerPort,
	                     const int nGameNetVersion,
	                     const int nTimeOut,
	                     int nOurGameID = 0,
	                     uint32_t nSetupID = 0 );

	static constexpr uint32_t DIRECT_ATTEMPT_TIMEOUT = 15000;
	static constexpr int DIRECT_ATTEMPTS = 2;
	NNet::CGameLinksManager *GetLinks() const;

	const std::vector<NNet::CNodeAddress> &GetPeerTargets() const
	{
		return peerTargets;
	}

	void StartNextPeerConnection();
	void RetryEstablishedConnection();
	void AcceptAttempt( const CPeerSetupPacket &packet );
	bool IsAttemptExpired() const;

	int GetAttempt() const
	{
		return nAttempt;
	}

	int GetAttemptLimit() const
	{
		return nAttemptLimit;
	}

	const EState GetState() const override
	{
		return eState;
	}

	const int GetClientServerID() const override
	{
		return nClientServerID;
	}

	struct IConnection *CreateConnection() const override;

	bool Segment() override;
	void ProcessConnectServerPacket( class CConnectServerPacket *pPacket );

	//
	bool ProcessClientAccepted( class CGameConnectingClientAccepted *pPacket );
	bool ProcessClientConnectInfo( class CClientGameConnectInfo *pPacket );
	void AddPeerLocalAddress( const std::string &szIP, int nPort ) override;

	CConnectionEffort *CreateNextEffort( const int nOurGameID ) override;

	//
	bool ProcessClientIdentity( CPeerSetupPacket *pPacket );
};

class CSimpleServerEffort : public CConnectionEffort
{
	OBJECT_NOCOPY_METHODS( CSimpleServerEffort )
	friend struct CPlayGameProcessorTestAccess;

	EState eState;

	int nMyServerID;
	int nClientServerID;
	int nClientLocalID;

	CPtr<CNet> pNet;
	CPtr<CNet> pAcceptGamersNet;
	CPtr<NNet::ILinksManager> pLinksManager;
	std::string szClientIP;
	int nClientPort;

	uint32_t dwStartConnectClientTime;
	uint32_t dwPredBreakThroughSend;
	int nOurGameID = 0;
	uint32_t nSetupID = 0;
	int nAttempt = 0;

public:
	CSimpleServerEffort() {}

	CSimpleServerEffort( const int nMyServerID,
	                     const int nClientServerID,
	                     const std::string &szClientIP,
	                     const int nClientPort,
	                     CNet *pNet,
	                     CNet *pAcceptGamersNet,
	                     NNet::ILinksManager *pLinksManager,
	                     int nOurGameID = 0,
	                     uint32_t nSetupID = 0,
	                     int nAttempt = 0 );

	const EState GetState() const override
	{
		return eState;
	}

	const int GetClientServerID() const override
	{
		return nClientServerID;
	}

	struct IConnection *CreateConnection() const override;
	bool Segment() override;

	//
	bool ProcessClientIdentity( CPeerSetupPacket *pPacket );

	CConnectionEffort *CreateNextEffort( const int nOurGameID ) override;
};

class CThroughServerEffort : public CConnectionEffort
{
	OBJECT_NOCOPY_METHODS( CThroughServerEffort );

	int nClientServerID;
	int nMyServerID;
	int nOurGameID;
	CPtr<CNet> pNet;
	EState eState;

public:
	CThroughServerEffort() {}

	CThroughServerEffort( const int nClientServerID, const int nMyServerID, const int nOurGameID, CNet *pNet );

	const EState GetState() const override
	{
		return ES_SUCCESS;
	}

	const int GetClientServerID() const override
	{
		return nClientServerID;
	}

	bool Segment() override
	{
		return true;
	}

	struct IConnection *CreateConnection() const override;

	class CConnectionEffort *CreateNextEffort( const int nOurGameID ) override
	{
		return 0;
	}
};
