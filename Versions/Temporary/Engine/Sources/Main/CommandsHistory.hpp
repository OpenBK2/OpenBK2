#pragma once

#include "../System/CheckSumLog.h"
namespace NDb
{
	struct SMapInfo;
}

struct ICommandsHistory : public ICheckSumLog
{
	virtual void StartNewGame( const NDb::SMapInfo *pMap ) = 0;
	virtual const NDb::SMapInfo *GetMap() const = 0;
	
	virtual void AddCommand( const int nSegment, struct IAILogicCommandB2 *pCmd ) = 0;
	virtual void ExecuteSegmentCommands( const int nSegment, struct ITransceiver *pTranceiver ) = 0;

	//virtual bool SaveReplay( const string &szFileName );

	// only remember last checksum
	virtual bool AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry ) = 0;
	virtual const unsigned long GetLastChecksum() const = 0;
};
struct SReplayInfo;
ICommandsHistory *CreateCommandsHistory( const SReplayInfo &replay );

