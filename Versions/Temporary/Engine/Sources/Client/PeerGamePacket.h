#pragma once

#include "Server_Client_Common/NetPacket.h"
#include "Server_Client_Common/NetSaver.h"

#include <cstdint>
#include <vector>

// Sequence numbers belong to the peer session, not its UDP/relay transport.
// Retaining serialized messages until application acknowledgement lets a route
// switch resend the uncertain suffix without losing or repeating game commands.
class CPeerGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CPeerGamePacket );

public:
	int nGameID = 0;
	uint32_t nSetupID = 0;
	uint64_t nSequence = 0;
	uint64_t nAcknowledged = 0;
	bool bRequestAck = false;
	std::vector<char> data;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 2, &nGameID );
		saver.Add( 3, &nSetupID );
		saver.Add( 4, &nSequence );
		saver.Add( 5, &nAcknowledged );
		saver.Add( 6, &bRequestAck );
		AddNetPacketBytes( saver, 7, &data );
		return 0;
	}
};
