#include "stdafx.h"
#include "Clients.h"
#include "LadderLobby.h"
#include "vendor/MySQL/include/mysql.h"
#include "Misc/StrProc.h"
#include "Statistics.h"
#include "HashMapConvertor.h"

#include <zlib.h>

#define CHECK_TABLE_STRUCTURE
#ifndef _FINALRELEASE
	#define LOG_FULL_GAME_RESULT
#endif

#define MYSQL_QUERY( a1, a2, a3 ) \
{	/*DebugTrace( "MySQL: %s", a2 );*/ \
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

struct SPlayerInfoToLog
{
	int nDBID;
	int nMapPlayed;
	int nTechPlayed;
	int nWin;
	int nGameLength;
	vector<int> mapsRequested;
	vector<int> techsRequested;
	vector<int> unitsKilled;
	vector<int> unitsLost;
	vector<int> reinfXP;
	vector<int> reinfUsed;
	
	void ConvertToHashMap( hash_map<string,int> *pHashMap )
	{
#define CONVERT_NUMBER( name, var ) NHashMapConvertor::ConvertNumber( pHashMap, name, &(var), false )
#define CONVERT_VECTOR( name, var ) NHashMapConvertor::ConvertVector( pHashMap, name, &(var), false )

		CONVERT_NUMBER( "playerid", nDBID );
		CONVERT_VECTOR( "mapsrequested", mapsRequested );
		CONVERT_VECTOR( "techsrequested", techsRequested );
		CONVERT_VECTOR( "reinfxp", reinfXP );
		CONVERT_VECTOR( "reinfused", reinfUsed );
		CONVERT_NUMBER( "win", nWin );
		CONVERT_NUMBER( "mapplayed", nMapPlayed );
		CONVERT_NUMBER( "techplayed", nTechPlayed );
		CONVERT_VECTOR( "unitskilled", unitsKilled );
		CONVERT_VECTOR( "unitslost", unitsLost );
		CONVERT_NUMBER( "gamelength", nGameLength );

#undef CONVERT_NUMBER
#undef CONVERT_VECTOR
	}
};

void CClients::DBLogGameResult( SLadderGameInfo *pGameInfo )
{
	for ( hash_map<int,string>::const_iterator it = pGameInfo->nickByID.begin(); it != pGameInfo->nickByID.end(); ++it )
	{
		SPlayerInfoToLog info;
		info.nDBID = GetDBUserIDbyNick( it->second );
		const int nPlayerID = it->first;
		info.nMapPlayed = pGameInfo->nMapID;
		info.nTechPlayed = pGameInfo->nTechLevel;
		info.nWin = ( pGameInfo->winners.find( nPlayerID ) != pGameInfo->winners.end() ) ? 1 : 0;
		info.mapsRequested = pGameInfo->mapsRequested[nPlayerID];
		info.techsRequested = pGameInfo->techsRequested[nPlayerID];
		info.nGameLength = ( pGameInfo->nDeathTime - pGameInfo->nStartTime ) / 1000;
		hash_map<string,int> buffer;
		info.ConvertToHashMap( &buffer );
		DBLogRawGameResult( buffer );
	}
}

void CClients::DBLogRawGameResult( const hash_map<string,int> &info )
{
#ifdef LOG_FULL_GAME_RESULT
#ifdef CHECK_TABLE_STRUCTURE
	{
		hash_set<string> availableColumns = GetTableColumns( "ResultsLog" );
		list<string> columnsToCreate;
		for ( hash_map<string,int>::const_iterator it = info.begin(); it != info.end(); ++it )
		{
			const string &szStatsName = it->first;
			if ( availableColumns.find( szStatsName ) == availableColumns.end() )
				columnsToCreate.push_back( szStatsName );
		}
		columnsToCreate.sort();
		string szQuery = "ALTER TABLE ResultsLog  ";
		for ( list<string>::const_iterator it = columnsToCreate.begin(); it != columnsToCreate.end(); ++it )
		{
			const string &szColumnName = *it;
			szQuery += "ADD COLUMN " + szColumnName + " INTEGER UNSIGNED NOT NULL DEFAULT '0', ";
		}
		szQuery.erase( szQuery.length() - 2, 2 );
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	}
#endif

	string szQuery = "INSERT INTO `ResultsLog` (  ";
	for ( hash_map<string,int>::const_iterator it = info.begin(); it != info.end(); ++it )
	{
		const string &szColumnName = it->first;
		szQuery += szColumnName + ",";
	}
	szQuery.erase( szQuery.size() - 1, 1 );
	szQuery += ") VALUES (  ";

	for ( hash_map<string,int>::const_iterator it = info.begin(); it != info.end(); ++it )
	{
		const string &szColumnValue = StrFmt( "%d", it->second );
		szQuery += "'" + szColumnValue + "',";
	}
	szQuery.erase( szQuery.size() - 1, 1 );
	szQuery += ")";

	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
#endif
}


