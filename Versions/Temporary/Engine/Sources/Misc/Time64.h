#pragma once

#include <chrono>
#include <cstdint>

// Milliseconds, 64 bit, for the timeouts the lobby and the async detector measure.
//
// This was GetSystemTimeAsFileTime's 100 nanosecond ticks since 1601, divided by
// 10000. The epoch moves to steady_clock's, which is unspecified, and no caller
// notices: every one of them compares or subtracts two readings and never asks
// what the absolute value means.
//
// steady_clock rather than system_clock for the reason port/time.h gives for
// GetCurrentTimeMilliseconds: these are intervals, and a wall clock can step
// backwards when the machine's time is corrected. Sixty-four bits here, so unlike
// that function there is no wrap to work around.
inline uint64_t GetLongTickCount()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch() ).count();
}
