#pragma once

#include "net/netaddress.h"
#include "ConnectionEffort.h"

class CNet;

namespace NNet
{
	class CGameLinksManager;
	class CNodeAddress;
}

class CSimpleClientEffort : public CConnectionEffort
{
	OBJECT_NOCOPY_METHODS( CSimpleClientEffort )

	int nMyServerID;
	int nClientServerID;
	int nGameNetVersion;
	CPtr<CNet> pNet;
	CPtr<CNet> pConnet2PlayersNet;

	enum ESubState { ECONNECT_SERVER, ECONNECT_CLIENT };
	ESubState eSubState;

	EState eState;
	CPtr<class CConnectServerProcessor> pConnectServerProcessor;
	CPtr<NNet::CGameLinksManager> pLinksManager;
	NNet::CNodeAddress serverAddress;

	float fStartConnectClientTime;
	float fPredBreakThroughSend;
	string szClientIP;
	int nClientPort;
	int nTimeOut;
public:
	CSimpleClientEffort() { }
	CSimpleClientEffort( const int nMyServerID, const int nClientServerID, CNet *pNet, 
											 const string &szServerIP, const int nServerPort, const int nGameNetVersion, const int nTimeOut );

	virtual const EState GetState() const { return eState; }
	virtual const int GetClientServerID() const { return nClientServerID; }

	virtual struct IConnection* CreateConnection() const;

	virtual bool Segment();
	void ProcessConnectServerPacket( class CConnectServerPacket *pPacket );

	//
	bool ProcessClientAccepted( class CGameConnectingClientAccepted *pPacket );
	bool ProcessClientConnectInfo( class CClientGameConnectInfo *pPacket );

	CConnectionEffort* CreateNextEffort( const int nOurGameID );

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
	string szClientIP;
	int nClientPort;

	float fStartConnectClientTime;
	float fPredBreakThroughSend;
public:
	CSimpleServerEffort() { }
	CSimpleServerEffort( const int nMyServerID, const int nClientServerID, 
											 const string &szClientIP, const int nClientPort, CNet *pNet, CNet *pAcceptGamersNet );

	virtual const EState GetState() const { return eState; }
	virtual const int GetClientServerID() const { return nClientServerID; }

	virtual struct IConnection* CreateConnection() const;
	virtual bool Segment();

	//
	bool ProcessClientIdentity( class CIndentityPacket *pPacket );

	CConnectionEffort* CreateNextEffort( const int nOurGameID );
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
	CThroughServerEffort() { }
	CThroughServerEffort( const int nClientServerID, const int nMyServerID, const int nOurGameID, CNet *pNet );

	virtual const EState GetState() const { return ES_SUCCESS; }
	virtual const int GetClientServerID() const { return nClientServerID; }

	virtual bool Segment() { return true; }

	virtual struct IConnection* CreateConnection() const;
	class CConnectionEffort* CreateNextEffort( const int nOurGameID ) { return 0; }
};


