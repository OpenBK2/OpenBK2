#include "stdafx.h"
#include "Clients.h"
#include "Server_Client_Common/NetLogger.h"
#include "LadderLobby.h"
#include "vendor/MySQL/include/mysql.h"
#include "Misc/StrProc.h"
#include "Statistics.h"
#include "HashMapConvertor.h"
#include "LadderStats.h"
#include "Misc/Time64.h"
#include "System/RandomGen.h"

#include <zlib.h>

int NUMBER_OF_RACES_IN_LADDER = 4;
int MAX_NUMBER_OF_REINFORCEMENTS = 30;

const float QUERIES_CALC_INTERVAL = 2.0f;

BASIC_REGISTER_CLASS( CClients );

#define CHECK_TABLE_STRUCTURE

#define MYSQL_QUERY( a1, a2, a3 ) \
	{	/*DebugTrace( "MySQL: %s", a2 );*/ \
	++nQueries;RecalcDBOverload();\
	if ( mysql_real_query( a1, a2, a3 ) )\
	{ \
		DebugTrace( "Replaying last MySQL query: %s", a2 ); \
		if ( const int nMySQLResult = mysql_real_query( a1, a2, a3 ) )\
			{ NI_ASSERT( false, StrFmt( "MySQL query error, query = \"%s\", errorcode = %d", a2, nMySQLResult ) ); }\
	} \
	(*pStatisticsCollector)["QueriesPerSecond"]->Add( 1.0f );\
	}

#define MYSQL_CHECK_RESULT \
	if ( !pResult ) { DebugTrace( "MySQL: Invalid SQL Query !" ); } \
	NI_ASSERT( pResult, StrFmt( "Invalid SQL Query : %s", szQuery.c_str()) );

void CClients::RecalcDBOverload()
{
	const float fTimeInterval = float( GetLongTickCount() - dwQueriesCountTime ) / 1000.0f;
	if ( fTimeInterval > QUERIES_CALC_INTERVAL )
	{
		dwQueriesCountTime = GetLongTickCount(); 
		fPrevQueriesPerSecond = float( nQueries ) / QUERIES_CALC_INTERVAL;
		nQueries = 0;
		fQueriesPerSecond = 0.0f;
	}
	else
		fQueriesPerSecond = float( nQueries + fPrevQueriesPerSecond * QUERIES_CALC_INTERVAL ) / ( fTimeInterval + QUERIES_CALC_INTERVAL );
}

bool CClients::IsCriticalBusy() const
{
	if ( fQueriesPerSecond > 30.0f )
		return ( NRandom::Random( 0.0f, fQueriesPerSecond - 29.0f ) > 1.0f );
	else
		return false;
}

CClients::CClients( MYSQL *_pDB )
{
	pMySQL = _pDB;
	pStatisticsCollector = NStatistics::CreateCollector( "MySQL" );
	pStatisticsCollector->SetSpecific( "QueriesPerSecond", NStatistics::CreateAverageValuePerTimeCounter() );
	LoadIgnoreFriendList();
	nQueries = 0;
	dwQueriesCountTime = 0;
	fQueriesPerSecond = 0.0f;
	fPrevQueriesPerSecond = 0.0f;

	nMaxXP = 0;
	{
		const string szQuery = "SELECT MAX(`xp`) FROM gamestats";
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
		MYSQL_RES *pResult = 0;
		pResult = mysql_store_result( pMySQL );
		MYSQL_CHECK_RESULT
		if ( mysql_num_rows( pResult ) > 0 )
		{
			MYSQL_ROW row = mysql_fetch_row( pResult );
			if ( row[0] != NULL )
				nMaxXP = NStr::ToInt( row[0] );
		}
		mysql_free_result( pResult );
	}

}

int CClients::GetDBUserIDbyNick( const string &_szNick )
{
	hash_map<string, int>::const_iterator it = DBUserIDByNick.find( _szNick );
	if ( it != DBUserIDByNick.end() )
		return it->second;
	string szNick = EscapeString( _szNick );
	string szQuery = "SELECT t.userID FROM users AS t, names AS n WHERE ( n.Name = '" +
		szNick + "' AND t.name = n.nameID )";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = 0;
	pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	if ( mysql_num_rows( pResult ) > 0 )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		const string szDBUserID = row[0];
		mysql_free_result( pResult );
		return NStr::ToInt( szDBUserID );
	}
	else 
	{
		mysql_free_result( pResult );
		return -1;
	}
}

void CClients::LoadIgnoreFriendList()
{
	{
		string szQuery = "SELECT recipient, sender FROM ignorelist ";
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
		MYSQL_RES *pResult = 0;
		pResult = mysql_store_result( pMySQL );
		MYSQL_CHECK_RESULT
			int nRows = mysql_num_rows( pResult );
		for ( int i = 0; i < nRows; ++i )
		{
			MYSQL_ROW row = mysql_fetch_row( pResult );
			ignoreList[ NStr::ToInt( row[0] ) ].insert( NStr::ToInt( row[1] ) );
		}
		mysql_free_result( pResult );
		WriteMSG( "Server-side ignore list loaded.\n" );
	}

	{
		string szQuery = "SELECT player, notifier FROM friendlist ";
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
		MYSQL_RES *pResult = 0;
		pResult = mysql_store_result( pMySQL );
		MYSQL_CHECK_RESULT
			int nRows = mysql_num_rows( pResult );
		for ( int i = 0; i < nRows; ++i )
		{
			MYSQL_ROW row = mysql_fetch_row( pResult );
			friendList[ NStr::ToInt( row[0] ) ].insert( NStr::ToInt( row[1] ) );
		}
		mysql_free_result( pResult );
		WriteMSG( "Server-side friends list loaded.\n" );
	}
}

void CClients::AddIgnoreFriendPair( const int nRecipient, const string &szSender, EIgnoreFriendList eList )
{
	if ( IsOnLine( nRecipient ) )
	{
		string szNick;
		GetNick( nRecipient, &szNick );
		const int nRecipientDBUserID = GetDBUserIDbyNick( szNick );
		const int nSenderDBUserID = GetDBUserIDbyNick( szSender );
		if ( nRecipientDBUserID == -1 || nSenderDBUserID == -1 )
			return;
		switch( eList )
		{
		case IGNORE_LIST:
			ignoreList[ nRecipientDBUserID ].insert( nSenderDBUserID );
			break;
		case FRIEND_LIST:
			friendList[ nRecipientDBUserID ].insert( nSenderDBUserID );
		}

		AddIgnoreFriendPairToDB( nRecipientDBUserID, nSenderDBUserID, eList );
	}
}

void CClients::DeleteIgnoreFriendPair( const int nRecipient, const string &szSender, EIgnoreFriendList eList )
{
	if ( IsOnLine( nRecipient ) )
	{
		string szNick;
		GetNick( nRecipient, &szNick );
		const int nRecipientDBUserID = GetDBUserIDbyNick( szNick );
		const int nSenderDBUserID = GetDBUserIDbyNick( szSender );
		if ( nRecipientDBUserID == -1 || nSenderDBUserID == -1 )
			return;
		switch( eList )
		{
		case IGNORE_LIST:
			ignoreList[ nRecipientDBUserID ].remove( nSenderDBUserID );
		break;
		case FRIEND_LIST:
			friendList[ nRecipientDBUserID ].remove( nSenderDBUserID );
		}
		DeleteIgnoreFriendPairFromDB( nRecipientDBUserID, nSenderDBUserID, eList );
	}
}

void CClients::AddIgnoreFriendPairToDB( const int nRecipientDBUserID, const int nSenderDBUserID, EIgnoreFriendList eList )
{
	string szQuery;
	switch( eList )
	{
	case IGNORE_LIST:
		szQuery = StrFmt( "INSERT INTO ignorelist (recipient,sender) VALUES ( '%d', '%d' )", nRecipientDBUserID,
			nSenderDBUserID );
		break;
	case FRIEND_LIST:
		szQuery = StrFmt( "INSERT INTO friendlist (player,notifier) VALUES ( '%d', '%d' )", nRecipientDBUserID,
			nSenderDBUserID );
	}
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
}

void CClients::DeleteIgnoreFriendPairFromDB( const int nRecipientDBUserID, const int nSenderDBUserID, EIgnoreFriendList eList )
{
	string szQuery;
	switch( eList )
	{
	case IGNORE_LIST:
		szQuery = StrFmt( "DELETE FROM ignorelist WHERE recipient = '%d' AND sender = '%d'", nRecipientDBUserID,
			nSenderDBUserID );
		break;
	case FRIEND_LIST:
		szQuery = StrFmt( "DELETE FROM friendlist WHERE player = '%d' AND notifier = '%d'", nRecipientDBUserID,
			nSenderDBUserID );
	}
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
}

bool CClients::InIgnoreFriendList( const int nRecipient, const string &szSender, EIgnoreFriendList eList )
{
	if ( IsOnLine( nRecipient ) )
	{
		hash_map<int,string>::const_iterator it = nickByID.find( nRecipient );
		const int nRecipientDBID = GetDBUserIDbyNick( it->second );
		const int nSenderDBID = GetDBUserIDbyNick( szSender );
		{
      hash_map<int, hash_set<int> >::const_iterator it;
			switch( eList )
			{
			case IGNORE_LIST:
				{
					it = ignoreList.find( nRecipientDBID );
					if ( it == ignoreList.end() )
						return false;
					const hash_set<int> &recipientsIgnoreList = it->second;
					return recipientsIgnoreList.find( nSenderDBID ) != recipientsIgnoreList.end();
					break;
				}
			case FRIEND_LIST:
				{
					it = friendList.find( nRecipientDBID );
					if ( it == friendList.end() )
						return false;
					const hash_set<int> &recipientsFriendList = it->second;
					return recipientsFriendList.find( nSenderDBID ) != recipientsFriendList.end();
				}
			}
			return false;
		}
	}
	else
    return false;
}

list<string> CClients::GetIgnoreFriendList( const int nClient, EIgnoreFriendList eList )
{
	if ( IsOnLine( nClient ) )
	{
		string szQuery;
		switch( eList )
		{
		case IGNORE_LIST:
			szQuery = StrFmt( "SELECT names.Name FROM ignorelist, users, names WHERE users.userID = ignorelist.sender AND names.nameID = users.name AND ignorelist.recipient = '%d'" ,
				GetDBUserIDbyNick( nickByID[nClient] ) );
			break;
		case FRIEND_LIST:
			szQuery = StrFmt( "SELECT names.Name FROM friendlist, users, names WHERE users.userID = friendlist.notifier AND names.nameID = users.name AND friendlist.player = '%d'" ,
				GetDBUserIDbyNick( nickByID[nClient] ) );
		}
			
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
		MYSQL_RES *pResult = 0;
		pResult = mysql_store_result( pMySQL );
		MYSQL_CHECK_RESULT
		int nRows = mysql_num_rows( pResult );
		list<string> clientsIgnoreFriendList;
		for ( int i = 0; i < nRows; ++i )
		{
			MYSQL_ROW row = mysql_fetch_row( pResult );
			clientsIgnoreFriendList.push_back( row[0] );
		}
		mysql_free_result( pResult );		
		return clientsIgnoreFriendList;
	}
	else
		return list<string>();
}

bool CClients::IsCorrectCDKey( const string &szCDKey )
{
	const string szQuery = "SELECT cdkey FROM validCDKeys WHERE cdkey = '" + EscapeString( szCDKey ) + "'";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	const bool bIsValid = ( mysql_num_rows( pResult ) > 0 );
	mysql_free_result( pResult );
	return bIsValid;
}

bool CClients::IsBadNick( const string &szNick )
{
	if ( szNick.empty() )
		return true;
	// TODO:
	// Нужна проверка ника на соответствие требованиям

	return	false;
}

bool CClients::IsBannedNick( const string &szNick )
{
	bool ans = false;
	string szQuery = "SELECT banned FROM names WHERE Name = '" + EscapeString( szNick ) + "'";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = 0;
	pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	if ( mysql_num_rows( pResult ) > 0 )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		if ( row[0] == "1" )
			ans = true;
	}
	
	mysql_free_result( pResult );
	return ans;
}

bool CClients::IsBannedCDKey( const string &szCDKey )
{
	bool ans = false;
	string szQuery = "SELECT banned FROM CDKeys WHERE CDKey = '" + EscapeString( szCDKey ) + "'";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = 0;
	pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	if ( mysql_num_rows( pResult ) > 0 )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		if ( row[0] == "1" )
			ans = true;
	}

	mysql_free_result( pResult );
	return ans;
}

const string CClients::GetCDKey( const string &szNick )
{
	if ( szNick.empty() )
		return "";
	string szQuery = "SELECT cdkeys.CDKey FROM cdkeys, users, names WHERE ( cdkeys.cdkeyID = users.cdkey AND names.nameID = users.name AND names.Name = '" +
		EscapeString( szNick ) + "')";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT;
	string ans = "";
	if ( mysql_num_rows( pResult ) > 0 )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		ans = row[0];
	}
	mysql_free_result( pResult );
	return ans;
}

const string CClients::GetPassword( const string &szNick )
{
	if ( szNick.empty() )
		return "";

	string szQuery = "SELECT users.password FROM users, names WHERE ( users.name = names.nameID AND names.Name = '" +
		EscapeString( szNick ) + "')";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT;
	string ans = "";
	if ( mysql_num_rows( pResult ) > 0 )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		ans = row[0];
	}
	mysql_free_result( pResult );
	return ans;
}

const string CClients::GetEmail( const string &szNick )
{
	if ( szNick.empty() )
		return "";

	string szQuery = "SELECT users.email FROM users, names WHERE ( users.name = names.nameID AND names.Name = '" +
		EscapeString( szNick ) + "')";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT;
	string ans = "";
	if ( mysql_num_rows( pResult ) > 0 )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		ans = row[0];
	}
	mysql_free_result( pResult );
	return ans;
}

bool CClients::IsNickRegistered( const string &szNick )
{
	if ( szNick.empty() )
		return false;
	string szQuery = "SELECT nameID FROM names WHERE Name = '" + EscapeString( szNick ) + "'";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = 0;
	pResult = mysql_store_result( pMySQL );
	if ( pResult && mysql_num_rows( pResult ) > 0 )
	{
		mysql_free_result( pResult );
		return true;	
	}
	mysql_free_result( pResult );
	return false;
}

void CClients::Register( const string &_szNick, const string &_szPassword, const string &_szCDKey )
{
	DebugTrace( "Registration without email address!!" );
	Register( _szNick, _szPassword, _szCDKey, "nobody@nowhere.org" );
}

void CClients::Register( const string &_szNick, const string &_szPassword, const string &_szCDKey, const string &_szEmail )
{
	if ( _szNick.empty() )
		return;

	string szNick = EscapeString( _szNick );
	string szPassword = EscapeString( _szPassword );
	string szCDKey = EscapeString( _szCDKey );
	string szEmail = EscapeString( _szEmail );
	string szQuery = "SELECT cdkeyID FROM cdkeys WHERE CDKey = '" + szCDKey + "'";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = 0;
	pResult = mysql_store_result( pMySQL );

	if ( mysql_num_rows( pResult ) == 0 ) 
	{
		mysql_free_result( pResult );
		szQuery = "INSERT INTO cdkeys (CDKey) VALUES ('" + szCDKey + "')";
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );

		szQuery = "SELECT cdkeyID FROM cdkeys WHERE CDKey = '" + szCDKey + "'";
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
		pResult = mysql_store_result( pMySQL );
		MYSQL_CHECK_RESULT
	}
	MYSQL_ROW row = mysql_fetch_row( pResult );
	string szCDKeyID = row[0];
	mysql_free_result( pResult );

	szQuery = "INSERT INTO names (Name) VALUES ('" + szNick + "')";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	szQuery = "SELECT MAX(nameID) FROM names";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	row = mysql_fetch_row( pResult );
	string szNameID = row[0];
	mysql_free_result( pResult );

	szQuery = "INSERT INTO gamestats (xp) VALUES ('0')";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	szQuery = "SELECT MAX(statsID) FROM gamestats";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	row = mysql_fetch_row( pResult );
	string szStatsID = row[0];
	mysql_free_result( pResult );

	szQuery = "INSERT INTO users (name, cdkey, email, password, gamestats) VALUES ('"+ szNameID + "','" +
		szCDKeyID + "','" + szEmail + "','"	+ szPassword + "','" + szStatsID + "')";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );

}

SLadderDBInfo* CClients::GetLadderInfoFromDB( const string &szNick )
{
	hash_map<string, CPtr<SLadderDBInfo> >::const_iterator it = ladderInfoCache.find( szNick );
	if ( it == ladderInfoCache.end() )
	{
		static hash_map<string,int> rawData;
		GetRawLadderInfoFromDB( &rawData, szNick );
		static CPtr<SLadderDBInfo> pInfo;
		pInfo = new SLadderDBInfo();
		ConvertLadderInfo( pInfo, &rawData, true );
		if ( IsOnLine( szNick ) )
		{
			ladderInfoCache[szNick] = pInfo;
		}
		return pInfo;
	}
	else
	{
		return it->second;
	}
}

void CClients::PutLadderInfoToDB( const string &szNick )
{
	hash_map< string, CPtr<SLadderDBInfo> >::const_iterator it = ladderInfoCache.find( szNick );
	if ( it != ladderInfoCache.end() )
	{
		SLadderDBInfo* pInfo = it->second;
		static hash_map<string,int> rawData;
		ConvertLadderInfo( pInfo, &rawData, false );
		PutRawLadderInfoToDB( szNick, rawData );
		nMaxXP = Max( nMaxXP, pInfo->nXP );
	}
}

void CClients::LockLadderInfo( const string &szNick )
{
	if ( ladderInfoCache.find( szNick ) == ladderInfoCache.end() )
		return;
	++ladderInfoCacheLockCounter[szNick];
}

void CClients::UnlockLadderInfo( const string &szNick )
{
	hash_map<string,int>::iterator it = ladderInfoCacheLockCounter.find( szNick );
	if ( it != ladderInfoCacheLockCounter.end() )
	{
		int &nCounter = it->second;
		--nCounter;
		if ( nCounter > 0 )
			return;
		ladderInfoCacheLockCounter.erase( it );
		if ( !IsOnLine( szNick ) )
		{
			ladderInfoCache.erase( szNick );
		}
	}
}

void CClients::ConvertLadderInfo( SLadderDBInfo *pInfo, hash_map<string,int> *pHashMap, const bool bReadFromHashMap ) const
{
#define CONVERT_NUMBER( name, var ) NHashMapConvertor::ConvertNumber( pHashMap, name, &(pInfo->var), bReadFromHashMap )
#define CONVERT_VECTOR( name, var, nSize ) NHashMapConvertor::ConvertVector( pHashMap, name, &(pInfo->var), bReadFromHashMap );\
	if ( pInfo->var.size() < nSize ) pInfo->var.resize( nSize, 0 )

	CONVERT_NUMBER( "xp", nXP );
	CONVERT_NUMBER( "level", nLevel );
	CONVERT_NUMBER( "maxxpearned", nMaxXPEarned );
	CONVERT_NUMBER( "maxxplost", nMaxXPLost );
	CONVERT_NUMBER( "unitskilled", nUnitsKilled );
	CONVERT_NUMBER( "unitslost", nUnitsLost );
	CONVERT_NUMBER( "unitseff", nUnitsEffectiveness );
	CONVERT_NUMBER( "keypointeff", nKeyPointsEffectiveness );
	CONVERT_NUMBER( "totalplaytime", nTotalPlayTime );
	CONVERT_NUMBER( "totalgamesplayed", nTotalGamesPlayed );
	CONVERT_NUMBER( "winsinseries", nWinsInSeries );
	CONVERT_VECTOR( "racewinssolo", raceWinsSolo, NUMBER_OF_RACES_IN_LADDER );
	CONVERT_VECTOR( "racelossessolo", raceLossesSolo, NUMBER_OF_RACES_IN_LADDER );
	CONVERT_VECTOR( "racewinsteam", raceWinsTeam, NUMBER_OF_RACES_IN_LADDER );
	CONVERT_VECTOR( "racelossesteam", raceLossesTeam, NUMBER_OF_RACES_IN_LADDER );
	CONVERT_VECTOR( "reinforcementused", reinforcementUsed, MAX_NUMBER_OF_REINFORCEMENTS );
	CONVERT_VECTOR( "winsagainst", winsAgainst, NUMBER_OF_RACES_IN_LADDER );
	CONVERT_VECTOR( "lossesagainst", lossesAgainst, NUMBER_OF_RACES_IN_LADDER );
	CONVERT_VECTOR( "mapsplayed", mapsPlayed, 0 );
	CONVERT_VECTOR( "techsplayed", techsPlayed, 0 );
	CONVERT_VECTOR( "medals", medals, NUMBER_OF_RACES_IN_LADDER );

#undef CONVERT_NUMBER
#undef CONVERT_VECTOR
}

void CClients::GetRawLadderInfoFromDB( hash_map<string,int> *pInfo, const string &szNick )
{
	pInfo->clear();
	const string szClientDBID = StrFmt( "%d", GetDBUserIDbyNick( szNick ) );
	const string szQuery = "SELECT g.* FROM gamestats AS g, users AS u WHERE u.gamestats = g.statsID AND u.userID = '" + szClientDBID + "'";
  MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	if ( mysql_num_rows( pResult ) > 0 )
	{
		const MYSQL_ROW row = mysql_fetch_row( pResult );
		int i = 0;
		MYSQL_FIELD *pField;
		while ( pField = mysql_fetch_field( pResult ) )
		{
			(*pInfo)[pField->name] = NStr::ToInt( row[i] );
			++i;
		}
	}
	mysql_free_result( pResult );
}

void CClients::PutRawLadderInfoToDB( const string &szNick, const hash_map<string,int> &ladderInfo )
{
	const int nClientDBID = GetDBUserIDbyNick( szNick );
	const string szClientDBID = StrFmt( "%d", nClientDBID );
	{
		hash_map<string,int>::const_iterator statsIDiter = ladderInfo.find( "statsID" );
		if ( statsIDiter != ladderInfo.end() && statsIDiter->second != nClientDBID )
		{
			DebugTrace( "PutLadderInfoToDB: invalid ClientDBID detected!" );
			return;
		}
	}
#ifdef CHECK_TABLE_STRUCTURE
	{
		hash_set<string> availableColumns = GetTableColumns( "gamestats" );
		list<string> columnsToCreate;
		for ( hash_map<string,int>::const_iterator it = ladderInfo.begin(); it != ladderInfo.end(); ++it )
		{
			const string &szStatsName = it->first;
			if ( availableColumns.find( szStatsName ) == availableColumns.end() )
				columnsToCreate.push_back( szStatsName );
		}
		columnsToCreate.sort();
		string szQuery = "ALTER TABLE gamestats   ";
		for ( list<string>::const_iterator it = columnsToCreate.begin(); it != columnsToCreate.end(); ++it )
		{
			const string &szColumnName = *it;
			szQuery += "ADD COLUMN " + szColumnName + " INTEGER UNSIGNED NOT NULL DEFAULT '0', ";
		}
		szQuery.erase( szQuery.length() - 2, 2 );
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	}
#endif
	string szQuery = "UPDATE gamestats AS g, users AS u SET  ";
	for ( hash_map<string,int>::const_iterator it = ladderInfo.begin(); it != ladderInfo.end(); ++it )
	{
		const string &szStatsName = it->first;
		const int &nStatsValue = it->second;
		szQuery += " g." + szStatsName + " = " + StrFmt( "'%d',", nStatsValue );
	}
	szQuery.erase( szQuery.length() - 1, 1 );
	szQuery += " WHERE u.gamestats = g.statsID AND u.userID = '" + szClientDBID + "'";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
}

hash_set<string> CClients::GetTableColumns( const string &szTableName )
{
	const string szQuery = "SHOW COLUMNS FROM " + szTableName;
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT
	hash_set<string> columns;
	MYSQL_ROW row;
	while ( row = mysql_fetch_row( pResult ) )
	{
		columns.insert( row[0] );
	}
	mysql_free_result( pResult );
	return columns;
}

bool CClients::IsOnLine( const string &szNick ) const
{
	string szNickLowerCase;
	NStr::ToLower( &szNickLowerCase, szNick );

	return onLineNicks.find( szNickLowerCase ) != onLineNicks.end();
}

bool CClients::IsOnLine( const int nClientID ) const
{
	return onLine.find( nClientID ) != onLine.end();
}

void CClients::SetOnLine( const string &szNick, const int nClientID )
{
	string szNickLowerCase;
	NStr::ToLower( &szNickLowerCase, szNick );
	onLineNicks.insert( szNickLowerCase );
	onLine.erase( nClientID );
	gameConnections.erase( nClientID );
	onLine[nClientID].eState = ES_ONLINE;
	onLine[nClientID].bWant2ReceiveChat = true;

	const int nDBUserID = GetDBUserIDbyNick( szNick );
	DBUserIDByNick[szNick] = nDBUserID;
	idByNick[szNick] = nClientID;
	nickByID[nClientID] = szNick;

#ifndef _FINALRELEASE
	GetNetLogger()->OpenLogFile( szNick );
	GetNetLogger()->Log( szNick, "online" );
#endif
}

void CClients::SetOffLine( const int nClientID )
{
	onLine.erase( nClientID );
	const string szNick = nickByID[nClientID];
	
	string szNickLowerCase;
	NStr::ToLower( &szNickLowerCase, szNick );
	onLineNicks.erase( szNickLowerCase );

	nickByID.erase( nClientID );
	idByNick.erase( szNick );
	DBUserIDByNick.erase( szNick );
	gameConnections.erase( nClientID );
	hash_map<string,int>::iterator it = ladderInfoCacheLockCounter.find( szNick );
	if ( it == ladderInfoCacheLockCounter.end() )
		ladderInfoCache.erase( szNick );

#ifndef _FINALRELEASE
	GetNetLogger()->Log( szNick, "offline" );
	GetNetLogger()->CloseLogFile( szNick );
#endif
}

const bool CClients::GetNick( const int nClientID, string *pszNick ) const
{
	hash_map<int, string>::const_iterator iter = nickByID.find( nClientID );
	if ( iter != nickByID.end() )
	{
		*pszNick = iter->second;
		return true;
	}
	else
		return false;
}

const bool CClients::GetClientID( const string &szNick, int *pnClientID ) const
{
	hash_map<string, int>::const_iterator iter = idByNick.find( szNick );
	if ( iter != idByNick.end() )
	{
		*pnClientID = iter->second;
		return true;
	}
	else
		return false;
}

const bool CClients::GetCommonClientInfo( const int nClientID, SCommonClientInfo *pCommonClientInfo ) const
{
	hash_map<int, SCommonClientInfo>::const_iterator iter = onLine.find( nClientID );
	if ( iter == onLine.end() )
		return false;
	else
	{
		*pCommonClientInfo = iter->second;
		return true;
	}
}

void CClients::SetCommonClientInfo( const int nClientID, const SCommonClientInfo &commonClientInfo )
{
	hash_map<int, SCommonClientInfo>::iterator iter = onLine.find( nClientID );
	if ( iter != onLine.end() )
		iter->second = commonClientInfo;
}

void CClients::SetGameConnectInfo( const int nClientID, const int nConnection, 
																	 const string &szIP, const int nGameConnectPort )
{
	hash_map<int, SCommonClientInfo>::iterator iter = onLine.find( nClientID );
	if ( iter != onLine.end() )
	{
		SGameConnection &conn= gameConnections[nClientID];

		conn.connections[nConnection].szIP = szIP;
		conn.connections[nConnection].nPort = nGameConnectPort;
	}
}

bool CClients::GetGameConnectInfo( const int nClientID, const int nConnection, SGameConnection::SAddressInfo *pAddressInfo ) const
{
	hash_map<int, SGameConnection>::const_iterator iter = gameConnections.find( nClientID );
	if ( iter == gameConnections.end() )
		return false;

	const SGameConnection &conn = iter->second;
	hash_map<int, SGameConnection::SAddressInfo>::const_iterator addr_iter = conn.connections.find( nConnection );
	if ( addr_iter == conn.connections.end() )
		return false;

	*pAddressInfo = addr_iter->second;
	return true;
}

void CClients::Log( const int nClientID, const string &szMsg ) const
{
#if !defined( _FINALRELEASE ) && !defined( _BETARELEASE )
	//DebugTrace( "%s", szMsg.c_str() );
	hash_map<int, string>::const_iterator iter = nickByID.find( nClientID );
	if ( iter != nickByID.end() )
		GetNetLogger()->Log( iter->second, szMsg );
#endif // _FINALRELEASE
}

bool CClients::IsCDKeyOnline( const string &szCDKey )
{
	if ( szCDKey == "" )
		return true;
	string szQuery = "SELECT n.Name FROM cdkeys AS c, users AS u, names AS n WHERE c.CDKey = '" + EscapeString( szCDKey ) + 
		"' AND c.cdkeyID = u.cdkey AND u.name = n.nameID";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	MYSQL_RES *pResult = mysql_store_result( pMySQL );
	MYSQL_CHECK_RESULT;
	bool bResult = false;
	int nRows = mysql_num_rows( pResult );
	for ( int i = 0; i < nRows; ++i )
	{
		MYSQL_ROW row = mysql_fetch_row( pResult );
		string szNick = row[0];
		if ( IsOnLine( szNick ) )
		{
			bResult = true;
			break;
		}
	}
	mysql_free_result( pResult );
	return bResult;
}

string CClients::EscapeString( const string &szString ) const
{
	char *pBuffer = new char[ szString.length() * 2 + 2 ];
	mysql_real_escape_string( pMySQL, pBuffer, szString.c_str(), szString.length() );
	const string szOut = pBuffer;
	delete []pBuffer;
	return szOut;
}

void CClients::DBLogServerStatistics( const vector<string> &names, const vector<float> &values )
{
	NI_VERIFY( names.size() == values.size(), "Invalid data in CClients::DBLogServerStatistics", return );
#ifdef CHECK_TABLE_STRUCTURE
	hash_set<string> availableFields = GetTableColumns( "ServerLog" );
	list<string> columnsToCreate;
	for ( int i = 0; i < names.size(); ++i )
	{
		if ( availableFields.find( names[i] ) == availableFields.end() )
		{
			columnsToCreate.push_back( names[i] );
		}
	}
	if ( !columnsToCreate.empty() )
	{
		string szQuery = "ALTER TABLE ServerLog ";
		for ( list<string>::iterator it = columnsToCreate.begin(); it != columnsToCreate.end(); ++it )
		{
			const string &szColumnName = *it;
			szQuery += StrFmt( "ADD COLUMN %s FLOAT DEFAULT '-1', ", szColumnName.c_str() );
		}
		szQuery.erase( szQuery.length() - 2, 2 );
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	}
#endif
	string szQuery = "INSERT INTO ServerLog ( LogTime, ";
	for ( int i = 0; i < names.size(); ++i )
	{
		szQuery += StrFmt( "%s, ", names[i] );
	}
	szQuery.erase( szQuery.length() - 2, 2 );
	szQuery += " ) VALUES ( NOW(), ";
	for ( int i = 0; i < values.size(); ++i )
	{
		szQuery += StrFmt( "'%f', ", values[i] );
	}
	szQuery.erase( szQuery.length() - 2, 2 );
	szQuery += " )";
	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
}

#undef MYSQL_QUERY
#undef MYSQL_CHECK_RESULT

