#pragma once

#include "Server_Client_Common/PacketProcessor.h"

class CPinger : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CPinger )
	UINT64 nLastConnectTime;
	CObj<class CServerClient> pServerClient;
	std::string szName;
	std::string szPassword;
	int nConnectPeriod;
	std::string szIP;
	int nPort;
	int nNetVersion;
	int nAttemptsLeft;
	bool bConnectNow;
public:
	CPinger();
	CPinger( const int &nPeriod, const std::string &_szName, const std::string &_szPassword, const std::string &_szIP,
		const int &_nPort, const int &_nNetVersion );

	bool Segment();

	bool ProcessConnectServerResult( class CConnectServerPacket *pPacket );
};


