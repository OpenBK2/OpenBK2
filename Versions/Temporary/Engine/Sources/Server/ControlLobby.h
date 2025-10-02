#pragma once

#include "../Server_Client_Common/PacketProcessor.h"

class CControlLobby : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CControlLobby )

	CPtr<class CPrimeNumbers> pPrime;
	CPtr<class CClients> pClients;
	CPtr<class CNet> pNet;

	string szCfgFile;
	bool bCheckCDKeyIsValid;

	hash_set<int> connecting;
	hash_map<int, __int64> askedNumbers;
public:
	CControlLobby() { }
	CControlLobby( class CClients *pClients, class CNet *pNet, const string &szCfgFile );

	virtual bool Segment() { return true; }
	void ReloadConfig();
	//
	bool ProcessNewClient( class CNetNewClient *pPacket );
	bool ProcessCheckConnectAnswer( class CCheckConnectAnswerPacket *pPacket );
	bool ProcessRemoveClient( class CNetRemoveClient *pNetRemoveClient );
	bool ProcessCommonClientStatePacket( class CCommonClientStatePacket *pPacket );
	bool ProcessNewConnectingClient( class CNewGameConnectingClient *pPacket );
	bool ProcessDirectClientPacket( class CDirectPacketToClient *pPacket );
	bool ProcessThroughServerGamePacket( class CThroughServerGamePacket *pPacket );
	bool ProcessPingPacket( class CPingPacket *pPacket );
	bool ProcessForgottenPasswordPacket( class CForgottenPasswordPacket *pPacket );

	bool CanBePaused() { return false; }

};

