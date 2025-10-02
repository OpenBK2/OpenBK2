#pragma once

#include "Server_Client_Common_export.h"


class CNetPacket;

namespace NNet
{
	interface ILinksManager;
	interface IDriver;
	class CNodeAddress;
	struct SNetDriverConsts;
}

class SERVER_CLIENT_COMMON_EXPORT CNet : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CNet );

	CPtr<NNet::IDriver> pNetDriver;
	int nApplicationID;
	int nServerPort;

	static float fTimeOut;

	bool bDebugNetPaused;
	list< CPtr<CNetPacket> > pausePackets;
	int nTimeOut;

	void SendZipped( const int nClientID, const CMemoryStream &memStream );
	CNetPacket* UnzipPacketIfNecessary( CNetPacket *pPacket );
public:
	CNet() { }
	CNet( const int nApplicationID, const int nServerPort, const int nTimeout );

	bool InitAsServer();
	bool InitAsClient();
	bool InitAsClient( NNet::ILinksManager *pLinksManager );
	bool InitAsServer( NNet::ILinksManager *pLinksManager );

	void StartGame();
	void StartNewPlayerAccept();
	void Kick( const int nClient );
	void ConnectGame( const NNet::CNodeAddress addr, const CMemoryStream &pwd );

	const int GetState() const;
	const int GetRejectReason() const;

	const string GetIP( const int nClientID ) const;
	const int GetPort( const int nClientID ) const;

	void SendPacket( CNetPacket *pPacket );
	CNetPacket* ReceivePacket();

	static const float GetTimeOut();
	static void SetTimeOut( const float fTimeOut );

// debug functions
	void DebugTogglePause( bool bPause );
};

