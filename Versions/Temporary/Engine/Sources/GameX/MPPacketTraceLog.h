#pragma once

#include <string>
#include <vector>

namespace NGameX
{
struct SMatchPacketTraceSlot
{
	int nSlot;
	int nClientID;
	int nTeam;
	bool bPresent;
	SMatchPacketTraceSlot() : nSlot( -1 ), nClientID( -1 ), nTeam( -1 ), bPresent( false ) {}
};

void MatchPacketTrace_Reset();
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
	const std::vector<SMatchPacketTraceSlot> &slots );

void MatchPacketTrace_Log(
	int nCommonSegment,
	const char *szEventType,
	const char *szName,
	int nFromClientID,
	const std::string &szDetails );

void MatchPacketTrace_RecordDropScheduled( int nSlot, int nSegment );
void MatchPacketTrace_RecordDropApplied( int nSlot, int nSegment );
void MatchPacketTrace_SetFinalState( unsigned long dwPresentMask, unsigned long dwLaggersMask, unsigned long dwTransceiverMask );
void MatchPacketTrace_Flush( const char *szReason );
}

