#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>

// Wall-clock transport policy only; none of these measurements enter simulation.
struct SPeerPathQuality
{
	static constexpr uint32_t TIMEOUT = 15000;
	static constexpr uint32_t PROBE_PERIOD = 2000;
	static constexpr uint32_t SWITCH_HOLD = 10000;
	static constexpr uint32_t SWITCH_COOLDOWN = 30000;
	std::map<uint64_t, uint32_t> pending;
	std::deque<bool> results;
	uint32_t lastReply = 0;
	unsigned replies = 0;
	float rtt = 0;
	float jitter = 0;

	void Record( bool success )
	{
		results.push_back( success );
		if ( results.size() > 12 )
		{
			results.pop_front();
		}
	}

	void Expire( uint32_t now )
	{
		for ( auto it = pending.begin(); it != pending.end(); )
		{
			if ( now - it->second >= TIMEOUT )
			{
				Record( false );
				it = pending.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	bool Reply( uint64_t nonce, uint32_t now )
	{
		const auto it = pending.find( nonce );
		if ( it == pending.end() || now - it->second >= TIMEOUT )
		{
			return false;
		}
		const float sample = static_cast<float>( now - it->second );
		const float difference = sample > rtt ? sample - rtt : rtt - sample;
		jitter = replies ? jitter * 0.75f + difference * 0.25f : sample * 0.5f;
		rtt = replies ? rtt * 0.875f + sample * 0.125f : sample;
		++replies;
		lastReply = now;
		pending.erase( it );
		Record( true );
		return true;
	}

	uint32_t SilenceTimeout() const
	{
		// Never shorten the requested 15-second floor on a fast LAN.
		return (std::max)( TIMEOUT, static_cast<uint32_t>( rtt + 4 * jitter + 1000 ) );
	}

	bool Healthy( uint32_t now ) const
	{
		return replies >= 3 && now - lastReply < TIMEOUT;
	}

	int Score( uint32_t now ) const
	{
		if ( replies < 5 || !Healthy( now ) || results.empty() )
		{
			return -1;
		}
		const auto missed = std::count( results.begin(), results.end(), false );
		return static_cast<int>( rtt + 2 * jitter + 1500.0f * missed / results.size() );
	}

	static bool PreferAlternative( bool directActive, int directScore, int relayScore )
	{
		if ( directScore < 0 || relayScore < 0 )
		{
			return false;
		}
		// Relay must earn its extra hop. Returning to direct has a smaller margin.
		return directActive ? relayScore + 100 < directScore * 0.75f : directScore + 25 < relayScore * 0.9f;
	}
};
