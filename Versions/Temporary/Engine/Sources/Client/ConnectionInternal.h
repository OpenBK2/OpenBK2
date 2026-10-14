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

	std::list<class CPtr<CNetPacket>> packets;
	CPtr<class CPacketProcessor> pFinishConnection;

	//
	int CheckTestConnection( CNetPacket *pPacket );

public:
	CSimpleConnection() {}

	CSimpleConnection(
			CNet *pNet, bool bAsClient, const int nClientServerID, const int nClientLocalID, const int nMyServerID );
	bool IsAlive() const override;

	CNet *GetNet() override
	{
		return pNet;
	}

	void SendGamePacket( CNetPacket *pPacket ) override;
	bool ProcessPacketFromPlayersNet( CNetPacket *pPacket ) override;

	bool ProcessPacketFromServer( CNetPacket *pPacket ) override
	{
		return false;
	}

	CNetPacket *GetPacket() override;

	void TogglePauseNet( const bool bPause ) const override;
};

class CThroughServerConnection : public IConnection
{
	OBJECT_NOCOPY_METHODS( CThroughServerConnection );
	friend struct CPlayGameProcessorTestAccess;

	int nClientServerID;
	CPtr<CNet> pNet;
	bool bConnectionTested;

	std::list<class CPtr<CNetPacket>> packets;

public:
	CThroughServerConnection() {}

	CThroughServerConnection( const int nClientServerID, CNet *pNet );

	CNet *GetNet() override
	{
		return pNet;
	}

	void SendGamePacket( CNetPacket *pPacket ) override;

	bool ProcessPacketFromPlayersNet( CNetPacket *pPacket ) override
	{
		return false;
	}

	bool ProcessPacketFromServer( CNetPacket *pPacket ) override;

	CNetPacket *GetPacket() override;

	bool IsAlive() const override
	{
		return true;
	}

	void TogglePauseNet( const bool bPause ) const override;
};
