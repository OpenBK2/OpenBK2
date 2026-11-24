#pragma once

#include "Net/NetAddress.h"
#include "PeerConnectionRoute.h"

class CNetPacket;

struct IConnection : public CObjectBase
{
public:
	virtual class CNet *GetNet() = 0;
	virtual void SendGamePacket( CNetPacket *pPacket ) = 0;

	virtual bool ProcessPacketFromPlayersNet( CNetPacket *pPacket ) = 0;
	virtual bool ProcessPacketFromServer( CNetPacket *pPacket ) = 0;
	virtual CNetPacket *GetPacket() = 0;

	virtual bool IsAlive() const = 0;

	virtual bool GetPeerAddress( NNet::CNodeAddress *address ) const
	{
		return false;
	}

	virtual EPeerConnectionRoute GetRoute() const
	{
		return EPeerConnectionRoute::Direct;
	}

	virtual void TogglePauseNet( const bool bPause ) const {}
};
