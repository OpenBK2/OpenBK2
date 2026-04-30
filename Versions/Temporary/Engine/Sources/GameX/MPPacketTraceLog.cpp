#include "stdafx.h"

#include "MPPacketTraceLog.h"
#include "Misc/StrProc.h"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <map>
#include <sstream>

namespace NGameX
{
namespace
{
struct SDropTrace
{
	int nScheduledSegment;
	int nAppliedSegment;
	SDropTrace() : nScheduledSegment( -1 ), nAppliedSegment( -1 ) {}
};

struct STraceState
{
	bool bActive;
	bool bFlushed;
	int nEventID;
	int nGameID;
	std::string szSessionName;
	std::string szMapName;
	unsigned long ulMapChecksum;
	int nOwnClientID;
	int nOwnSlot;
	int nHostClientID;
	unsigned long dwInitialPlayers;
	unsigned long dwInitialPresentMask;
	unsigned long dwInitialTransceiverMask;
	unsigned long dwFinalPresentMask;
	unsigned long dwFinalLaggersMask;
	unsigned long dwFinalTransceiverMask;
	std::vector<SMatchPacketTraceSlot> slots;
	std::vector<std::string> events;
	std::map<std::string, int> eventTypeCounters;
	std::map<std::string, int> eventNameCounters;
	std::map<int, SDropTrace> drops;
	std::string szFlushReason;

	STraceState()
		: bActive( false ), bFlushed( false ), nEventID( 0 ), nGameID( -1 ),
		  ulMapChecksum( 0 ), nOwnClientID( -1 ), nOwnSlot( -1 ), nHostClientID( -1 ),
		  dwInitialPlayers( 0 ), dwInitialPresentMask( 0 ), dwInitialTransceiverMask( 0 ),
		  dwFinalPresentMask( 0 ), dwFinalLaggersMask( 0 ), dwFinalTransceiverMask( 0 )
	{
	}
};

STraceState g_trace;

std::string SanitizeText( const std::string &src )
{
	std::string out = src;
	for ( size_t i = 0; i < out.size(); ++i )
	{
		if ( out[i] == '\n' || out[i] == '\r' || out[i] == '\t' )
			out[i] = ' ';
	}
	return out;
}

std::string FormatMask( unsigned long value )
{
	std::ostringstream ss;
	ss << "0x" << std::uppercase << std::hex << std::setw( 8 ) << std::setfill( '0' ) << value;
	return ss.str();
}
}

void MatchPacketTrace_Reset()
{
	g_trace = STraceState();
	g_trace.bActive = true;
}

void MatchPacketTrace_SetHeader(
	int nGameID,
	const std::string &szSessionName,
	const std::string &szMapName,
	unsigned long ulMapChecksum,
	int nOwnClientID,
	int nOwnSlot,
	int nHostClientID,
	unsigned long dwInitialPlayers,
	unsigned long dwInitialPresentMask,
	unsigned long dwInitialTransceiverMask,
	const std::vector<SMatchPacketTraceSlot> &slots )
{
	if ( !g_trace.bActive )
		MatchPacketTrace_Reset();

	g_trace.nGameID = nGameID;
	g_trace.szSessionName = szSessionName;
	g_trace.szMapName = szMapName;
	g_trace.ulMapChecksum = ulMapChecksum;
	g_trace.nOwnClientID = nOwnClientID;
	g_trace.nOwnSlot = nOwnSlot;
	g_trace.nHostClientID = nHostClientID;
	g_trace.dwInitialPlayers = dwInitialPlayers;
	g_trace.dwInitialPresentMask = dwInitialPresentMask;
	g_trace.dwInitialTransceiverMask = dwInitialTransceiverMask;
	g_trace.slots = slots;
}

void MatchPacketTrace_Log(
	int nCommonSegment,
	const char *szEventType,
	const char *szName,
	int nFromClientID,
	const std::string &szDetails )
{
	if ( !g_trace.bActive || g_trace.bFlushed )
		return;

	const std::string eventType = szEventType ? szEventType : "UNKNOWN";
	const std::string eventName = szName ? szName : "UNKNOWN";
	const std::string details = SanitizeText( szDetails );

	++g_trace.nEventID;
	g_trace.eventTypeCounters[eventType] += 1;
	g_trace.eventNameCounters[eventName] += 1;

	std::ostringstream line;
	line << "#" << std::setw( 6 ) << std::setfill( '0' ) << g_trace.nEventID
		 << " seg=" << nCommonSegment
		 << " type=" << eventType
		 << " name=" << eventName
		 << " from=" << nFromClientID;
	if ( !details.empty() )
		line << " " << details;
	g_trace.events.push_back( line.str() );
}

void MatchPacketTrace_RecordDropScheduled( int nSlot, int nSegment )
{
	if ( !g_trace.bActive || g_trace.bFlushed || nSlot < 0 )
		return;

	SDropTrace &drop = g_trace.drops[nSlot];
	if ( drop.nScheduledSegment < 0 || nSegment < drop.nScheduledSegment )
		drop.nScheduledSegment = nSegment;
}

void MatchPacketTrace_RecordDropApplied( int nSlot, int nSegment )
{
	if ( !g_trace.bActive || g_trace.bFlushed || nSlot < 0 )
		return;

	SDropTrace &drop = g_trace.drops[nSlot];
	if ( drop.nAppliedSegment < 0 || nSegment < drop.nAppliedSegment )
		drop.nAppliedSegment = nSegment;
}

void MatchPacketTrace_SetFinalState( unsigned long dwPresentMask, unsigned long dwLaggersMask, unsigned long dwTransceiverMask )
{
	if ( !g_trace.bActive || g_trace.bFlushed )
		return;
	g_trace.dwFinalPresentMask = dwPresentMask;
	g_trace.dwFinalLaggersMask = dwLaggersMask;
	g_trace.dwFinalTransceiverMask = dwTransceiverMask;
}

void MatchPacketTrace_Flush( const char *szReason )
{
	if ( !g_trace.bActive || g_trace.bFlushed )
		return;

	g_trace.bFlushed = true;
	g_trace.szFlushReason = szReason ? szReason : "unknown";

	// Keep latest match trace in one deterministic file so logs can be diffed across clients.
	FILE *file = fopen( "last_match_packets.txt", "w" );
	if ( !file )
		return;

	fprintf( file, "=== last_match_packets ===\n" );
	fprintf( file, "flush_reason=%s\n", g_trace.szFlushReason.c_str() );
	fprintf( file, "game_id=%d session_name=%s map_name=%s map_checksum=%s\n",
		g_trace.nGameID, g_trace.szSessionName.c_str(), g_trace.szMapName.c_str(), FormatMask( g_trace.ulMapChecksum ).c_str() );
	fprintf( file, "own_client_id=%d own_slot=%d host_client_id=%d\n",
		g_trace.nOwnClientID, g_trace.nOwnSlot, g_trace.nHostClientID );
	fprintf( file, "initial_players=%s initial_present_mask=%s initial_transceiver_mask=%s\n",
		FormatMask( g_trace.dwInitialPlayers ).c_str(),
		FormatMask( g_trace.dwInitialPresentMask ).c_str(),
		FormatMask( g_trace.dwInitialTransceiverMask ).c_str() );
	fprintf( file, "final_present_mask=%s final_laggers_mask=%s final_transceiver_mask=%s\n",
		FormatMask( g_trace.dwFinalPresentMask ).c_str(),
		FormatMask( g_trace.dwFinalLaggersMask ).c_str(),
		FormatMask( g_trace.dwFinalTransceiverMask ).c_str() );

	fprintf( file, "\n-- slots --\n" );
	for ( size_t i = 0; i < g_trace.slots.size(); ++i )
	{
		const SMatchPacketTraceSlot &slot = g_trace.slots[i];
		fprintf( file, "slot=%d client_id=%d team=%d present=%d\n",
			slot.nSlot, slot.nClientID, slot.nTeam, slot.bPresent ? 1 : 0 );
	}

	fprintf( file, "\n-- events (%d) --\n", int( g_trace.events.size() ) );
	for ( size_t i = 0; i < g_trace.events.size(); ++i )
	{
		fprintf( file, "%s\n", g_trace.events[i].c_str() );
	}

	fprintf( file, "\n-- event_type_counts --\n" );
	for ( std::map<std::string, int>::const_iterator it = g_trace.eventTypeCounters.begin(); it != g_trace.eventTypeCounters.end(); ++it )
	{
		fprintf( file, "%s=%d\n", it->first.c_str(), it->second );
	}

	fprintf( file, "\n-- event_name_counts --\n" );
	for ( std::map<std::string, int>::const_iterator it = g_trace.eventNameCounters.begin(); it != g_trace.eventNameCounters.end(); ++it )
	{
		fprintf( file, "%s=%d\n", it->first.c_str(), it->second );
	}

	fprintf( file, "\n-- drop_summary --\n" );
	for ( std::map<int, SDropTrace>::const_iterator it = g_trace.drops.begin(); it != g_trace.drops.end(); ++it )
	{
		const int slot = it->first;
		const SDropTrace &drop = it->second;
		const int delta = ( drop.nScheduledSegment >= 0 && drop.nAppliedSegment >= 0 ) ? ( drop.nAppliedSegment - drop.nScheduledSegment ) : 0;
		fprintf( file, "slot=%d scheduled_seg=%d applied_seg=%d delta=%d\n",
			slot, drop.nScheduledSegment, drop.nAppliedSegment, delta );
	}

	fclose( file );
}
}

