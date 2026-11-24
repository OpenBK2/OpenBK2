#pragma once

#include "Server_Client_Common/NetPacket.h"

#include <cstdint>

// Opaque lobby forwarding measures the entire peer-server-peer round trip.
// This control stream is independent of the ordered peer gameplay stream.
class CPeerPathProbePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CPeerPathProbePacket );

public:
	int room = 0;
	uint32_t setup = 0;
	uint64_t nonce = 0;
	uint64_t cookie = 0;
	bool reply = false;
	int directScore = -1;
	int relayScore = -1;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 2, &room );
		saver.Add( 3, &setup );
		saver.Add( 4, &nonce );
		saver.Add( 5, &cookie );
		saver.Add( 6, &reply );
		saver.Add( 7, &directScore );
		saver.Add( 8, &relayScore );
		return 0;
	}
};
