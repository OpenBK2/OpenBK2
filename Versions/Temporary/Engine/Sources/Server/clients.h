#pragma once

#include "Server_Client_Common/CommonClientState.h"

struct st_mysql;
typedef st_mysql MYSQL;
struct SLadderStatistics;
struct IStatisticsCollector;
struct SLadderDBInfo;
struct SLadderGameInfo;

struct SCommonClientInfo
{
	ECommonClientState eState;
	bool bWant2ReceiveChat;
	BYTE cLobbyID;
	int nGameID;

	SCommonClientInfo()
		: eState( ES_ONLINE ), bWant2ReceiveChat( true ), cLobbyID( 255 ), nGameID( -1 ) { }
};

struct SGameConnection
{
	struct SAddressInfo
	{
		std::string szIP;
		int nPort;

		SAddressInfo() : nPort( 0 ) { }
	};

	std::unordered_map<int, SAddressInfo> connections;
};
enum EIgnoreFriendList
{
	IGNORE_LIST = 0,
	FRIEND_LIST = 1
};

class CClients : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CClients );

	MYSQL *pMySQL;
	CObj<IStatisticsCollector> pStatisticsCollector;
	std::unordered_map<int, std::string> nickByID;
	std::unordered_map<std::string, int> idByNick;
	std::unordered_map<std::string, int> DBUserIDByNick; // cache for online users. use GetDBUserIDByNick instead!!

	std::unordered_set<std::string> onLineNicks;

	std::unordered_map<int, SCommonClientInfo> onLine;
	std::unordered_map<int, SGameConnection> gameConnections;
	
	std::unordered_map< std::string, CPtr<SLadderDBInfo> > ladderInfoCache;
	std::unordered_map< std::string, int > ladderInfoCacheLockCounter;

	std::unordered_map< int, std::unordered_set<int> > ignoreList; // <recipient, sender>
	std::unordered_map< int, std::unordered_set<int> > friendList; // <player, list of friends >
	int nQueries;
	UINT64 dwQueriesCountTime;
	float fQueriesPerSecond;
	float fPrevQueriesPerSecond;

	int nMaxXP;
	std::string EscapeString( const std::string &szString ) const;
	
	void LoadIgnoreFriendList();
	void AddIgnoreFriendPairToDB( const int nRecipientDBUserID, const int nSenderDBUserID, EIgnoreFriendList eList );
	void DeleteIgnoreFriendPairFromDB( const int nRecipientDBUserID, const int nSenderDBUserID, EIgnoreFriendList eList );
	int GetDBUserIDbyNick( const std::string &szNick );
	std::unordered_set<std::string> GetTableColumns( const std::string &szTableName );
	void GetRawLadderInfoFromDB( std::unordered_map<std::string,int> *pInfo, const std::string &szNick );
	void PutRawLadderInfoToDB( const std::string &szNick, const std::unordered_map<std::string,int> &ladderInfo );
	void ConvertLadderInfo( SLadderDBInfo *pInfo, std::unordered_map<std::string,int> *pHashMap, const bool bReadFromHashMap ) const;
	void DBLogRawGameResult( const std::unordered_map<std::string,int> &info );
public:
	CClients() { }
	CClients( MYSQL *pMySQL );

	bool IsCriticalBusy() const;
	float GetQPS() const { return fQueriesPerSecond; }
	void RecalcDBOverload();

	bool IsBadNick( const std::string &szNick );
	bool IsBannedNick( const std::string &szNick );
	bool IsBannedCDKey( const std::string &szCDKey );

	const std::string GetCDKey( const std::string &szNick );
	const std::string GetPassword( const std::string &szNick );

	bool IsCorrectCDKey( const std::string &szCDKey );
	bool IsNickRegistered( const std::string &szNick );
	bool IsOnLine( const std::string &szNick ) const;
	bool IsOnLine( const int nClientID ) const;
	bool IsCDKeyOnline( const std::string &szCDKey );

	void Register( const std::string &szNick, const std::string &szPassword, const std::string &szCDKey );
	void Register( const std::string &_szNick, const std::string &_szPassword, const std::string &_szCDKey, const std::string &_szEmail );
	const std::string GetEmail( const std::string &szNick );

	void SetOnLine( const std::string &szNick, const int nClientID );
	void SetOffLine( const int nClientID );
	void SetGameConnectInfo( const int nClientID, const int nConnection, const std::string &szIP, const int nGameConnectPort );
	bool GetGameConnectInfo( const int nClientID, const int nConnection, SGameConnection::SAddressInfo *pAddressInfo ) const;

	const bool GetNick( const int nClientID, std::string *pszNick ) const;
	const bool GetClientID( const std::string &szNick, int *pnClientID ) const;
	const bool GetCommonClientInfo( const int nClientID, SCommonClientInfo *pCommonClientInfo ) const;
	void SetCommonClientInfo( const int nClientID, const SCommonClientInfo &commonClientInfo );

	const std::unordered_map<int, SCommonClientInfo>& GetOnLine() const { return onLine; }

	void AddIgnoreFriendPair( const int nRecipient, const std::string &szSender, EIgnoreFriendList eList );
	void DeleteIgnoreFriendPair( const int nRecipient, const std::string &szSender, EIgnoreFriendList eList );
	bool InIgnoreFriendList( const int nRecipient, const std::string &szSender, EIgnoreFriendList eList );
	std::list<std::string> GetIgnoreFriendList( const int nClient, EIgnoreFriendList eList );

	SLadderDBInfo* GetLadderInfoFromDB( const std::string &szNick );
	void PutLadderInfoToDB( const std::string &szNick );
	
	void DBLogGameResult( SLadderGameInfo *pGameInfo );
	//
	void Log( const int nClientID, const std::string &szMsg ) const;
	void DBLogServerStatistics( const std::vector<std::string> &names, const std::vector<float> &values );

	void LockLadderInfo( const std::string &szNick );
  void UnlockLadderInfo( const std::string &szNick );

	int GetMaxXP() const { return nMaxXP; }
};


