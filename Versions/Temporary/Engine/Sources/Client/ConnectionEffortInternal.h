#pragma once

#include "ConnectionEffort.h"
#include "Net/NetAddress.h"

class CNet;

namespace NNet
{
class CGameLinksManager;
class CNodeAddress;
} // namespace NNet

class CSimpleClientEffort : public CConnectionEffort
{
	OBJECT_NOCOPY_METHODS( CSimpleClientEffort )

	int nMyServerID;
	int nClientServerID;
	int nGameNetVersion;
	CPtr<CNet> pNet;
	CPtr<CNet> pConnet2PlayersNet;

	enum ESubState
	{
		ECONNECT_SERVER,
		ECONNECT_CLIENT
	};

	ESubState eSubState;

	EState eState;
	CPtr<class CConnectServerProcessor> pConnectServerProcessor;
	CPtr<NNet::CGameLinksManager> pLinksManager;
	NNet::CNodeAddress serverAddress;

	float fStartConnectClientTime;
	float fPredBreakThroughSend;
	std::string szClientIP;
	int nClientPort;
	int nTimeOut;

public:
	CSimpleClientEffort() {}

	CSimpleClientEffort( const int nMyServerID,
	                     const int nClientServerID,
	                     CNet *pNet,
	                     const std::string &szServerIP,
	                     const int nServerPort,
	                     const int nGameNetVersion,
	                     const int nTimeOut );

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

	CConnectionEffort *CreateNextEffort( const int nOurGameID ) override;

	//
	bool ProcessClientIdentity( class CIndentityPacket *pPacket );
};

class CSimpleServerEffort : public CConnectionEffort
{
	OBJECT_NOCOPY_METHODS( CSimpleServerEffort )

	EState eState;

	int nMyServerID;
	int nClientServerID;
	int nClientLocalID;

	CPtr<CNet> pNet;
	CPtr<CNet> pAcceptGamersNet;
	std::string szClientIP;
	int nClientPort;

	float fStartConnectClientTime;
	float fPredBreakThroughSend;

public:
	CSimpleServerEffort() {}

	CSimpleServerEffort( const int nMyServerID,
	                     const int nClientServerID,
	                     const std::string &szClientIP,
	                     const int nClientPort,
	                     CNet *pNet,
	                     CNet *pAcceptGamersNet );

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
	bool ProcessClientIdentity( class CIndentityPacket *pPacket );

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
