#pragma once

#include "Server_Client_Common/NetPacket.h"

#include <cstdint>

// Forwarded unchanged by the existing lobby. Only the joining client selects
// the route, and attempt numbers fence off delayed UDP handshakes from retries.
class CPeerSetupPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CPeerSetupPacket );

public:
	enum EAction
	{
		BeginAttempt,
		AcceptAttempt,
		SelectDirect,
		SelectRelay,
		ConfirmDirect,
		ConfirmRelay,
		RequestRecovery,
		Identity
	};

	int nGameID = 0;
	uint32_t nSetupID = 0;
	int nAttempt = 0;
	EAction eAction = BeginAttempt;
	int nServerID = 0;
	bool bRecovery = false;

	CPeerSetupPacket() {}

	CPeerSetupPacket( int nGameID, uint32_t nSetupID, int nAttempt, EAction eAction, int nServerID = 0 )
		: nGameID( nGameID )
		, nSetupID( nSetupID )
		, nAttempt( nAttempt )
		, eAction( eAction )
		, nServerID( nServerID )
	{
	}

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 2, &nGameID );
		saver.Add( 3, &nSetupID );
		saver.Add( 4, &nAttempt );
		saver.Add( 5, &eAction );
		saver.Add( 6, &nServerID );
		saver.Add( 7, &bRecovery );
		return 0;
	}
};
