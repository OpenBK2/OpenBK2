#pragma once

namespace NDb
{
	struct SUnitStatsModifier;
	enum EDBUnitRPGType;
	struct SReinforcement;
	struct SPartyDependentInfo;
	enum EReinforcementType;
	struct SObjectBaseRPGStats;
}

struct IAIScenarioTracker : public CObjectBase
{
	enum { tidTypeID = 0x1116BC00 };

	enum { INFINITE_CALLS = -10 };

	// Types of scenario trackers (i.e. types of games)
	enum EGameType
	{
		EGT_SINGLE,
		EGT_MULTI_FLAG_CONTROL,
	};

	struct SKillInfo
	{
		int nPlayer;
		NDb::EDBUnitRPGType eUnitType;
		NDb::EReinforcementType eReinfType;
		int nKilledUnitPlayer;
		NDb::EDBUnitRPGType eKilledUnitType;
		NDb::EReinforcementType eKilledReinfType;
		float fExpPrice;
		bool bInfantryKill;
	};

	//virtual bool IsSingleplayer() const { return true; }
	virtual EGameType GetGameType() const { return EGT_SINGLE; }
	virtual void SetGameType( EGameType _eType ) { NI_ASSERT( 0, "Illegal call" ); }

	// reinforcements
	virtual const NDb::SReinforcement * GetReinforcement( int nPlayer, NDb::EReinforcementType eType ) const = 0;
	virtual int GetReinforcementCallsLeft( int nPlayer ) = 0;
	virtual void DecreaseReinforcementCallsLeft( int nPlayer, int nCalls ) = 0;		// if nCalls==0, then decrease by 1, but register as reinf call
	virtual void IncreaseReinforcementCallsLeft( int nPlayer, int nCalls ) { }
	virtual void RegisterReinforcementCall( int nPlayer, NDb::EReinforcementType eType ) = 0;
	
	virtual float GetReinforcementXP( int nPlayer, NDb::EReinforcementType eType ) const = 0;
	virtual void SetReinforcementXP( int nPlayer, NDb::EReinforcementType eType, float fXP ) = 0;
	virtual int GetReinforcementXPLevel( int nPlayer, NDb::EReinforcementType eType ) const = 0; // XPLevel: [0..]
	virtual float GetReinforcementXPForLevel( NDb::EReinforcementType eType, int nXPLevel ) const = 0; // XPLevel: [0..]

	virtual bool RegisterUnitKill( const SKillInfo &info ) = 0;	// Returns true if level-up occurred
	virtual int GetUnitKills( const int nPlayer ) const = 0;
	virtual int GetUnitKills( const int nPlayer, const int nKilledPlayer ) const = 0;

	virtual const NDb::SUnitStatsModifier *GetEnemyDifficultyModifier() = 0;
	virtual const NDb::SUnitStatsModifier *GetPlayerDifficultyModifier() = 0;
	virtual int GetDifficultyLevel() const { return 0; }
	virtual const float GetEnemyDifficultyRCallsModifier() = 0;
	virtual const float GetEnemyDifficultyRTimeModifier() = 0;
	virtual const NDb::SReinforcement * GetStartUnits( int nPlayer ) const { return 0; }

	// Player management - Mainly for multiplayer
	virtual void AddPlayer( const int nPlayer ) = 0;
	virtual void RemovePlayer( const int nPlayer ) = 0;
	virtual bool IsPlayerPresent( const int nPlayer ) const = 0;
	virtual int GetLocalPlayer() const = 0;
	virtual void SetLocalPlayer( int nPlayer ) = 0;
	virtual int GetNPlayers() const = 0;
	virtual int GetPlayerSide( int nPlayer ) const = 0;
	virtual const NDb::SPartyDependentInfo *GetPlayerParty( const int nPlayer ) = 0;

	virtual bool IsMissionActive() const = 0;

	// Key buildings (flags)
	virtual void KeyBuildingOwnerChange( const int nBuildingLinkID, const int nNewOwnerPlayer ) = 0;
	virtual const int GetKeyBuildingOwner( const int nBuildingLinkID ) = 0;		// return side (0..2)
	virtual const std::pair<int,int> GetKeyBuildingSummary() = 0;	// first = 0th side, second = 1st side buldings
	virtual const float GetRecycleSpeedCoeff( const int nSide ) = 0;
	virtual const NDb::SObjectBaseRPGStats *GetKeyBuildingFlagObject( const int nPlayer ) { return 0; }

	// Leaders (commanders)
	virtual int GetLeaderLevel( int nPlayer, NDb::EReinforcementType eType ) = 0; // return -1 if no leader
	virtual const NDb::SUnitStatsModifier *GetLeaderModifier( int nPlayer, NDb::EReinforcementType eType ) = 0;		// 0 if no leader

	// Debug
	virtual bool DebugAssignLeader( NDb::EReinforcementType eReinf, const std::wstring &wszName ) { return false; }

	virtual bool GiveXP( const int nPlayer, NDb::EReinforcementType eReinf, const int nXP ) = 0;		// Give to reinf and player
	virtual bool GiveXPToPlayer( const int nPlayer, const int nXP ) = 0;														// Give to player only
	virtual const NDb::SUnitStatsModifier *GetPlayerChapterModifier( NDb::EReinforcementType eReinf ) = 0;
};


