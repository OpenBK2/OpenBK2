#pragma once

#include "AILogic/B2AI.h"

#include <vector>

// Exercise the real multiplayer transport loop without loading a map.
// Only simulation stepping and player neutralization are observed here.
class CMPHeadlessAI : public IAILogic
{
	OBJECT_NOCOPY_METHODS( CMPHeadlessAI );

	bool bSuspended = false;

public:
	int nSegments = 0;
	std::vector<int> neutralized;

	void Suspend() override
	{
		bSuspended = true;
	}

	void Resume() override
	{
		bSuspended = false;
	}

	void Segment() override
	{
		++nSegments;
	}

	bool IsSuspended() const override
	{
		return bSuspended;
	}

	void ClearAI() override {}

	bool IsMissionLoaded() const override
	{
		return false;
	}

	void SetProgressHook( IProgressHook * ) override {}

	void Init( ICheckSumLog *, const NDb::SMapInfo *, const NDb::SAIGameConsts *, IAIScenarioTracker * ) override {}

	void InitAfterMapLoad( const NDb::SMapInfo * ) override {}

	void ToggleWarFog( const bool ) override {}

	int GetMiniMapWarFogSizeX() const override
	{
		return 0;
	}

	int GetMiniMapWarFogSizeY() const override
	{
		return 0;
	}

	const bool GetMiniMapUnitsInfo( std::vector<SMiniMapUnitInfo> & ) override
	{
		return false;
	}

	bool GetMiniMapWarForInfo( CArray2D<uint8_t> **, bool ) override
	{
		return false;
	}

	bool UpdateAcknowledgment( SAIAcknowledgment & ) override
	{
		return false;
	}

	bool UpdateAcknowledgment( SAIBoredAcknowledgement & ) override
	{
		return false;
	}

	bool IsCombatSituation() override
	{
		return false;
	}

	void PrepareUpdates() override {}

	CObjectBase *GetUpdate() override
	{
		return nullptr;
	}

	float GetZ( const CVec2 & ) const override
	{
		return 0.0f;
	}

	const bool GetIntersectionWithTerrain( CVec3 *, const CVec3 &, const CVec3 & ) const override
	{
		return false;
	}

	const uint32_t GetNormal( const CVec2 & ) const override
	{
		return 0;
	}

	ITerraAIObserver *CreateTerraAIObserver( const int, const int ) override
	{
		return nullptr;
	}

	void RegisterGroup( const std::vector<int> &, const int, bool ) override {}

	void UnregisterGroup( const int ) override {}

	void GroupCommand( SAIUnitCmd *, const uint16_t, bool ) override {}

	void UnitCommand( SAIUnitCmd *, const uint16_t, const int ) override {}

	void TransferUnits( const std::vector<int> &, int, int ) override {}

	const int GenerateGroupNumber() override
	{
		return 0;
	}

	void RequestBuildPreview( const EActionCommand, const CVec2 &, const CVec2 &, bool ) override {}

	void ShowAreas( const std::vector<int> &, EActionNotify &, bool ) override {}

	void GetShootAreas( int, SShootAreas * ) override {}

	const int WAR_FOG_FULL_UPDATE() const override
	{
		return 0;
	}

	const int VIS_POWER() const override
	{
		return 0;
	}

	void PickedObj( const int ) override {}

	void PickEmpty() override {}

	void SetGlobeScriptHandler( IGlobeScriptHandler * ) override {}

	CStaticMapHeights *GetHeights() const override
	{
		return nullptr;
	}

	void NetGameStarted() override {}

	bool IsNetGameStarted() const override
	{
		return false;
	}

	void WriteDetailedChecksumInfo( const char * ) override {}

	void SetMyDiplomacyInfo( const int, const int ) override {}

	void SetNPlayers( const int ) override {}

	void SetNetGame( const bool ) override {}

	void NeutralizePlayer( const int nPlayer ) override
	{
		neutralized.push_back( nPlayer );
	}

	const int GetPlayerDiplomacy( const int ) const override
	{
		return 0;
	}

	const int GetUnitCount( const int ) const override
	{
		return 0;
	}

	const bool HasPlayerNoUnits( const int ) const override
	{
		return true;
	}

	void SetNeedNewGroupNumber() override {}

	const bool NeedNewGroupNumber() const override
	{
		return false;
	}

	void ResetNeedNewGroupNumber() override {}

	const NDb::SAIGameConsts *GetAIConsts() const override
	{
		return nullptr;
	}

	void DumpAfterAssinc() const override {}
};
