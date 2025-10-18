#pragma once

#include "Misc/2Darray.h"
#include "dbmpconsts.h"
#include "ScenarioTracker.h"
#include "Misc/HashFuncs.h"

#include <zconf.h>

struct IScriptWrapper;

namespace NScenarioTracker
{
typedef std::unordered_map<int/*NDb::EReinforcementType*/, CDBPtr<NDb::SReinforcement> > CReinforcementTypes;
typedef std::vector<CDBPtr<NDb::SMapInfo> > CWonMissions;
typedef std::unordered_map<NDb::EReinforcementType, float, SEnumHash> CReinforcementXPs;
typedef std::unordered_map<NDb::EReinforcementType, int, SEnumHash> CReinforcementLevels;

void SearchAvalableReinforcements( const NDb::SMapInfo *pCurMission, CReinforcementTypes *pMissionReinf, CReinforcementTypes *pChapterReinf, int nPlayer );

// BEGIN new scenario tracker

class CScenarioTracker : public IScenarioTracker
{
	OBJECT_NOCOPY_METHODS( CScenarioTracker );
	typedef std::unordered_map<int/*NDb::EReinforcementType*/, CDBPtr<NDb::SReinforcement> > CReinforcementTypes;
	typedef std::vector<EReinforcementState> CReinforcementEnableStates;
	typedef std::vector<CDBPtr<NDb::SMapInfo> > CWonMissions;
	typedef std::unordered_map< CDBPtr<NDb::SMapInfo>, SMissionStats, SDBPtrHash > CMissionsStats;
	typedef std::unordered_map<int/*NDb::EReinforcementType*/, SLeaderInfo > CLeaderList;
	typedef std::vector<char> CKnownReinforcements;
	typedef std::vector< CPtr<CMapObj> > CObjectiveObjects;
	typedef std::unordered_map< int, CObjectiveObjects > CObjectivesObjects;
	
	struct SFavoriteReinf
	{
		ZDATA
		int nTotalCount;
		int nCurrentCount;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nTotalCount); f.Add(3,&nCurrentCount); return 0; }
		
		SFavoriteReinf() : nTotalCount( 0 ), nCurrentCount( 0 ) {}
	};
	
	ZDATA

	// reinforcements, that was given when chapter started
	CReinforcementTypes playerChapter;								
	CReinforcementTypes enemyChapter;
	int nReinforcementCallsLeftInChapter;
	int nReinforcementCallsUsed;
	int nMainEnemy;

	// current status
	CDBPtr<NDb::SCampaign> pCampaign;
	CDBPtr<NDb::SChapter> pChapter;
	CDBPtr<NDb::SMapInfo> pMission; //pMap;


	CReinforcementTypes playerMission;				// reinforcements given to current mission
	CReinforcementTypes enemyMission;
	bool bCampaignFinished;
	bool bChapterFinished;


	std::vector<EMissionObjectiveState> objectives;
	std::vector<int> known_objectives;

	CWonMissions wonMissions;

	ZSKIP //CReinforcementXPs playerReinfXPs;
		int nMainEnemyMissionCalls;

	CReinforcementEnableStates playerReinfPotential;

	bool bMissionWon;

	CArray2D<int> kills;
	ZSKIP //CReinforcementXPs playerXPAdds;
		ZSKIP //CReinforcementXPs playerXPLevels;
		ZSKIP //int nPlayerXP;
		CDBPtr<NDb::SAIGameConsts> pAIConsts;
	int nDifficulty;
	int nEnemyReinfCallsLeft;

	CDBPtr<NDb::SMapInfo> pLastMission;

	CMissionsStats missionsStats;

	std::vector< std::vector<int> > statistic;
	ZSKIP //int nPlayerXPAdds;

		ZSKIP //CArray2D<int> priceKills;

		CLeaderList leaders;

	CKnownReinforcements knownReinfs;

	ZSKIP //CReinforcementXPs playerReinfXPs;
	ZSKIP //CReinforcementXPs playerXPAdds;
	ZSKIP //CReinforcementXPs playerXPLevels;
	float fPlayerXP;
	float fPlayerXPAdds;
	CArray2D<float> priceKills;
	int nReinforcementCallsLeftOld;
	int nAvailablePromotions;
	ZSKIP //vector<int> leaderFirstNames;
	ZSKIP //vector<int> leaderLastNames;
	ZSKIP //vector<int> leaderPictures;
	ZSKIP //CReinforcementEnableStates chapterReinfPotential; // потенциально доступные подкрепления в данной главе
	std::vector<SChapterReinf> chapterCurrentReinfs;
	SPlayerColor playerColorUser;
	SPlayerColor playerColorFriend;
	SPlayerColor playerColorEnemy;
	SPlayerColor playerColorNeutral;
	int nReinforcementCallsLeftInMission;
	bool bIsTutorial;
	CObjectivesObjects objectivesObjects;
	int nMedalKillsGiven;
	int nMedalTacticsGiven;
	int nMedalEconomyGiven;
	int nMedalMunchkinGiven;
	std::vector<int> freeLeaders;
	std::vector<SFavoriteReinf> favoriteReinfs;
	float fLastVisiblePlayerStatsExpCareer;
	float fLastVisiblePlayerStatsExpNextRank;
	bool bIsCustomCampaign;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&playerChapter); f.Add(3,&enemyChapter); f.Add(4,&nReinforcementCallsLeftInChapter); f.Add(5,&nReinforcementCallsUsed); f.Add(6,&nMainEnemy); f.Add(7,&pCampaign); f.Add(8,&pChapter); f.Add(9,&pMission); f.Add(10,&playerMission); f.Add(11,&enemyMission); f.Add(12,&bCampaignFinished); f.Add(13,&bChapterFinished); f.Add(14,&objectives); f.Add(15,&known_objectives); f.Add(16,&wonMissions); f.Add(18,&nMainEnemyMissionCalls); f.Add(19,&playerReinfPotential); f.Add(20,&bMissionWon); f.Add(21,&kills); f.Add(25,&pAIConsts); f.Add(26,&nDifficulty); f.Add(27,&nEnemyReinfCallsLeft); f.Add(28,&pLastMission); f.Add(29,&missionsStats); f.Add(30,&statistic); f.Add(33,&leaders); f.Add(34,&knownReinfs); f.Add(38,&fPlayerXP); f.Add(39,&fPlayerXPAdds); f.Add(40,&priceKills); f.Add(41,&nReinforcementCallsLeftOld); f.Add(42,&nAvailablePromotions); f.Add(47,&chapterCurrentReinfs); f.Add(48,&playerColorUser); f.Add(49,&playerColorFriend); f.Add(50,&playerColorEnemy); f.Add(51,&playerColorNeutral); f.Add(52,&nReinforcementCallsLeftInMission); f.Add(53,&bIsTutorial); f.Add(54,&objectivesObjects); f.Add(55,&nMedalKillsGiven); f.Add(56,&nMedalTacticsGiven); f.Add(57,&nMedalEconomyGiven); f.Add(58,&nMedalMunchkinGiven); f.Add(59,&freeLeaders); f.Add(60,&favoriteReinfs); f.Add(61,&fLastVisiblePlayerStatsExpCareer); f.Add(62,&fLastVisiblePlayerStatsExpNextRank); f.Add(63,&bIsCustomCampaign); return 0; }
private:

	void GiveChapterReinforcement( CReinforcementTypes *pChapterReinf, int nPlayer ) const;
	void SearchPotentialReinforcements();
	void MapStart();
	void ApplyMissionBonus( const NDb::SChapterBonus *pBonus, SMissionStats *pMissionStats );

	int CalcScore( int nUnitsLost, int nUnitsKilled, int nLostSum, int nKilledSum, int nReinfCalled ) const;
	void UpdateStatistics( bool bWin );
	int GetPlayerRankIndex( int nXP ) const;
	int CalcPossibleEnemyExp( int nPlayer ) const;
	// returns true on levelup, returns rest of exp
	bool AddReinfExp( float *pExp, NDb::EReinforcementType eReinfType );
	// returns true on levelup, returns rest of exp
	bool AddLeaderExp( float *pExp, SLeaderInfo *pLeader, NDb::EReinforcementType eReinfType );
public:
	CScenarioTracker();
	~CScenarioTracker();

	// objectives
	// Задания, про которые игрок уже знает
	int GetKnownObjectiveCount() const;
	// Возвращает ID задания
	int GetKnownObjectiveID( const int nIndex );

	int GetObjectiveCount() const;
	enum EMissionObjectiveState GetObjectiveState( const int nID ) const;
	// Задание, впервые изменившее состояние из EMOS_WAITING попадает в конец списка известных
	void SetObjectiveState( const int nID, const EMissionObjectiveState eState );
	bool GetObjectivePlaces( int nID, std::vector<CVec3> *pPlaces ) const;
	void SetObjectiveObjects( int nID, const std::vector< CMapObj* > &objects );
	bool IsDynamicObjective( int nID ) const;

	// reinforcements
	int GetReinforcementCallsLeft( int nPlayer );
	void DecreaseReinforcementCallsLeft( int nPlayer, int nCalls );
	void IncreaseReinforcementCallsLeft( int nPlayer, int nCalls );
	int GetReinforcementCallsLeftInChapter() const;	// Calls left in chapter for player 0, no matter in mission or not
	int GetReinforcementCallsOld() const { return nReinforcementCallsLeftOld; }		// Calls left in chapter before last mission, or 0 if just started chapter
	void RegisterReinforcementCall( int nPlayer, NDb::EReinforcementType eType );

	int GetDifficultyLevel() const { return nDifficulty; }
	const NDb::SReinforcement * GetReinforcement( int nPlayer, NDb::EReinforcementType eType ) const;

	float GetReinforcementXP( int nPlayer, NDb::EReinforcementType eType ) const;
	void SetReinforcementXP( int nPlayer, NDb::EReinforcementType eType, float fXP );
	int GetReinforcementXPLevel( int nPlayer, NDb::EReinforcementType eType ) const;
	float GetReinforcementXPForLevel( NDb::EReinforcementType eType, int nXPLevel ) const;
	virtual bool GiveXP( const int nPlayer, NDb::EReinforcementType eReinf, const int nXP );		// Give to reinf and player
	virtual bool GiveXPToPlayer( const int nPlayer, const int nXP );														// Give to player only

	bool IsCustomMission() const { return pCampaign == 0; }
	bool IsCustomCampaign() const { return bIsCustomCampaign; }

	// mission
	void MissionStart( const NDb::SMapInfo *_pMission, const int nTechLevel = -1 );			// tech level is ignored
	void CustomMissionStart( const NDb::SMapInfo * pMission, int nDifficulty, bool bTutorial );
	bool IsTutorialMission() const { return bIsTutorial; }

	void MissionCancel();
	void MissionWin();
	bool IsMissionActive() const { return GetCurrentMission() != 0; }
	bool IsMissionWon() const { return bMissionWon; }
	const NDb::SMapInfo * GetCurrentMission() const { return pMission; }
	const NDb::SMapInfo* GetLastMission() const { return pLastMission; }
	const SMissionStats* GetMissionStats( const NDb::SMapInfo *pMission ) const;
	int GetMissionRecommendedReinfCalls() const;
	bool IsOnlyRecommendedReinfCalls() const;
	void ClearMissionScriptVars();
	// mission statistics
	int GetNPlayers() const;
	int GetPlayerSide( int nPlayer ) const;

	// campaign
	void CampaignStart( const NDb::SCampaign *_pCampaign, const int _nDifficulty, bool bIsTutorial, bool bCustom );
	bool IsCampaignActive() const { return !bCampaignFinished && pCampaign != 0; }
	const NDb::SCampaign * GetCurrentCampaign() const { return pCampaign; }
	const NDb::SDifficultyLevel* GetDifficultyLevelDB() const;
	bool IsTutorialCampaign() const { return bIsTutorial; }
	void GetAllMissionStats( std::vector<const SMissionStats*> *pMissions ) const;
	NDb::EReinforcementType GetFavoriteReinf() const;
	void MarkFavoriteReinf( NDb::EReinforcementType eType );
	void SetLastVisiblePlayerStatsExp( float fCareer, float fNextRank );
	void GetLastVisiblePlayerStatsExp( float *pCareer, float *pNextRank ) const;
	std::wstring GetReinfName( NDb::EReinforcementType eType ) const;

	// chapter
	bool IsChapterActive() const { return !bChapterFinished && pChapter != 0; }
	void NextChapter();
	void GetEnabledMissions( CMissions * pMissions );
	void GetCompletedMissions( CMissions * pMissions );
	const NDb::SChapter * GetCurrentChapter() const { return pChapter; }
	int GetMissionToEnableCount() const;

	// check if reinforcement is potentially active
	virtual EReinforcementState GetReinforcementEnableState( int nPlayer, NDb::EReinforcementType eType );
	void GetChapterCurrentReinforcements( std::vector<SChapterReinf> *pReinf, int nPlayer ) const;

	virtual bool RegisterUnitKill( const SKillInfo &info );
	virtual int GetUnitKills( const int nPlayer ) const;
	virtual int GetUnitKills( const int nPlayer, const int nKilledPlayer ) const;
	int GetUnitPriceKills( const int nPlayer, const int nKilledPlayer ) const;
	virtual const NDb::SUnitStatsModifier *GetEnemyDifficultyModifier();
	virtual const NDb::SUnitStatsModifier *GetPlayerDifficultyModifier();
	virtual const float GetEnemyDifficultyRCallsModifier();
	virtual const float GetEnemyDifficultyRTimeModifier();

	virtual const NDb::SPlayerRank *GetPlayerRank() const;
	virtual int GetPlayerRankIndex() const;
	virtual void AddPlayer( const int nPlayer ) {}
	virtual void RemovePlayer( const int nPlayer ) {}
	virtual bool IsPlayerPresent( const int nPlayer ) const;
	int GetLocalPlayer() const { return 0; }
	void SetLocalPlayer( int nPlayer ) { NI_ASSERT( nPlayer == 0, "Wrong local player" ); }

	virtual void KeyBuildingOwnerChange( const int nBuildingLinkID, const int nNewOwnerPlayer ) {}
	virtual const int GetKeyBuildingOwner( const int nBuildingLinkID ) { return 2; }
	virtual const std::pair<int,int> GetKeyBuildingSummary() { return std::pair<int,int>( 0, 0 ); }
	virtual const float GetRecycleSpeedCoeff( const int nSide ) { return 1.0f; }

	int GetStatistics( int nPlayer, EStatisticsKind eKind ) const;
	void SetStatistics( int nPlayer, EStatisticsKind eKind, int nValue );
	void UpdateStatistics() {}
	virtual int GetScoreWithUpdate( int nPlayer ) { return 0; }
	void SetMultiplayerInfo( const SMultiplayerInfo &info ) {}
	SMultiplayerInfo* GetMultiplayerInfo() { return 0; }
	bool IsNewReinf( NDb::EReinforcementType eType ) const;
	const SPlayerColor& GetPlayerColor( int nPlayer ) const;
	void SetPlayerColour( const int nPlayer, const int nNewColour ) { }

	// Changeable sides for MP
	virtual void SetPlayerParty( const int nPlayer, const int nNewSide ) { NI_ASSERT( 0, "Wrong call to Scenartio Tracker Single" ); }
	virtual const NDb::SPartyDependentInfo *GetPlayerParty( const int nPlayer );
	virtual void SetPlayerSide( const int nPlayer, const int nNewTeam ) { NI_ASSERT( 0, "Wrong call to Scenartio Tracker Single" ); }

	// Leaders (commanders)
	const SLeaderInfo *GetLeaderInfo( const NDb::EReinforcementType eReinf ) const;
	void AutoGenerateLeaderInfo( SGenerateLeaderInfo *pInfo ) const;
	bool AssignLeader( const NDb::EReinforcementType eReinf, const SGenerateLeaderInfo &info, SUndoLeaderInfo *pUndo );		// Return true if successful
	void SetLeaderLastSeenInfo( const NDb::EReinforcementType eReinf, const SLeaderInfo::SLeaderStatSet &lastSeenInfo );
	float GetLeaderRankExp( NDb::EReinforcementType eType, int nLevel ) const;
	void UndoAssignLeader( const SUndoLeaderInfo &undo );
	int GetLeaderLevel( int nPlayer, NDb::EReinforcementType eType );
	const NDb::SUnitStatsModifier *GetLeaderModifier( int nPlayer, NDb::EReinforcementType eType );
	const std::wstring& GetLeaderRankName( int nRank ) const;
	int GetAvailablePromotions() const;
	void SetAvailablePromotions( int nCount );

	const NDb::SUnitStatsModifier *GetPlayerChapterModifier( NDb::EReinforcementType eReinf );

	// Debug
	bool DebugAssignLeader( NDb::EReinforcementType eReinf, const std::wstring &wszName )
	{
		SGenerateLeaderInfo info;
		info.wszFullName = wszName;
		SUndoLeaderInfo undo;
		return AssignLeader( eReinf, info, &undo );
	}
};

// CScenarioTrackerMultiplayer

class CScenarioTrackerMultiplayer : public IScenarioTracker
{
	OBJECT_BASIC_METHODS( CScenarioTrackerMultiplayer );
	struct SPlayerInfo
	{
		ZDATA
		CReinforcementTypes reinforcementTypes;
		int nReinforcementCallsLeft;
		bool bPresent;			// Not all players may exist in a particular game
		NTimer::STime timeKeyPointsOwned;
		int nMultiplayerSide;		// Index in MPConsts
		int nDiplomacySide;
		ZSKIP //CVec3 vColour;
		CReinforcementXPs experience;
		CReinforcementLevels reinfLevels;
		CReinforcementLevels leaderLevels;
		ZSKIP //DWORD dwColor;
		ZSKIP //CDBPtr<NDb::SBackground> pUnitFullInfoColor;
		SPlayerColor playerColor;
		int nReinforcementCallsUsed;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&reinforcementTypes); f.Add(3,&nReinforcementCallsLeft); f.Add(4,&bPresent); f.Add(5,&timeKeyPointsOwned); f.Add(6,&nMultiplayerSide); f.Add(7,&nDiplomacySide); f.Add(9,&experience); f.Add(10,&reinfLevels); f.Add(11,&leaderLevels); f.Add(14,&playerColor); f.Add(15,&nReinforcementCallsUsed); return 0; }

		SPlayerInfo() : nReinforcementCallsLeft( 0 ), nReinforcementCallsUsed( 0 ), bPresent( false ) { }
	};

	ZDATA
		CDBPtr<NDb::SMapInfo> pMission;
	std::vector<SPlayerInfo> players;

	ZSKIP //bool bMissionWon; // special serialization
		ZONSERIALIZE

		std::unordered_map<int, int> flags;
	bool bNoKeyBuildings;			// No key buildings to fight for, fight to death, limited number of calls

	std::vector< std::vector<int> > statistic;
	int nPlayerXPAdds;

	CArray2D<float> kills;
	CArray2D<int> priceKills;

	CDBPtr<NDb::SAIGameConsts> pAIConsts;
	int nTechLevel;
	std::unordered_map<int, NTimer::STime> flagTimes;
	int nNeutralPlayer;
	SMultiplayerInfo multiplayerInfo;
	ZSKIP //int nLocalPlayer; in OnSerialize()
		CDBPtr<NDb::SMapInfo> pLastMission;
	CDBPtr<NDb::SMultiplayerConsts> pMPConsts;
	EGameType eType;
	SPlayerColor playerColorNeutral;
	NTimer::STime timeMissionStart;
	CArray2D<int> reinfCallsByType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMission); f.Add(3,&players); OnSerialize( f ); f.Add(5,&flags); f.Add(6,&bNoKeyBuildings); f.Add(7,&statistic); f.Add(8,&nPlayerXPAdds); f.Add(9,&kills); f.Add(10,&priceKills); f.Add(11,&pAIConsts); f.Add(12,&nTechLevel); f.Add(13,&flagTimes); f.Add(14,&nNeutralPlayer); f.Add(15,&multiplayerInfo); f.Add(17,&pLastMission); f.Add(18,&pMPConsts); f.Add(19,&eType); f.Add(20,&playerColorNeutral); f.Add(21,&timeMissionStart); f.Add(22,&reinfCallsByType); return 0; }

	int nLocalPlayer;
	bool bMissionWon; // special serialization
	int nSecondsToReinf;
private:
	void OnSerialize( IBinSaver &f )
	{
		if ( !f.IsChecksum() )
		{
			f.Add(4,&bMissionWon); 
			f.Add(16,&nLocalPlayer);
		}
	}
	void CalculateScore( const int nPlayer );
	bool AddReinfExp( float *pExp, const int nPlayer, NDb::EReinforcementType eReinfType );
		// returns true on levelup, returns rest of exp

public:
	CScenarioTrackerMultiplayer();

	//{ IAIScenarioTracker
	virtual EGameType GetGameType() const { return eType; }
	virtual void SetGameType( EGameType _eType ) { eType = _eType; }

	// reinforcements
	const NDb::SReinforcement * GetReinforcement( int nPlayer, NDb::EReinforcementType eType ) const;
	int GetReinforcementCallsLeft( int nPlayer );
	void DecreaseReinforcementCallsLeft( int nPlayer, int nCalls );
	void IncreaseReinforcementCallsLeft( int nPlayer, int nCalls );
	void RegisterReinforcementCall( int nPlayer, NDb::EReinforcementType eType );
	void GetReinforcementCallsInfo( int nPlayer, std::vector<int> *pCallsByType );

	float GetReinforcementXP( int nPlayer, NDb::EReinforcementType eType ) const;
	void SetReinforcementXP( int nPlayer, NDb::EReinforcementType eType, float fXP );
	int GetReinforcementXPLevel( int nPlayer, NDb::EReinforcementType eType ) const;
	float GetReinforcementXPForLevel( NDb::EReinforcementType eType, int nXPLevel ) const;
	//}

	bool IsCustomMission() const { return false; }
	bool IsCustomCampaign() const { return false; }
	
	// objectives
	// Задания, про которые игрок уже знает
	int GetKnownObjectiveCount() const { return 0; }
	// Возвращает ID задания
	int GetKnownObjectiveID( const int nIndex ) { return -1; }

	int GetObjectiveCount() const { return 0; }
	enum EMissionObjectiveState GetObjectiveState( const int nID ) const;
	// Задание, впервые изменившее состояние из EMOS_WAITING попадает в конец списка известных
	void SetObjectiveState( const int nID, const EMissionObjectiveState eState ) {}
	bool GetObjectivePlaces( int nID, std::vector<CVec3> *pPlaces ) const { return false; }
	void SetObjectiveObjects( int nID, const std::vector< CMapObj* > &objects ) {}
	bool IsDynamicObjective( int nID ) const { return false; }

	// mission
	void MissionStart( const NDb::SMapInfo * pMission, const int _nTechLevel = -1 );
	void CustomMissionStart( const NDb::SMapInfo * pMission, int nDifficulty, bool bTutorial ) { NI_ASSERT( false, "wrong call" ); }
	bool IsTutorialMission() const { return false; }
	void MissionCancel();
	void MissionWin();
	bool IsMissionActive() const;
	bool IsMissionWon() const { return bMissionWon; }
	const NDb::SMapInfo * GetCurrentMission() const;
	const NDb::SMapInfo* GetLastMission() const { return pLastMission; }
	void ClearMissionScriptVars();

	// mission statistics
	int GetNPlayers() const;
	int GetPlayerSide( int nPlayer ) const;

	// campaign
	void CampaignStart( const NDb::SCampaign * pCampaign, const int _nDifficulty, bool bIsTutorial, bool bCustom ) { NI_ASSERT( false, "wrong call" ); }
	bool IsCampaignActive() const { NI_ASSERT( false, "wrong call" ); return false; }
	const NDb::SCampaign * GetCurrentCampaign() const { return 0; }
	const NDb::SDifficultyLevel* GetDifficultyLevelDB() const { return 0; }
	bool IsTutorialCampaign() const { return false; }
	void GetAllMissionStats( std::vector<const SMissionStats*> *pMissions ) const {}
	NDb::EReinforcementType GetFavoriteReinf() const { return NDb::_RT_NONE; }
	void MarkFavoriteReinf( NDb::EReinforcementType eType ) {}
	void SetLastVisiblePlayerStatsExp( float fCareer, float fNextRank ) {}
	void GetLastVisiblePlayerStatsExp( float *pCareer, float *pNextRank ) const {}
	std::wstring GetReinfName( NDb::EReinforcementType eType ) const { return std::wstring(); }

	// chapter
	bool IsChapterActive() const { NI_ASSERT( false, "wrong call" ); return false; }
	void NextChapter() { NI_ASSERT( false, "wrong call" ); }
	void GetEnabledMissions( CMissions * pMissions ) { NI_ASSERT( false, "wrong call" ); }
	void GetCompletedMissions( CMissions * pMissions ) { NI_ASSERT( false, "wrong call" ); }
	const NDb::SChapter * GetCurrentChapter() const { return 0; }
	int GetMissionToEnableCount() const { return 0; }

	virtual EReinforcementState GetReinforcementEnableState( int nPlayer, NDb::EReinforcementType eType ) { return ERS_DISABLED; }
	void GetChapterCurrentReinforcements( std::vector<SChapterReinf> *pReinf, int nPlayer ) const { pReinf->clear(); }
	int GetReinforcementCallsLeftInChapter() const { return 0; }
	int GetReinforcementCallsOld() const { return 0; }
	virtual bool RegisterUnitKill( const SKillInfo &info );
	virtual int GetUnitKills( const int nPlayer ) const;
	virtual int GetUnitKills( const int nPlayer, const int nKilledPlayer ) const;
	int GetUnitPriceKills( const int nPlayer, const int nKilledPlayer ) const;
	virtual const NDb::SUnitStatsModifier *GetEnemyDifficultyModifier() { return 0; }
	virtual const NDb::SUnitStatsModifier *GetPlayerDifficultyModifier() { return 0; }
	virtual const float GetEnemyDifficultyRCallsModifier() { return 0; }
	virtual const float GetEnemyDifficultyRTimeModifier() { return 0; }
	virtual const NDb::SPlayerRank *GetPlayerRank() const { return 0; }
	virtual int GetPlayerRankIndex() const { return 0; }


	virtual void AddPlayer( const int nPlayer );
	virtual void RemovePlayer( const int nPlayer );
	virtual bool IsPlayerPresent( const int nPlayer ) const;
	int GetLocalPlayer() const { NI_VERIFY( nLocalPlayer >= 0, "Wrong local player", return 0; ); return nLocalPlayer; }
	void SetLocalPlayer( int nPlayer ) { nLocalPlayer = nPlayer; }

	virtual void KeyBuildingOwnerChange( const int nBuildingLinkID, const int nNewOwnerPlayer );
	virtual const int GetKeyBuildingOwner( const int nBuildingLinkID );		// return side (0..2)
	virtual const std::pair<int,int> GetKeyBuildingSummary();
	virtual const float GetRecycleSpeedCoeff( const int nSide );
	virtual const NDb::SObjectBaseRPGStats *GetKeyBuildingFlagObject( const int nPlayer );

	int GetStatistics( int nPlayer, EStatisticsKind eKind ) const;
	void SetStatistics( int nPlayer, EStatisticsKind eKind, int nValue );
	void UpdateStatistics();
	virtual int GetScoreWithUpdate( int nPlayer );
	void SetMultiplayerInfo( const SMultiplayerInfo &info );
	SMultiplayerInfo* GetMultiplayerInfo();
	virtual int GetMissionRecommendedReinfCalls() const { return INFINITE_CALLS; }
	bool IsNewReinf( NDb::EReinforcementType eType ) const { return false; }
	const SPlayerColor& GetPlayerColor( int nPlayer ) const;

	// Changeable sides for MP
	virtual void SetPlayerParty( const int nPlayer, const int nNewSide );
	virtual const NDb::SPartyDependentInfo *GetPlayerParty( const int nPlayer );
	virtual void SetPlayerSide( const int nPlayer, const int nNewTeam );
	virtual const NDb::SReinforcement * GetStartUnits( int nPlayer ) const;
	virtual void SetPlayerColour( const int nPlayer, const int nNewColour );

	// Leaders (commanders)
	const SLeaderInfo *GetLeaderInfo( const NDb::EReinforcementType eReinf ) const;
	void AutoGenerateLeaderInfo( SGenerateLeaderInfo *pInfo ) const {}
	bool AssignLeader( const NDb::EReinforcementType eReinf, const SGenerateLeaderInfo &info, SUndoLeaderInfo *pUndo ) { NI_ASSERT( 0, "Wrong call to Scenartio Tracker MP" ); return false; }
	void SetLeaderLastSeenInfo( const NDb::EReinforcementType eReinf, const SLeaderInfo::SLeaderStatSet &lastSeenInfo ) {}
	float GetLeaderRankExp( NDb::EReinforcementType eType, int nLevel ) const { return 0.0f; }
	void UndoAssignLeader( const SUndoLeaderInfo &undo ) {}
	int GetLeaderLevel( int nPlayer, NDb::EReinforcementType eType ) { return GetReinforcementXPLevel( nPlayer, eType ); }
	const NDb::SUnitStatsModifier *GetLeaderModifier( int nPlayer, NDb::EReinforcementType eType ) { return 0; }
	const std::wstring& GetLeaderRankName( int nRank ) const { static std::wstring wszEmpty; return wszEmpty; }
	int GetAvailablePromotions() const { return 0; }
	void SetAvailablePromotions( int nCount ) {}

	void UpdateReinforcements( const int nPlayer );		// Find best reinforcement sets for current map/tech level

	virtual bool GiveXP( const int nPlayer, NDb::EReinforcementType eReinf, const int _nXP )
		{ float fXP = _nXP;	return AddReinfExp( &fXP, nPlayer, eReinf ); }		// Give to reinf and player
	virtual bool GiveXPToPlayer( const int nPlayer, const int nXP ) { return false; }														// Give to player only
	const NDb::SUnitStatsModifier *GetPlayerChapterModifier( NDb::EReinforcementType eReinf ) { return 0; }
};

// END new scenario tracker


} //namespace NScenarioTracker

