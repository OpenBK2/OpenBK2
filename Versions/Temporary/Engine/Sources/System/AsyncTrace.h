#pragma once

#include "System_export.h"

#include <string>

//! Tracing for the things that make a run depend on when rather than on what.
//!
//! Two of them are known. Resources load on a background thread and readiness is
//! polled rather than waited on, so the segment at which a resource's value
//! appears is decided by how fast the machine is. And executors are scheduled on
//! a wheel keyed by segment, so one appearing a segment earlier or later moves
//! work between segments.
//!
//! The point of this is to be diffed between two runs of the same binary on the
//! same replay. A checksum says two runs differ; this says what they did
//! differently. Everything is off unless trace_async_events is set, because the
//! volume is high and the formatting is not free.
namespace NAsyncTrace
{
SYSTEM_EXPORT bool IsEnabled();

//! Stamped on every line written after it, so an event lands somewhere in the
//! simulation rather than only somewhere in a sequence. Called once per
//! segment; anything traced outside a segment carries the previous one.
SYSTEM_EXPORT void SetGameTime( const int nGameTime );

SYSTEM_EXPORT void Write( const char *szCategory, const std::string &szWhat );

//! Separate from Write so that the resource hooks, which live in a template in
//! a header, do not drag a formatting library in behind them.
SYSTEM_EXPORT void WriteResource( const char *szWhat, const char *szResName, const int nID );
}
