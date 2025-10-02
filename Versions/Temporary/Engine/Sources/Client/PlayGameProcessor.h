#pragma once

#include "LinksManager.h"
#include "../Server_Client_Common/PacketProcessor.h"

class CNet;
class CNetPacket;
interface IConnection;
class CConnectionEffort;
class CConnectServerProcessor;

class CPlayGameProcessor : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CPlayGameProcessor )

	CPtr<CNet> pNet;
	CPtr<CNet> pAcceptGamersNet;

	typedef hash_map<int, CPtr<IConnection> > TConnections;
	TConnections connections;

	typedef hash_map< int, CPtr<CConnectionEffort> > TConnectionEfforts;
	TConnectionEfforts connectionEfforts;

	int nMyServerID;

	string szServerIP;
	int nNetGameVersion;
	int nServerPort;

	CPtr<CNetPacket> pConnectGamePacket;
	CPtr<CConnectServerProcessor> pConnectServerProcessor;

	struct SWaitingPacket
	{
		CPtr<CNetPacket> pPacket;
		DWORD dwStartTime;

		SWaitingPacket() : dwStartTime( 0 ) { }
		SWaitingPacket( CNetPacket *_pPacket, const DWORD _dwStartTime )
			: pPacket( _pPacket ), dwStartTime( _dwStartTime ) { }
	};
	list<SWaitingPacket> waitingPackets;

	CPtr<NNet::CGameLinksManager> pLinksManager;
	NNet::CNodeAddress serverAddress;
	int nOurGameID;
	int nTimeOut;

	//
	void StartServerConnect();
	void ProcessConnectServerProcessor();
	void ProcessAcceptingGamersPackets();
	void ProcessEfforts();
	void Clear();
	bool CreateConnectionEffort( CConnectionEffort *pEffort );
public:
	CPlayGameProcessor() { }
	CPlayGameProcessor( CNet *pNet, const char* pszServerIP, const int nNetGameVersion, const int nServerPort, const int nTimeOut );

	virtual bool Segment();
	void SendGamePacket( CNetPacket *pPacket, bool bBroadcast );
	void ProcessConnectServerPacket( class CConnectServerPacket *pPacket );

	virtual bool ProcessPacket( CNetPacket *pPacket );

	//
	bool ProcessCreateGame( class CCreateGamePacket *pPacket );
	bool ProcessConnectGame( class CConnectGamePacket *pPacket );
	bool ProcessMyIDPacket( class CMyIDPacket *pPacket );
	bool ProcessGameConnectingClientAccepted( class CGameConnectingClientAccepted *pPacket );
	bool ProcessAnswerConnectGame( class CAnswerConnectGame *pPacket );
	bool ProcessClientWantToConnect( class CClientWantToConnect *pPacket );
	bool ProcessLeaveGame( class CLeaveGamePacket *pPacket );
	bool ProcessGameKilled( class CGameKilled *pPacket );
	bool ProcessKickClient( class CGameKickClient *pPacket );
	bool ProcessClientWasKicked( class CGameClientWasKicked *pPacket );
	bool ProcessConnectedGameID( class CConnectedGameID *pPacket );
	bool ProcessConnectGameFailed( class CConnectGameFailed *pPacket );
	bool ProcessGameClientDead( class CGameClientDead *pPacket );

	// debug functions
	void TogglePauseAcceptGamersNet( bool bPause );
	void TogglePauseConnectGamer( const int nGamer, bool bPause );
};

