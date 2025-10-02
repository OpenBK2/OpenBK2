#pragma once

#include "GameX/DBScenario.h"

#include "AILogic/ScenarioTracker.h"


// BEGIN new scenario tracker

namespace NDb
{
	struct SBackground;
}
class CMapObj;

typedef vector<CDBPtr<NDb::SMapInfo> > CMissions;
struct IScenarioTracker : public IAIScenarioTracker
{
	enum { tidTypeID = 0x11165340 };

	enum EReinforcementState { ERS_DISABLED, ERS_NOT_ENABLED, ERS_ENABLED };
	
	enum EStatisticsKind
	{
		ESK_TIME, // mission time at seconds (real time, not game time)
		ESK_CAMPAIGN_TIME, // campaign time at seconds (real time, not game time)
		
		ESK_EXP_EARNED,
		ESK_CAMPAIGN_EXP_CURRENT,
		ESK_CAMPAIGN_EXP_NEXT_LEVEL,
		
		ESK_UNITS_LOST,
		ESK_UNITS_KILLED,
		
		ESK_KEY_BUILDINGS_CAPTURED,
		ESK_REINFORCEMENTS_CALLED,
		
		ESK_SCORE,

		ESK_UNITS_LOST_PRICE,
		ESK_UNITS_KILLED_PRICE,

		ESK_ENEMY_UNITS_MAX_PRICE,

		ESK_CAMPAIGN_UNITS_LOST,
		ESK_CAMPAIGN_UNITS_KILLED,

		ESK_CAMPAIGN_MISSIONS_PASSED,

		ESK_TACTICAL_EFFICIENCY,
		ESK_STRATEGIC_EFFICIENCY,
	};
	
	struct SMissionStats
	{
		struct SOldReinf
		{
			ZDATA
			EReinforcementState eState;
			CDBPtr<NDb::SReinforcement> pDBReinf;
			ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pDBReinf); return 0; }
		};
		
		ZDATA
		vector< CDBPtr<NDb::SReinforcement> > bonusReinforcements;
		CDBPtr<NDb::SPlayerRank> pNewPlayerRank;
		list< CDBPtr<NDb::SMedal> > medals;
		vector<SOldReinf> oldReinfs;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&bonusReinforcements); f.Add(3,&pNewPlayerRank); f.Add(4,&medals); f.Add(5,&oldReinfs); return 0; }
	};
	
	// specific multiplayer info
	struct SMultiplayerInfo
	{
		enum ENetMode
		{
			ENM_LADDER_GAME,
			ENM_CUSTOM_GAME,
		};

		struct SPlayer
		{
			ZDATA
			wstring wszName;
			int nTeam; // diplomacy side
			ZSKIP//int nSide; // player slot index in MapInfo
			int nLevel;
			wstring wszRank;
			int nParty; 
			int nIndex;
			int nCountry;
			ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&nTeam); f.Add(5,&nLevel); f.Add(6,&wszRank); f.Add(7,&nParty); f.Add(8,&nIndex); f.Add(9,&nCountry); return 0; }
		};
		
		enum EMessageType
		{
			EMT_NEW_RANK,
			EMT_LOST_RANK,
			EMT_NEW_LEVEL,
			EMT_LOST_LEVEL,
			EMT_MEDAL,
		};
		
		struct SMessage
		{
			ZDATA
			EMessageType eType;
			wstring wszRank;
			int nLevel;
			int nXP;
			CDBPtr<NDb::STexture> pMedal;
			wstring wszMedalName;
			wstring wszMedalDesc;
			ZEND int operator&( IBinSaver &f ) { f.Add(2,&eType); f.Add(3,&wszRank); f.Add(4,&nLevel); f.Add(5,&nXP); f.Add(6,&pMedal); f.Add(7,&wszMedalName); f.Add(8,&wszMedalDesc); return 0; }
		};
		
		ZDATA
		vector<SPlayer> players;
		wstring wszGameType;
		ENetMode eNetMode;
		vector<SMessage> messages;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&players); f.Add(3,&wszGameType); f.Add(4,&eNetMode); f.Add(5,&messages); return 0; }
	};

	struct SLeaderInfo : public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( SLeaderInfo )
	public:
		struct SLeaderStatSet
		{
			ZDATA
			int nRank;
			float fExp;
			int nUnitsKilled;
			int nUnitsLost;
			float fExpDebt;
			ZEND int operator&( IBinSaver &f ) { f.Add(2,&nRank); f.Add(3,&fExp); f.Add(4,&nUnitsKilled); f.Add(5,&nUnitsLost); f.Add(6,&fExpDebt); return 0; }
			
			SLeaderStatSet() : fExpDebt( 0.0f ) {}
		};
		ZDATA
		wstring wszName;
		ZSKIP //SLeaderStatSet info;							// Current (in-mission) info
		ZSKIP //SLeaderStatSet storedInfo;				// Info before mission
		SLeaderStatSet info;							// Current (in-mission) info
		SLeaderStatSet storedInfo;				// Info before mission
		CDBPtr<NDb::STexture> pPicture;
		SLeaderStatSet lastSeenInfo;				// for interface visual effects purpose
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(5,&info); f.Add(6,&storedInfo); f.Add(7,&pPicture); f.Add(8,&lastSeenInfo); return 0; }
		SLeaderInfo() : wszName( L"" ) {}
	};
	
	struct SGenerateLeaderInfo
	{
		ZDATA
		// generated info
		ZSKIP //wstring wszFirstName;
		ZSKIP //wstring wszLastName;
		ZSKIP //CDBPtr<NDb::STexture> pPicture;
		
		// user info
		wstring wszFullName;
		
		// internal
		ZSKIP //int nFirstNameID;
		ZSKIP //int nLastNameID;
		ZSKIP //int nPictureID;
		int nID;

		// db info
		wstring wszName;
		CDBPtr<NDb::STexture> pPicture;
		ZEND int operator&( IBinSaver &f ) { f.Add(5,&wszFullName); f.Add(9,&nID); f.Add(10,&wszName); f.Add(11,&pPicture); return 0; }
		
		SGenerateLeaderInfo() : nID( -1 )
		{
		}
	};
	struct SUndoLeaderInfo
	{
		ZDATA
		NDb::EReinforcementType eReinf;

		ZSKIP //int nFirstNameValue;
		ZSKIP //int nLastNameValue;
		ZSKIP //int nPictureValue;

		int nValue;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&eReinf); f.Add(6,&nValue); return 0; }
	};
	
	struct SChapterReinf
	{
		ZDATA
		EReinforcementState eState;
		CDBPtr<NDb::SReinforcement> pDBReinf;
		bool bFromPrevChapter;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pDBReinf); f.Add(4,&bFromPrevChapter); return 0; }
	};
	
	struct SPlayerColor
	{
		ZDATA
		DWORD dwColor;
		CDBPtr<NDb::SBackground> pUnitFullInfo;
		int nColorIndex; // индекс цвета хитбара над юнитами на карте
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwColor); f.Add(3,&pUnitFullInfo); f.Add(4,&nColorIndex); return 0; }
		
		SPlayerColor() : dwColor( 0 ), nColorIndex( 0 ) {}
	};
	
	virtual bool IsCustomMission() const = 0;
	virtual bool IsCustomCampaign() const = 0;

	// objectives
	// Задания, про которые игрок уже знает
	virtual int GetKnownObjectiveCount() const = 0;
	// Возвращает ID задания
	virtual int GetKnownObjectiveID( const int nIndex ) = 0;

	virtual int GetObjectiveCount() const = 0;
	virtual enum EMissionObjectiveState GetObjectiveState( const int nID ) const = 0;
	// Задание, впервые изменившее состояние из EMOS_WAITING попадает в конец списка известных
	virtual void SetObjectiveState( const int nID, const EMissionObjectiveState eState ) = 0;
	virtual bool GetObjectivePlaces( int nID, vector<CVec3> *pPlaces ) const = 0;
	virtual void SetObjectiveObjects( int nID, const vector< CMapObj* > &objects ) = 0;
	virtual bool IsDynamicObjective( int nID ) const = 0;

	// mission
	virtual void MissionStart( const NDb::SMapInfo * pMission, const int nTechLevel = -1 ) = 0;
	virtual void CustomMissionStart( const NDb::SMapInfo * pMission, int nDifficulty, bool bTutorial ) = 0;
	virtual bool IsTutorialMission() const = 0;
	virtual void MissionCancel() = 0;
	virtual void MissionWin() = 0;
	virtual bool IsMissionWon() const = 0;
	virtual const NDb::SMapInfo * GetCurrentMission() const = 0;
	virtual int GetMissionRecommendedReinfCalls() const = 0;
	virtual bool IsOnlyRecommendedReinfCalls() const { return false; }
	virtual const NDb::SMapInfo* GetLastMission() const = 0;
	virtual const SMissionStats* GetMissionStats( const NDb::SMapInfo *pMission ) const { return 0; }
	virtual void GetReinforcementCallsInfo( int nPlayer, vector<int> *pCallsByType ) {}
	//{ CRAP - убрать из интерфейса, когда будет известно, что миссия запускается только после инициализации ScenarioTracker
	virtual void ClearMissionScriptVars() = 0;
	//}
	
	// campaign
	virtual void CampaignStart( const NDb::SCampaign * pCampaign, const int _nDifficulty, bool bIsTutorial, bool bCustom ) = 0;
	virtual bool IsCampaignActive() const = 0;
	virtual const NDb::SCampaign * GetCurrentCampaign() const = 0;
	virtual const NDb::SDifficultyLevel* GetDifficultyLevelDB() const = 0;
	virtual bool IsTutorialCampaign() const = 0;
	virtual void GetAllMissionStats( vector<const SMissionStats*> *pMissions ) const = 0;
	virtual NDb::EReinforcementType GetFavoriteReinf() const = 0;
	virtual void MarkFavoriteReinf( NDb::EReinforcementType eType ) = 0;
	virtual void SetLastVisiblePlayerStatsExp( float fCareer, float fNextRank ) = 0;
	virtual void GetLastVisiblePlayerStatsExp( float *pCareer, float *pNextRank ) const = 0;
	virtual wstring GetReinfName( NDb::EReinforcementType eType ) const = 0;
	
	// chapter
	virtual bool IsChapterActive() const = 0;
	virtual void NextChapter() = 0;
	virtual void GetEnabledMissions( CMissions * pMissions ) = 0;	// chapter must be started
	virtual void GetCompletedMissions( CMissions * pMissions ) = 0;	// chapter must be started
	virtual const NDb::SChapter * GetCurrentChapter() const = 0;
	virtual int GetMissionToEnableCount() const = 0;

	virtual EReinforcementState GetReinforcementEnableState( int nPlayer, NDb::EReinforcementType eType ) = 0;
	virtual void GetChapterCurrentReinforcements( vector<SChapterReinf> *pReinf, int nPlayer ) const = 0;

	virtual int GetReinforcementCallsLeftInChapter() const = 0;	// Calls left in chapter for player 0, no matter in mission or not
	virtual int GetReinforcementCallsOld() const = 0;	// Calls left in chapter before last mission, or 0 if chapter started
	virtual const NDb::SPlayerRank *GetPlayerRank() const = 0;
	virtual int GetPlayerRankIndex() const = 0;
	
	virtual int GetStatistics( int nPlayer, EStatisticsKind eKind ) const = 0;
	virtual void SetStatistics( int nPlayer, EStatisticsKind eKind, int nValue ) = 0;
	virtual void UpdateStatistics() = 0;
	virtual int GetScoreWithUpdate( int nPlayer ) = 0;
	//virtual bool IsMultiplayer() const = 0;
	virtual void SetMultiplayerInfo( const SMultiplayerInfo &info ) = 0;
	virtual SMultiplayerInfo* GetMultiplayerInfo() = 0;
	virtual bool IsNewReinf( NDb::EReinforcementType eType ) const = 0;
	// Player color
	virtual const SPlayerColor& GetPlayerColor( int nPlayer ) const = 0;

	// Changeable sides for MP
	virtual void SetPlayerParty( const int nPlayer, const int nNewSide ) = 0;
	virtual void SetPlayerSide( const int nPlayer, const int nNewTeam ) = 0;
	virtual void SetPlayerColour( const int nPlayer, const int nNewColour ) = 0;

	// Leaders (commanders)
	virtual const SLeaderInfo *GetLeaderInfo( const NDb::EReinforcementType eReinf ) const = 0;
	virtual void AutoGenerateLeaderInfo( SGenerateLeaderInfo *pInfo ) const = 0;
	virtual bool AssignLeader( const NDb::EReinforcementType eReinf, const SGenerateLeaderInfo &info, SUndoLeaderInfo *pUndo ) = 0;		// Return true if successful
	virtual void SetLeaderLastSeenInfo( const NDb::EReinforcementType eReinf, const SLeaderInfo::SLeaderStatSet &lastSeenInfo ) = 0;
	virtual float GetLeaderRankExp( NDb::EReinforcementType eType, int nLevel ) const = 0;

	virtual void UndoAssignLeader( const SUndoLeaderInfo &undo ) = 0;
	virtual const wstring& GetLeaderRankName( int nRank ) const = 0;
	virtual int GetAvailablePromotions() const = 0;
	virtual void SetAvailablePromotions( int nCount ) = 0;
};
IScenarioTracker * CreateScenarioTracker();
IScenarioTracker * CreateScenarioTrackerMultiplayer();

// END new scenario tracker


