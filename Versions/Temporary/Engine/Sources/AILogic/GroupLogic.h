#pragma once

#include "SegmentedObjects.h"
#include "Stats_B2_M1/AIUnitCmd.h"
#include "System/FreeIDs.h"

#include <cstdint>

class CCommonUnit;
class CAIUnit;
struct ICollisionsCollector;
enum EActionNotify : unsigned int;

// A group number is ( ( id << 4 ) | owner ) << 1, with the low bit set for a special
// group. It is a uint16_t everywhere it travels, so the id has 11 bits and the owner 4.
//
// The owner is there so that numbers picked on different machines cannot clash. A
// player's client picks the number for the groups its commands register, with the
// player's own number as the owner and a counter only that client advances, and the
// number then reaches every client inside the command. Simulation code that needs a
// group of its own (arriving reinforcements, start commands, scripts, the AI general,
// special groups) runs on every client and must pick the same number on each, so it
// cannot use either of those: it takes ids from a counter of its own, which every
// client advances identically and which is saved, under this owner, which no player
// can have. Using the local player's number and counter instead made the number
// depend on who was watching, and a replay watched as a player who issued commands
// had those groups emptied by the simulation's own.
//
// Players are numbered from 0 with the neutral player last, and the largest maps have
// ten players, so the neutral player is 10 and 11 to 15 are unused. Supporting more
// than 14 players plus the neutral one means widening the group number: the shifts and
// the mask in GetGroupNumberByID, GetSpecialGroupNumberByID, GetIdByGroupNumber and
// GetPlayerByGroupNumber, this owner value, and uint16_t to something wider in
// CGroupLogic, IAILogic (B2AI.h) and the GameX group commands (AILogicCommandInternal),
// which are serialized into replays and sent over the network, so that is a format
// change for both.
static constexpr uint8_t SIMULATION_GROUP_OWNER = 15;

class CFreeGroupIDs
{
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nMaxID = {};

public:
	int operator&( IBinSaver &f )
	{
		return 0;
	}

	CFreeGroupIDs()
		: nMaxID( 1 )
	{
	}

	const int Get()
	{
		nMaxID += 2;
		return nMaxID;
	}

	void Return( const int id ) {}
};

class CGroupLogic : public CAIObjectBase
{
	OBJECT_NOCOPY_METHODS( CGroupLogic );

public:
	int operator&( IBinSaver &saver ) override;

private:
	// Ids for the groups the local player's commands register; see
	// SIMULATION_GROUP_OWNER for why simulation code must not draw from it.
	CFreeGroupIDs groupIds;
	// The last id GenerateSimulationGroupNumber handed out. Only simulation code
	// advances it, so it is the same on every client, and it is saved.
	int nLastSimulationGroupID = 0;
	det_set<int> registeredGroups;
	CQueuesSet<CPtr<CCommonUnit>> groupUnits;
	std::list<CPtr<CCommonUnit>> followingUnits;

	// 0 - all units, except of infantry; 1 - infantry
	std::vector<NSegmObjs::CContainer<CPtr<CCommonUnit>>> segmUnits;
	NSegmObjs::CContainer<CPtr<CCommonUnit>> freezeUnits;
	NSegmObjs::CContainer<CPtr<CAIUnit>> firstPathUnits;
	NSegmObjs::CContainer<CPtr<CAIUnit>> secondPathUnits;

	NTimer::STime lastSegmTime = {};

	struct SAmbushInfo
	{
		int nUniqueId = {};
		CVec2 vAmbushCenter;
		SAIAngle wAmbushDir;
		bool bGivenCommandToRestore = {};

		SAmbushInfo()
			: nUniqueId( -1 )
			, vAmbushCenter( VNULL2 )
			, wAmbushDir( 0 )
			, bGivenCommandToRestore( false )
		{
		}

		SAmbushInfo( const int _nUniqueId )
			: nUniqueId( _nUniqueId )
			, vAmbushCenter( -1.0f, -1.0f )
			, wAmbushDir( 0 )
			, bGivenCommandToRestore( false )
		{
		}
	};

	typedef std::list<std::list<SAmbushInfo>> CAmbushGroups;
	CAmbushGroups ambushGroups;
	det_set<int> ambushUnits;
	NTimer::STime lastAmbushCheck = {};
	CPtr<ICollisionsCollector> pCollisionsCollector;

	//
	void DelGroup( const int nGroup );
	void DivideBySubGroups( const SAIUnitCmd &command, const int nGroup );
	// like M1
	void DivideBySubGroups2( const SAIUnitCmd &command, const int nGroup );

	// скорости юнитов в follow
	void SegmentFollowingUnits();
	// обработка застрявших из-зи коллизий юнитов
	void StayTimeSegment();

	void ProcessGridCommand( const CVec2 &vGridCenter, const CVec2 &vGridDir, const int nGroup, bool bPlaceInQueue );

	void EraseFromAmbushGroups( const SAIUnitCmd &command, const uint16_t wGroup );
	void CreateAmbushGroup( const uint16_t wGroup );
	void ProcessAmbushGroups();
	void SetToAmbush( CAmbushGroups::iterator &iter );

	//
	static uint16_t GetGroupNumberByID( const uint16_t wID, const uint8_t cOwner );
	static uint16_t GetSpecialGroupNumberByID( const uint16_t wID, const uint8_t cOwner );
	static uint16_t GetIdByGroupNumber( const uint16_t wGroup );
	static uint16_t GetPlayerByGroupNumber( const uint16_t wGroup );

public:
	CGroupLogic() {}

	void Init( ICollisionsCollector *pCollisionsCollector );

	void Clear()
	{
		DestroyContents();
	}

	// For a group the local player's commands register, from the client only.
	const uint16_t GenerateGroupNumber();
	// For a group simulation code makes itself. Never one a player could send, and
	// the same on every client.
	const uint16_t GenerateSimulationGroupNumber( const bool bSpecial = false );
	void RegisterGroup( const std::vector<int> &vIDs, const uint16_t wGroup, bool bPlayerCommand = false );
	void RegisterGroup( CObjectBase **pUnitsBuffer, const int nLen, const uint16_t wGroup );
	void UnregisterGroup( const uint16_t wGroup );

	void DelUnitFromGroup( class CCommonUnit *pUnit );
	void AddUnitToGroup( class CCommonUnit *pUnit, const int nGroup );
	void DelUnitFromSpecialGroup( class CCommonUnit *pUnit );
	void DetachTransferredUnit( CCommonUnit *pUnit );

	void GroupCommand( const SAIUnitCmd &command, const uint16_t wGroup, bool bPlaceInQueue );
	void UnitCommand( const SAIUnitCmd &command, class CCommonUnit *pGroupUnit, bool bPlaceInQueue );
	void InsertUnitCommand( const SAIUnitCmd &command, class CCommonUnit *pUnit );
	void PushFrontUnitCommand( const SAIUnitCmd &command, class CCommonUnit *pUnit );

	// послать updates на shoot areas для всех юнитов в группе
	void UpdateAllAreas( const std::vector<int> &units, const EActionNotify eAction );

	void Segment();

	void CreateSpecialGroup( const uint16_t wGroup );
	void UnregisterSpecialGroup( const uint16_t wSpecialGroup );

	int BeginGroup( const int nGroup ) const
	{
		return groupUnits.begin( nGroup );
	}

	int EndGroup() const
	{
		return groupUnits.end();
	}

	int Next( const int nIter ) const
	{
		return groupUnits.GetNext( nIter );
	}

	class CCommonUnit *GetGroupUnit( const int nIter ) const
	{
		return groupUnits.GetEl( nIter );
	}

	void AddFollowingUnit( class CCommonUnit *pUnit );

	void RegisterSegments( class CCommonUnit *pUnit, const bool bInitialization, const bool bAllInfo );
	void RegisterPathSegments( class CAIUnit *pUnit, const bool bInitialization );

	void UnregisterSegments( class CCommonUnit *pUnit );
	void UnregisterPathSegments( class CAIUnit *pUnit );

	void UnitSetToAmbush( class CCommonUnit *pUnit );
	void GetCheckSum( uint32_t *ulChecksum );
	void UpdateDebugChecksums( FILE *f );
};

const CVec2 GetGoPointByCommand( const SAIUnitCmd &cmd );
