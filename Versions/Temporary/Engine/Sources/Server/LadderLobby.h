#pragma once

#include "GameLobby.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Server_Client_Common/LadderLobbyPackets.h"
#include "Statistics.h"
#include "LadderStats.h"

class CLadderConsts;

class CLadderClient : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CLadderClient );
public:
	int nSide; // 0 - random, 1 - allies, 2 - fascist
	int nTeamSize; // players in team 
	bool bHistoricity;
	unsigned int uCheckSum;
	int nLevel;
	//SLadderStatistics dbInfo;
	std::unordered_set< int > techLevels;
	std::unordered_set< int > maps;
	UINT64 nStartTime;
	CLadderClient() {}
	bool CanPlay( int nMapID, int nTechLevel, const CLadderConsts *pConsts );
};

class CLadderCacheLocker : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CLadderCacheLocker )
	std::string szNick;
	CPtr<CClients> pClients;
	CLadderCacheLocker() {}
public:
	CLadderCacheLocker( CClients* _pClients, const std::string &_szNick );
	~CLadderCacheLocker();
};

struct SLadderGameInfo 
{
	std::list<int> team1Players;
	std::list<int> team2Players; // fascist
	std::unordered_map<int,std::string> nickByID;
	int nMapID;
	int nTechLevel;
	bool bHistoricity;
	UINT64 nStartTime;
	UINT64 nDeathTime;
	bool bIsDead;
	std::unordered_set<int> winners;
	std::unordered_map<int,int> playerRaces;
	std::unordered_map<int,std::vector<int> > reinfUsed;
	std::unordered_map<int,int> playerUnitsEff;
	std::unordered_map<int,int> playerKeyPointEff;
	std::unordered_map<int,int> unitsKilled;
	std::unordered_map<int,int> unitsLost;

	std::unordered_map<int,int> winXP;
	std::unordered_map<int,int> loseXP;
	std::unordered_set<int> updatedPlayers;
	bool bInvalid;
	enum EInvalidReason
	{
		OTHER_ERROR = 0,
		DIFFERENCE_DETECTED = 1,
		NO_WINNERS = 2,
		TOO_MANY_WINNERS = 3,
		INVALID_RACEINFO = 4,
		INVALID_RACE_NUMBER = 5,
		NEGATIVE_KILLED = 6,
		NEGATIVE_LOST = 7,
		TOO_MANY_REINFORCEMENTS = 8
	};
	EInvalidReason eInvalidReason;
// for DB logging
	std::unordered_map<int,int> teamLevels;
	std::unordered_map<int,CPtr<SLadderDBInfo> > playerInfo;
	std::unordered_map<int,std::vector<int> > mapsRequested;
	std::unordered_map<int,std::vector<int> > techsRequested;
	std::list< CPtr<CLadderCacheLocker> > lockers;
};

class CLadderLobby : public CGameLobby
{
	OBJECT_NOCOPY_METHODS( CLadderLobby )

	CPtr<CClients> pClients;
	std::unordered_map< int, std::string > nickByID;
	std::list<int> waitingList;
	std::unordered_map< int, CPtr<CLadderClient> > ladderClients;
	std::unordered_map< int, SLadderGameInfo > games;
	CPtr<CLadderConsts> pConsts;
	CObj<IStatisticsCollector> pStatisticsCollector;
	UINT64 nLastStepTime;

	std::string szCfgFile;

	void Initialize( const std::string &szCfgFile );
	bool MatchMakingStep();
	void UpdateGames();
	void CalcResults( const int nGameID );
	bool CheckGameResultIsFake( const int nGameID );
	void UpdatePlayerXP( SLadderGameInfo *pGameInfo, const int nPlayerID, SLadderDBInfo *pClientInfo, const bool bWin );


	bool CheckMedals( SLadderDBInfo *pInfo, const int nUnitsKilledInLastGame, const int nUnitsLostInLastGame );
	void SendLadderInfoToPlayer( const int nClientID, const std::string& szClientNick, const bool bFullStatistics );

protected:
	virtual const int GetLobbyID() const { return ERID_LADDER; }
	virtual void ClientEnterToLobby( const int nClientID );
	virtual void ClientLeaveLobby( const int nClientID );
	virtual void GetLobbyClients ( class CGetLobbyClientsPacket *pPacket ) { CPtr<CGetLobbyClientsPacket> p = pPacket; return; }
	virtual void GameDead( const int nGameID );
	virtual bool PlayerCanKickPlayer() const { return false; }
	virtual bool PlayerCanSeeGamesList() const { return false; }
	virtual bool PlayerCanUpdateGameInfo() const { return false; }
	virtual bool PlayerNeedSpecificGameInfo() const { return false; }
public:
	CLadderLobby() : CGameLobby() { }
	CLadderLobby( class CClients *_pClients, const std::string &szCfgFile )
		: CGameLobby( _pClients, szCfgFile ), pClients( _pClients ) { Initialize( szCfgFile ); }

	virtual bool Segment();
	void ReloadConfig();

	void CreateLadderGame( const SLadderGameInfo &gameInfo );
	// Packet Processors
	bool ProcessLadderInfoPacket( class CLadderInfoPacket *pPacket );
	bool ProcessLadderGameResultPacket( class CLadderGameResultPacket *pPacket );
	bool ProcessLadderStatisticsRequestPacket( class CLadderStatisticsRequestPacket *pPacket );
	bool ProcessSurrenderPacket( CLadderSurrenderPacket *pPacket );
};


