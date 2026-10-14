#pragma once

#include "ServerClientInterface.h"

#include "Client_export.h"

class CLIENT_EXPORT CServerClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CServerClient )
	friend struct CPlayGameProcessorTestAccess;

	CPtr<class CNet> pNet;

	std::list<CPtr<class CPacketProcessor>> processors;
	std::list<CPtr<class CNetPacket>> packets;

	CPtr<class CPlayGameProcessor> pPlayGameProcessor;

	// debug pause
	bool bDebugPaused;
	std::list<CPtr<class CNetPacket>> pausedPackets;

	struct SPausedGamePacket
	{
		CPtr<class CNetPacket> pPacket;
		bool bBroadcast;

		SPausedGamePacket()
			: bBroadcast( false )
		{
		}

		SPausedGamePacket( class CNetPacket *_pPacket, bool _bBroadcast )
			: pPacket( _pPacket )
			, bBroadcast( _bBroadcast )
		{
		}
	};

	std::list<SPausedGamePacket> pausedGamePackets;
	int nTimeOut;

	void PushPacket( class CNetPacket *pPacket );
	void GetPacketsFromNet();

public:
	CServerClient() {}

	CServerClient( const char *pServerIPAddress, const int nNetGameVersion, const int nServerPort, const int _nTimeOut );
	~CServerClient() override;

	class CNetPacket *GetPacket() override;
	void SendPacket( class CNetPacket *pPacket ) override;
	void SendGamePacket( class CNetPacket *pPacket, bool bBroadcast ) override;

	void Segment() override;

	// debug functions
	void TogglePause( bool bPause );
	void TogglePauseServerConnection( bool bPause );
	void TogglePauseAcceptGamers( bool bPause );
	void TogglePauseConnectGamer( const int nGamer, bool bPause );
};
