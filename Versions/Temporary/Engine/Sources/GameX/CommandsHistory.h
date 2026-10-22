#pragma once

#include "Main/CommandsHistory.hpp"
#include "MPInterfaceData.h"
#include "System/Commands.h"

#include <cstdint>

#include <zconf.h>

struct IAILogicCommandB2;
struct IRandomSeed;
typedef uLong CLogType;
typedef std::unordered_map<int /*#entry*/, CLogType> ChecksumLog;
typedef std::unordered_map<int /*#segment*/, std::pair<CLogType, ChecksumLog>> CSegmentChecksum;

// The state checksums a recording sampled, so a playback can be checked against
// them rather than only against reaching the end.
//
// Not a map: IBinSaver serializes vector, pair, set, list and unordered_map but
// not map, and an unordered_map would make the file bytes depend on hash
// iteration order.
//
// Two parallel vectors rather than one vector of pairs, because the vector
// overload sends a type with no operator& through DoDataVector, which is a
// single bulk copy of the whole array, while a pair has its own overload and so
// would cost chunk framing per sample. At tick resolution that is the
// difference between one copy and tens of thousands of small chunks. They are
// only ever appended together, so they stay the same length and in increasing
// game time, which lets a playback walk them with a cursor.
//
// uint32_t and not uLong for the values. uLong is unsigned long, 4 bytes on
// Windows and 8 on Linux, and the whole point of these is being compared
// between the two. adler32 fits in 32 bits.
struct SReplayCheckSums
{
	ZDATA
	// What produced the values, so a recording sampled one way is not
	// compared against a run sampling another way and reported as diverged.
	int nLightMode = {};
	int nPeriod = {};
	std::vector<int> times;
	std::vector<uint32_t> values;
	// How many random numbers had been drawn by each sample. Almost every
	// draw is routine per-unit AI, so the count is a function of how many
	// units exist and the order they are walked in. If it parts company
	// before the state checksum does, the divergence is in what consumed
	// the generator rather than in the arithmetic, which are very different
	// things to go looking for. Wrapping is harmless: both sides wrap alike
	// and only equality is asked.
	std::vector<uint32_t> rngCalls;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &nLightMode );
		f.Add( 3, &nPeriod );
		f.Add( 4, &times );
		f.Add( 5, &values );
		f.Add( 6, &rngCalls );
		return 0;
	}

	void Add( const int nGameTime, const uint32_t uValue, const uint32_t uRngCalls )
	{
		times.push_back( nGameTime );
		values.push_back( uValue );
		rngCalls.push_back( uRngCalls );
	}

	// A file could carry mismatched lengths; everything reading these should
	// treat that as no samples rather than index past the shorter one.
	size_t Size() const
	{
		return times.size() == values.size() ? times.size() : 0;
	}

	// Separately, because a replay written before chunk 6 has the samples but
	// not the counts.
	bool HasRngCalls() const
	{
		return rngCalls.size() == Size() && Size() > 0;
	}
};

struct SMultiplayerReplayInfo
{
	ZDATA
	CDBPtr<NDb::SMapInfo> pMap;
	std::vector<SMPSlot> slots;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nTechLevel = {};
	int nTimeLimit = {};
	int nCaptureTime = {};
	bool bUnitExperience = {};
	int nLastCommandSegment = {};
	int nLastGameSegment = {};
	int nWinningSide = {};
	// Which start path LoadHistory has to take. Recorded as "is it single
	// player" rather than as the game type, and chunk 11 is the newest
	// field, because CStructureSaver::DataChunk memsets a POD chunk to zero
	// when the file does not carry it. A replay written before this field
	// existed therefore reads false here, which selects the multiplayer path
	// that was previously taken unconditionally. Storing the EGameType
	// instead would read as EGT_SINGLE, which is zero, and silently
	// misclassify every existing replay.
	bool bSinglePlayer = {};
	// Which start path a playback has to take, and the two arguments it
	// cannot otherwise know. Deliberately the decision rather than the
	// engine's EGameType: this is a file format, so the numbering is ours to
	// choose, and choosing it means zero can mean what replays recorded
	// before these fields should mean. Storing EGameType would have put
	// EGT_SINGLE, which is zero, on every old replay. The alternative, a
	// bias or a flag bit so that zero is never valid, is only needed when
	// the values are fixed elsewhere and cannot be renumbered.
	//
	//   0  as before: bSinglePlayer picks multiplayer or a custom mission
	//   1  campaign
	int nStartKind = {};
	// Both default to what the playback path used to hard-code, so an older
	// replay behaves exactly as it did.
	int nDifficulty = {};
	bool bIsTutorial = {};
	// What recorded this, so that a playback can say "a different build made
	// this" instead of reporting a divergence. A recording is only
	// comparable against the build and the data it was made with: change the
	// simulation and it diverges on every run, identically, which is exactly
	// how a reproducible defect presents and is why two sessions were spent
	// chasing one that was not there.
	//
	// Empty and zero both mean "not recorded", which covers every replay
	// written before these chunks as well as a stripped build that has no
	// identity to give. Neither is ever reported as a mismatch.
	//
	// The two cover different halves and neither implies the other. The
	// image id is a digest over the build ids of the simulation modules, so
	// it changes when any of them is relinked, including for a comment, and
	// over-reports; the map checksum covers the authored content a playback
	// re-reads and is blind to a unit statistic or a game constant edited
	// elsewhere in the database, so it under-reports. Together they catch
	// the two things that actually invalidate a recording in practice.
	std::string szImageId;
	uint32_t uMapCheckSum = {};
	// The configuration that recorded it, so that the commonest reason two
	// image ids differ can be named. A release recording verified under a
	// sanitizer build is the main thing the harness does, and those two ids
	// never match however identical the sources are.
	std::string szConfiguration;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &pMap );
		f.Add( 3, &slots );
		f.Add( 4, &nTechLevel );
		f.Add( 5, &nTimeLimit );
		f.Add( 6, &nCaptureTime );
		f.Add( 7, &bUnitExperience );
		f.Add( 8, &nLastCommandSegment );
		f.Add( 9, &nLastGameSegment );
		f.Add( 10, &nWinningSide );
		f.Add( 11, &bSinglePlayer );
		f.Add( 12, &nStartKind );
		f.Add( 13, &nDifficulty );
		f.Add( 14, &bIsTutorial );
		f.Add( 15, &szImageId );
		f.Add( 16, &uMapCheckSum );
		f.Add( 17, &szConfiguration );
		return 0;
	}

	SMultiplayerReplayInfo()
		: nTechLevel( -1 )
		, nTimeLimit( -1 )
		, nCaptureTime( -1 )
		, bUnitExperience( false )
		, nLastCommandSegment( -1 )
		, nLastGameSegment( -1 )
		, nWinningSide( -1 )
		, bSinglePlayer( false )
		, nStartKind( 0 )
		, nDifficulty( 0 )
		, bIsTutorial( false )
	{
	}

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
		bSinglePlayer = src.bSinglePlayer;
		nStartKind = src.nStartKind;
		nDifficulty = src.nDifficulty;
		bIsTutorial = src.bIsTutorial;
		szImageId = src.szImageId;
		uMapCheckSum = src.uMapCheckSum;
		szConfiguration = src.szConfiguration;
		return *this;
	}
};

class CCommandsHistory : public ICommandsHistory
{
	OBJECT_NOCOPY_METHODS( CCommandsHistory );

	CHistory savingHistory;
	// Playback verification state. Not serialized: it describes this run rather
	// than the replay. The cursor walks recordedCheckSums, which is in
	// increasing game time, so matching is a scan and not a lookup.
	size_t nCheckSumCursor = 0;
	int nCheckSumsMatched = 0;
	bool bCheckSumMismatch = false;
	bool bCheckSumModeReported = false;
	bool bRngCallsMismatch = false;
	// Whether the start seed came out of the replay rather than being the fresh
	// entropy the constructor installs. Without it the playback runs a different
	// random stream and divergence says nothing, so there is nothing to verify.
	bool bSeedFromReplay = false;
	// Diagnostic only, not serialized. Held open across a sample because the
	// entries arrive in the thousands.
	FILE *pEntryDumpFile = 0;
	ZDATA
	CHistory loadedHistory;
	bool bFinishedHistory = {};
	bool bGameFinished = {};
	bool bLoadedHistory = {};
	bool bCanAddCommand = {};
	CPtr<IRandomSeed> pStartSeed;
	SMultiplayerReplayInfo replayInfo;
	uint32_t dwLastCheckSum = {};
	// Also in the save game, so a mission saved and reloaded does not lose
	// the samples taken before the save and write a truncated replay.
	SReplayCheckSums recordedCheckSums;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &loadedHistory );
		f.Add( 3, &bFinishedHistory );
		f.Add( 4, &bGameFinished );
		f.Add( 5, &bLoadedHistory );
		f.Add( 6, &bCanAddCommand );
		f.Add( 7, &pStartSeed );
		f.Add( 8, &replayInfo );
		f.Add( 9, &dwLastCheckSum );
		f.Add( 10, &recordedCheckSums );
		return 0;
	}

	bool IsFinished( const CSegmentChecksum &_segmentChecksum, const int nGameTime ) const;
	bool IsSegmentLogDiffer( const int nGameTime,
	                         const int nEntry,
	                         const uint32_t ulChecksum,
	                         const CSegmentChecksum &_segmentChecksum );
	bool IsEntryQualified( int nEntry ) const;
	const bool SerializeHistory( const std::string &szFileName, const bool bRead );

public:
	CCommandsHistory();

	void StartNewGame( const NDb::SMapInfo *_pMap ) override;
	bool LoadHistory( const std::string &szFileName );
	bool SaveReplay( const std::string &szFileName,
	                 const SB2GameSpecificData &gameDesc,
	                 const std::vector<SMPSlot> &slots,
	                 const int nWinningSide );

	const NDb::SMapInfo *GetMap() const override
	{
		return replayInfo.pMap;
	}

	void AddCommand( const int nSegment, struct IAILogicCommandB2 *pCmd ) override;
	void ExecuteSegmentCommands( const int nSegment, struct ITransceiver *pTranceiver ) override;

	bool AddChecksumLog( const int nGameTime, const uint32_t ulChecksum, const int nEntry ) override;
	// Compare one playback sample against what the replay recorded for the same
	// game time. Returns false once a divergence has been reported, which is
	// what ICheckSumLog uses to mean "differs from the loaded one".
	bool VerifyCheckSum( const int nGameTime, const uint32_t uValue );

	// Whether a verification run can claim anything, and what.
	bool HasCheckSumMismatch() const
	{
		return bCheckSumMismatch;
	}

	int GetCheckSumsMatched() const
	{
		return nCheckSumsMatched;
	}

	const uint32_t GetLastChecksum() const override
	{
		return dwLastCheckSum;
	}

	CHistory GetCommandsHistory() const override;
};
