#pragma once

#include "../Server_Client_Common/CommonClientState.h"

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
		string szIP;
		int nPort;

		SAddressInfo() : nPort( 0 ) { }
	};

	hash_map<int, SAddressInfo> connections;
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
	hash_map<int, string> nickByID;
	hash_map<string, int> idByNick;
	hash_map<string, int> DBUserIDByNick; // cache for online users. use GetDBUserIDByNick instead!!

	hash_set<string> onLineNicks;

	hash_map<int, SCommonClientInfo> onLine;
	hash_map<int, SGameConnection> gameConnections;
	
	hash_map< string, CPtr<SLadderDBInfo> > ladderInfoCache;
	hash_map< string, int > ladderInfoCacheLockCounter;

	hash_map< int, hash_set<int> > ignoreList; // <recipient, sender>
	hash_map< int, hash_set<int> > friendList; // <player, list of friends >
	int nQueries;
	UINT64 dwQueriesCountTime;
	float fQueriesPerSecond;
	float fPrevQueriesPerSecond;

	int nMaxXP;
	string EscapeString( const string &szString ) const;
	
	void LoadIgnoreFriendList();
	void AddIgnoreFriendPairToDB( const int nRecipientDBUserID, const int nSenderDBUserID, EIgnoreFriendList eList );
	void DeleteIgnoreFriendPairFromDB( const int nRecipientDBUserID, const int nSenderDBUserID, EIgnoreFriendList eList );
	int GetDBUserIDbyNick( const string &szNick );
	hash_set<string> GetTableColumns( const string &szTableName );
	void GetRawLadderInfoFromDB( hash_map<string,int> *pInfo, const string &szNick );
	void PutRawLadderInfoToDB( const string &szNick, const hash_map<string,int> &ladderInfo );
	void ConvertLadderInfo( SLadderDBInfo *pInfo, hash_map<string,int> *pHashMap, const bool bReadFromHashMap ) const;
	void DBLogRawGameResult( const hash_map<string,int> &info );
public:
	CClients() { }
	CClients( MYSQL *pMySQL );

	bool IsCriticalBusy() const;
	float GetQPS() const { return fQueriesPerSecond; }
	void RecalcDBOverload();

	bool IsBadNick( const string &szNick );
	bool IsBannedNick( const string &szNick );
	bool IsBannedCDKey( const string &szCDKey );

	const string GetCDKey( const string &szNick );
	const string GetPassword( const string &szNick );

	bool IsCorrectCDKey( const string &szCDKey );
	bool IsNickRegistered( const string &szNick );
	bool IsOnLine( const string &szNick ) const;
	bool IsOnLine( const int nClientID ) const;
	bool IsCDKeyOnline( const string &szCDKey );

	void Register( const string &szNick, const string &szPassword, const string &szCDKey );
	void Register( const string &_szNick, const string &_szPassword, const string &_szCDKey, const string &_szEmail );
	const string GetEmail( const string &szNick );

	void SetOnLine( const string &szNick, const int nClientID );
	void SetOffLine( const int nClientID );
	void SetGameConnectInfo( const int nClientID, const int nConnection, const string &szIP, const int nGameConnectPort );
	bool GetGameConnectInfo( const int nClientID, const int nConnection, SGameConnection::SAddressInfo *pAddressInfo ) const;

	const bool GetNick( const int nClientID, string *pszNick ) const;
	const bool GetClientID( const string &szNick, int *pnClientID ) const;
	const bool GetCommonClientInfo( const int nClientID, SCommonClientInfo *pCommonClientInfo ) const;
	void SetCommonClientInfo( const int nClientID, const SCommonClientInfo &commonClientInfo );

	const hash_map<int, SCommonClientInfo>& GetOnLine() const { return onLine; }

	void AddIgnoreFriendPair( const int nRecipient, const string &szSender, EIgnoreFriendList eList );
	void DeleteIgnoreFriendPair( const int nRecipient, const string &szSender, EIgnoreFriendList eList );
	bool InIgnoreFriendList( const int nRecipient, const string &szSender, EIgnoreFriendList eList );
	list<string> GetIgnoreFriendList( const int nClient, EIgnoreFriendList eList );

	SLadderDBInfo* GetLadderInfoFromDB( const string &szNick );
	void PutLadderInfoToDB( const string &szNick );
	
	void DBLogGameResult( SLadderGameInfo *pGameInfo );
	//
	void Log( const int nClientID, const string &szMsg ) const;
	void DBLogServerStatistics( const vector<string> &names, const vector<float> &values );

	void LockLadderInfo( const string &szNick );
  void UnlockLadderInfo( const string &szNick );

	int GetMaxXP() const { return nMaxXP; }
};


