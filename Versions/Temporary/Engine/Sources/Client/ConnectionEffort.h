#pragma once

#include "../Server_Client_Common/PacketProcessor.h"

class CNet;

enum EConnEffort
{
	ECE_NONE,
	ECE_SIMPLE,
	ECE_THROUGH_SERVER,
};

class CConnectionEffort : public CPacketProcessor
{
protected:
	void SendBreakThroughPacket( const string &szIP, const int nPort );
public:
	enum EState { ES_CONNECTING, ES_SUCCESS, ES_FAILED };

	virtual const EState GetState() const = 0;
	virtual const int GetClientServerID() const = 0;

	virtual struct IConnection* CreateConnection() const = 0;
	virtual CConnectionEffort* CreateNextEffort( const int nOurGameID ) = 0;
};

namespace NEffortsFactory
{
	CConnectionEffort* CreateFirstClientEffort( const int nMyServerID, const int nClientServerID, class CNet *pNet, const int nTimeOut );
	CConnectionEffort* CreateFirstServerEffort
	( const int nMyServerID, const int nClientServerID,
		const string &szClientIP, const int nClientPort, CNet *pNet, CNet *pAcceptGamersNet );

	void SetServerInfo( const string &szServerIP, const int nServerPort, const int nGameNetVersion );
}


