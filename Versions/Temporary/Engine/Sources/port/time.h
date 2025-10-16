#pragma once

#include <chrono>
#include <cstdint>

// Portable replacement for GetTickCount: milliseconds from a monotonic clock.
//
// steady_clock rather than system_clock, because every caller here measures an
// interval, and system_clock can step backwards when the machine's wall clock
// is corrected.
//
// The result is 32 bits on purpose, matching what GetTickCount returned.
// Callers keep these readings in uint32_t fields, in NTimer::STime and in
// floats, and then subtract two of them. Unsigned subtraction gives the right
// interval even across the wrap at 2^32 milliseconds, roughly 49.7 days of
// uptime, which is exactly how GetTickCount behaved. Returning 64 bits would
// quietly break every such pair, because "now" would keep growing after the
// stored 32-bit value had already wrapped, turning a small difference into an
// enormous one.
//
// The epoch is unspecified and differs between platforms, so only the
// difference between two readings means anything. No caller depends on where
// zero sits.
inline uint32_t GetCurrentTimeMilliseconds()
{
	const auto sinceEpoch = std::chrono::steady_clock::now().time_since_epoch();
	return static_cast<uint32_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>( sinceEpoch ).count() );
}
