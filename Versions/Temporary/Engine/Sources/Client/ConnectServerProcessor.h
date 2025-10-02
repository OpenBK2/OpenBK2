#pragma once

#include "../Server_Client_Common/PacketProcessor.h"

class CConnectServerProcessor : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CConnectServerProcessor );

	enum EConnectState
	{
		ECS_INACTIVE,
		ECS_CONNECTING,
		ECS_ACTIVE,
	};

	EConnectState eConnectState;
	CPtr<class CNet> pServer;
	bool bCheckConnect;

	string szNick, szPassword;
	string szCDKey;
	string szEMail;

	enum EConnectType { ECT_LOGIN, ECT_REGISTER, ECT_FORGOT_PASSWORD };
	EConnectType eConnectType;
public:
	CConnectServerProcessor() : eConnectState( ECS_INACTIVE ) { }
	CConnectServerProcessor( class CNet *pServer, bool bCheckConnect );

	bool Segment();

	//
	bool ProcessCheckConnect( class CCheckConnectPacket *pCheckConnectPacket );
	bool ProcessLoginPacket( class CLoginPacket *pLoginPacket );
	bool ProcessRegisterPacket( class CRegisterPacket *pRegisterPacket );
	bool ProcessForgottenPasswordPacket( class CForgottenPasswordPacket *pPacket ); 
	bool ProcessNewClient( class CNetNewClient *pPacket );
};

