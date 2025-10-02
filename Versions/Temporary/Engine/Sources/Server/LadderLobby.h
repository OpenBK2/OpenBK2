#pragma once

#include "GameLobby.h"
#include "../Server_Client_Common/LobbiesIDs.h"
#include "../Server_Client_Common/LadderLobbyPackets.h"
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
	hash_set< int > techLevels;
	hash_set< int > maps;
	UINT64 nStartTime;
	CLadderClient() {}
	bool CanPlay( int nMapID, int nTechLevel, const CLadderConsts *pConsts );
};

class CLadderCacheLocker : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CLadderCacheLocker )
	string szNick;
	CPtr<CClients> pClients;
	CLadderCacheLocker() {}
public:
	CLadderCacheLocker( CClients* _pClients, const string &_szNick );
	~CLadderCacheLocker();
};

struct SLadderGameInfo 
{
	list<int> team1Players;
	list<int> team2Players; // fascist
	hash_map<int,string> nickByID;
	int nMapID;
	int nTechLevel;
	bool bHistoricity;
	UINT64 nStartTime;
	UINT64 nDeathTime;
	bool bIsDead;
	hash_set<int> winners;
	hash_map<int,int> playerRaces;
	hash_map<int,vector<int> > reinfUsed;
	hash_map<int,int> playerUnitsEff;
	hash_map<int,int> playerKeyPointEff;
	hash_map<int,int> unitsKilled;
	hash_map<int,int> unitsLost;

	hash_map<int,int> winXP;
	hash_map<int,int> loseXP;
	hash_set<int> updatedPlayers;
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
	hash_map<int,int> teamLevels;
	hash_map<int,CPtr<SLadderDBInfo> > playerInfo;
	hash_map<int,vector<int> > mapsRequested;
	hash_map<int,vector<int> > techsRequested;
	list< CPtr<CLadderCacheLocker> > lockers; 
};

class CLadderLobby : public CGameLobby
{
	OBJECT_NOCOPY_METHODS( CLadderLobby )

	CPtr<CClients> pClients;
	hash_map< int, string > nickByID;
	list<int> waitingList;
	hash_map< int, CPtr<CLadderClient> > ladderClients;
	hash_map< int, SLadderGameInfo > games;
	CPtr<CLadderConsts> pConsts;
	CObj<IStatisticsCollector> pStatisticsCollector;
	UINT64 nLastStepTime;

	string szCfgFile;

	void Initialize( const string &szCfgFile );
	bool MatchMakingStep();
	void UpdateGames();
	void CalcResults( const int nGameID );
	bool CheckGameResultIsFake( const int nGameID );
	void UpdatePlayerXP( SLadderGameInfo *pGameInfo, const int nPlayerID, SLadderDBInfo *pClientInfo, const bool bWin );


	bool CheckMedals( SLadderDBInfo *pInfo, const int nUnitsKilledInLastGame, const int nUnitsLostInLastGame );
	void SendLadderInfoToPlayer( const int nClientID, const string& szClientNick, const bool bFullStatistics );

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
	CLadderLobby( class CClients *_pClients, const string &szCfgFile )
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


