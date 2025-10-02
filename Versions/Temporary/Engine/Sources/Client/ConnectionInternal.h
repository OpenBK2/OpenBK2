#pragma once

#include "Connection.h"

class CNet;

class CSimpleConnection : public IConnection
{
	OBJECT_NOCOPY_METHODS( CSimpleConnection );

	CPtr<CNet> pNet;
	bool bAsClient;
	int nClientServerID;
	int nClientLocalID;
	int nMyServerID;
	bool bConnectionTested;

	list<class CPtr<CNetPacket> > packets;
	CPtr<class CPacketProcessor> pFinishConnection;

	//
	int CheckTestConnection( CNetPacket *pPacket );
public:
	CSimpleConnection() { }
	CSimpleConnection( CNet *pNet, bool bAsClient, const int nClientServerID, const int nClientLocalID, const int nMyServerID );
	virtual bool IsAlive() const;

	virtual CNet* GetNet() { return pNet; }

	virtual void SendGamePacket( CNetPacket *pPacket );
	virtual bool ProcessPacketFromPlayersNet( CNetPacket *pPacket );
	virtual bool ProcessPacketFromServer( CNetPacket *pPacket ) { return false; }

	virtual CNetPacket* GetPacket();

	virtual void TogglePauseNet( const bool bPause ) const;
};

class CThroughServerConnection : public IConnection
{
	OBJECT_NOCOPY_METHODS( CThroughServerConnection );

	int nClientServerID;
	CPtr<CNet> pNet;
	bool bConnectionTested;

	list<class CPtr<CNetPacket> > packets;
public:
	CThroughServerConnection() { }
	CThroughServerConnection( const int nClientServerID, CNet *pNet );

	virtual CNet* GetNet() { return pNet; }

	virtual void SendGamePacket( CNetPacket *pPacket );
	virtual bool ProcessPacketFromPlayersNet( CNetPacket *pPacket ) { return false; }
	virtual bool ProcessPacketFromServer( CNetPacket *pPacket );

	virtual CNetPacket* GetPacket();

	virtual bool IsAlive() const { return true; }

	virtual void TogglePauseNet( const bool bPause ) const;
};


