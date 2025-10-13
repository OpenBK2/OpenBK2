#pragma once

#include "Main/CommandsHistory.hpp"
#include "System/Commands.h"
#include "MPInterfaceData.h"

#include <cstdint>

#include <zconf.h>

struct IAILogicCommandB2;
struct IRandomSeed;
typedef uLong CLogType;
typedef std::unordered_map<int/*#entry*/,CLogType> ChecksumLog;
typedef std::unordered_map<int/*#segment*/, std::pair<CLogType,ChecksumLog> > CSegmentChecksum;

struct SMultiplayerReplayInfo
{
	ZDATA
		CDBPtr<NDb::SMapInfo> pMap;
		std::vector<SMPSlot> slots;
		int nTechLevel;
		int nTimeLimit;
		int nCaptureTime;
		bool bUnitExperience;
		int nLastCommandSegment;
		int nLastGameSegment;
		int nWinningSide;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMap); f.Add(3,&slots); f.Add(4,&nTechLevel); f.Add(5,&nTimeLimit); f.Add(6,&nCaptureTime); f.Add(7,&bUnitExperience); f.Add(8,&nLastCommandSegment); f.Add(9,&nLastGameSegment); f.Add(10,&nWinningSide); return 0; }
	SMultiplayerReplayInfo() : nTechLevel( -1 ), nTimeLimit( -1 ), nCaptureTime( -1 ), bUnitExperience( false ), nLastCommandSegment( -1 ), nLastGameSegment( -1 ), nWinningSide( -1 ) {}
	SMultiplayerReplayInfo &operator=( const SMultiplayerReplayInfo &src )
	{
		pMap = src.pMap;
		slots = src.slots;
		nTechLevel = src.nTechLevel;
		nTimeLimit = src.nTimeLimit;
		nCaptureTime = src.nCaptureTime;
		bUnitExperience = src.bUnitExperience;
		nLastCommandSegment = src.nLastCommandSegment;
		nLastGameSegment = src.nLastGameSegment;
		nWinningSide = src.nWinningSide;
		return *this;
	}
};



class CCommandsHistory : public ICommandsHistory
{
	OBJECT_NOCOPY_METHODS( CCommandsHistory );

	CHistory savingHistory;
	ZDATA
		CHistory loadedHistory;
		bool bFinishedHistory;
		bool bGameFinished;
		bool bLoadedHistory;
		bool bCanAddCommand;
		CPtr<IRandomSeed> pStartSeed;
		SMultiplayerReplayInfo replayInfo;
		uint32_t dwLastCheckSum;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&loadedHistory); f.Add(3,&bFinishedHistory); f.Add(4,&bGameFinished); f.Add(5,&bLoadedHistory); f.Add(6,&bCanAddCommand); f.Add(7,&pStartSeed); f.Add(8,&replayInfo); f.Add(9,&dwLastCheckSum); return 0; }

	bool IsFinished( const CSegmentChecksum &_segmentChecksum, const int nGameTime ) const;
	bool IsSegmentLogDiffer( const int nGameTime, const int nEntry, const unsigned long ulChecksum, const CSegmentChecksum &_segmentChecksum );
	bool IsEntryQualified( int nEntry ) const;
	const bool SerializeHistory( const std::string &szFileName, const bool bRead );
public:
	CCommandsHistory();

	void StartNewGame( const NDb::SMapInfo *_pMap );
	bool LoadHistory( const std::string &szFileName );
	bool SaveReplay( const std::string &szFileName, const SB2GameSpecificData &gameDesc, const std::vector<SMPSlot> &slots, const int nWinningSide );

	const NDb::SMapInfo *GetMap() const { return replayInfo.pMap; }

	void AddCommand( const int nSegment, struct IAILogicCommandB2 *pCmd );
	void ExecuteSegmentCommands( const int nSegment, struct ITransceiver *pTranceiver );

	bool AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry );
	const unsigned long GetLastChecksum() const { return dwLastCheckSum; }
	CHistory GetCommandsHistory() const;
};


