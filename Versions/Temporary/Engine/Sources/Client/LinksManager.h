#pragma once

#include "Net/LinksManager.h"

#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include <vector>

namespace NNet
{

class CGameLinksManager : public CLinksManagerCommon
{
	OBJECT_NOCOPY_METHODS( CGameLinksManager );

	std::list<sockaddr> bannedAddresses;

	struct SProbePeer
	{
		int room = 0;
		uint32_t setup = 0;
		int local = 0;
		uint64_t localCookie = 0;
		uint64_t remoteCookie = 0;
		uint32_t lastRequest = 0;
		bool hasValidatedAddress = false;
		CNodeAddress validatedAddress;
		std::vector<CNodeAddress> addresses;
		std::vector<std::pair<uint64_t, uint32_t>> replies;
	};

	// Recv runs on the net driver thread; configuration/measurements belong to the UI thread.
	mutable std::mutex probeMutex;
	mutable std::map<int, SProbePeer> probePeers;

protected:
	bool IsGoodAddress( const sockaddr &addr ) const override;

public:
	void BanAddress( const sockaddr &sockAddr );
	void ConfigureProbe(
			int peer, int local, int room, uint32_t setup, uint64_t cookie, const std::vector<CNodeAddress> &addresses );
	void SetRemoteCookie( int peer, uint64_t cookie );
	void RemoveProbe( int peer );
	void ClearProbes();
	void SendProbe( int peer, uint64_t nonce ) const;
	bool GetProbeAddress( int peer, CNodeAddress *address ) const;
	std::vector<std::pair<uint64_t, uint32_t>> TakeProbeReplies( int peer );
	bool Recv( CNodeAddress *source, CMemoryStream *packet ) const override;
};

} // namespace NNet
