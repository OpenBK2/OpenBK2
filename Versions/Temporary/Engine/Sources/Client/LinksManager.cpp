#include "stdafx.h"

#include "LinksManager.h"
#include "port/time.h"

#include <array>

namespace NNet
{

namespace
{
	constexpr uint32_t PROBE_MAGIC = 0x324b42fe;
	constexpr size_t PROBE_SIZE = 40;

	void WriteWord( std::array<unsigned char, PROBE_SIZE> &bytes, size_t word, uint32_t value )
	{
		for ( size_t i = 0; i < 4; ++i )
		{
			bytes[word * 4 + i] = static_cast<unsigned char>( value >> ( 8 * i ) );
		}
	}

	uint32_t ReadWord( const unsigned char *bytes, size_t word )
	{
		uint32_t value = 0;
		for ( size_t i = 0; i < 4; ++i )
		{
			value |= static_cast<uint32_t>( bytes[word * 4 + i] ) << ( 8 * i );
		}
		return value;
	}
} // namespace

void CGameLinksManager::ConfigureProbe(
		int peer, int local, int room, uint32_t setup, uint64_t cookie, const std::vector<CNodeAddress> &addresses )
{
	std::lock_guard<std::mutex> lock( probeMutex );
	SProbePeer &probe = probePeers[peer];
	if ( probe.setup != setup || probe.room != room )
	{
		probe = SProbePeer();
	}
	probe.room = room;
	probe.setup = setup;
	probe.local = local;
	probe.localCookie = cookie;
	for ( const auto &address : addresses )
	{
		if ( std::find( probe.addresses.begin(), probe.addresses.end(), address ) == probe.addresses.end() &&
		     probe.addresses.size() < 4 )
		{
			probe.addresses.push_back( address );
		}
	}
}

void CGameLinksManager::SetRemoteCookie( int peer, uint64_t cookie )
{
	std::lock_guard<std::mutex> lock( probeMutex );
	const auto it = probePeers.find( peer );
	if ( it != probePeers.end() )
	{
		it->second.remoteCookie = cookie;
	}
}

void CGameLinksManager::RemoveProbe( int peer )
{
	std::lock_guard<std::mutex> lock( probeMutex );
	probePeers.erase( peer );
}

void CGameLinksManager::ClearProbes()
{
	std::lock_guard<std::mutex> lock( probeMutex );
	probePeers.clear();
}

void CGameLinksManager::SendProbe( int peer, uint64_t nonce ) const
{
	std::lock_guard<std::mutex> lock( probeMutex );
	const auto it = probePeers.find( peer );
	if ( it == probePeers.end() || !it->second.remoteCookie )
	{
		return;
	}
	const SProbePeer &probe = it->second;
	std::array<unsigned char, PROBE_SIZE> bytes{};
	WriteWord( bytes, 0, PROBE_MAGIC );
	WriteWord( bytes, 1, probe.room );
	WriteWord( bytes, 2, probe.setup );
	WriteWord( bytes, 3, probe.local );
	WriteWord( bytes, 4, peer );
	WriteWord( bytes, 5, static_cast<uint32_t>( probe.remoteCookie ) );
	WriteWord( bytes, 6, static_cast<uint32_t>( probe.remoteCookie >> 32 ) );
	WriteWord( bytes, 7, static_cast<uint32_t>( nonce ) );
	WriteWord( bytes, 8, static_cast<uint32_t>( nonce >> 32 ) );
	CMemoryStream packet;
	packet.Write( bytes.data(), bytes.size() );
	for ( const auto &address : probe.addresses )
	{
		Send( address, packet );
	}
}

bool CGameLinksManager::GetProbeAddress( int peer, CNodeAddress *address ) const
{
	std::lock_guard<std::mutex> lock( probeMutex );
	const auto it = probePeers.find( peer );
	if ( it == probePeers.end() || !it->second.hasValidatedAddress )
	{
		return false;
	}
	*address = it->second.validatedAddress;
	return true;
}

std::vector<std::pair<uint64_t, uint32_t>> CGameLinksManager::TakeProbeReplies( int peer )
{
	std::lock_guard<std::mutex> lock( probeMutex );
	std::vector<std::pair<uint64_t, uint32_t>> replies;
	const auto it = probePeers.find( peer );
	if ( it != probePeers.end() )
	{
		replies.swap( it->second.replies );
	}
	return replies;
}

bool CGameLinksManager::Recv( CNodeAddress *source, CMemoryStream *packet ) const
{
	// Fixed-size probes bypass reliable stream ordering on the very same UDP socket.
	// A cookie learned through the authenticated lobby limits replies to room peers.
	for ( unsigned count = 0; count < 64 && CLinksManagerCommon::Recv( source, packet ); ++count )
	{
		const auto *bytes = reinterpret_cast<const unsigned char *>( packet->GetBuffer() );
		if ( packet->GetSize() != PROBE_SIZE || ReadWord( bytes, 0 ) != PROBE_MAGIC )
		{
			return true;
		}
		std::lock_guard<std::mutex> lock( probeMutex );
		const int peer = static_cast<int>( ReadWord( bytes, 3 ) );
		const auto it = probePeers.find( peer );
		if ( it == probePeers.end() )
		{
			continue;
		}
		SProbePeer &probe = it->second;
		const uint64_t cookie = ReadWord( bytes, 5 ) | ( static_cast<uint64_t>( ReadWord( bytes, 6 ) ) << 32 );
		const uint64_t nonce = ReadWord( bytes, 7 ) | ( static_cast<uint64_t>( ReadWord( bytes, 8 ) ) << 32 );
		const uint32_t kind = ReadWord( bytes, 9 );
		if ( ReadWord( bytes, 1 ) != static_cast<uint32_t>( probe.room ) || ReadWord( bytes, 2 ) != probe.setup ||
		     ReadWord( bytes, 4 ) != static_cast<uint32_t>( probe.local ) || kind > 1 || !cookie ||
		     cookie != ( kind ? probe.remoteCookie : probe.localCookie ) )
		{
			continue;
		}
		const uint32_t now = GetCurrentTimeMilliseconds();
		if ( kind )
		{
			if ( probe.replies.size() < 32 )
			{
				probe.replies.emplace_back( nonce, now );
			}
		}
		else if ( now - probe.lastRequest >= 250 )
		{
			probe.lastRequest = now;
			probe.hasValidatedAddress = true;
			probe.validatedAddress = *source;
			// An authenticated request also teaches the accepting peer the NAT endpoint.
			if ( std::find( probe.addresses.begin(), probe.addresses.end(), *source ) == probe.addresses.end() )
			{
				probe.addresses.insert( probe.addresses.begin(), *source );
				if ( probe.addresses.size() > 4 )
				{
					probe.addresses.pop_back();
				}
			}
			std::array<unsigned char, PROBE_SIZE> response;
			std::copy( bytes, bytes + PROBE_SIZE, response.begin() );
			WriteWord( response, 3, probe.local );
			WriteWord( response, 4, peer );
			WriteWord( response, 9, 1 );
			CMemoryStream reply;
			reply.Write( response.data(), response.size() );
			Send( *source, reply );
		}
	}
	return false;
}

bool CGameLinksManager::IsGoodAddress( const sockaddr &addr ) const
{
	bool bGood = true;
	std::list<sockaddr>::const_iterator iter = bannedAddresses.begin();
	while ( iter != bannedAddresses.end() && bGood )
	{
		const sockaddr &bannedAddr = *iter;
		bGood = memcmp( &addr, &bannedAddr, sizeof( addr ) ) != 0;

		++iter;
	}

	return bGood;
}

void CGameLinksManager::BanAddress( const sockaddr &sockAddr )
{
	bannedAddresses.push_back( sockAddr );
}

} // namespace NNet
