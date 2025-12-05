#include "stdafx.h"
#include "clients.h"
#include "LadderLobby.h"
#if 0
#include "vendor/MySQL/include/mysql.h"
#endif
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
	std::vector<int> mapsRequested;
	std::vector<int> techsRequested;
	std::vector<int> unitsKilled;
	std::vector<int> unitsLost;
	std::vector<int> reinfXP;
	std::vector<int> reinfUsed;
	
	void ConvertToHashMap( std::unordered_map<std::string,int> *pHashMap )
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
	for ( std::unordered_map<int,std::string>::const_iterator it = pGameInfo->nickByID.begin(); it != pGameInfo->nickByID.end(); ++it )
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
		std::unordered_map<std::string,int> buffer;
		info.ConvertToHashMap( &buffer );
		DBLogRawGameResult( buffer );
	}
}

void CClients::DBLogRawGameResult( const std::unordered_map<std::string,int> &info )
{
#if 0
#ifdef LOG_FULL_GAME_RESULT
#ifdef CHECK_TABLE_STRUCTURE
	{
		std::unordered_set<std::string> availableColumns = GetTableColumns( "ResultsLog" );
		std::list<std::string> columnsToCreate;
		for ( std::unordered_map<std::string,int>::const_iterator it = info.begin(); it != info.end(); ++it )
		{
			const std::string &szStatsName = it->first;
			if ( availableColumns.find( szStatsName ) == availableColumns.end() )
				columnsToCreate.push_back( szStatsName );
		}
		columnsToCreate.sort();
		std::string szQuery = "ALTER TABLE ResultsLog  ";
		for ( std::list<std::string>::const_iterator it = columnsToCreate.begin(); it != columnsToCreate.end(); ++it )
		{
			const std::string &szColumnName = *it;
			szQuery += "ADD COLUMN " + szColumnName + " INTEGER UNSIGNED NOT NULL DEFAULT '0', ";
		}
		szQuery.erase( szQuery.length() - 2, 2 );
		MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
	}
#endif

	std::string szQuery = "INSERT INTO `ResultsLog` (  ";
	for ( std::unordered_map<std::string,int>::const_iterator it = info.begin(); it != info.end(); ++it )
	{
		const std::string &szColumnName = it->first;
		szQuery += szColumnName + ",";
	}
	szQuery.erase( szQuery.size() - 1, 1 );
	szQuery += ") VALUES (  ";

	for ( std::unordered_map<std::string,int>::const_iterator it = info.begin(); it != info.end(); ++it )
	{
		const std::string &szColumnValue = StrFmt( "%d", it->second );
		szQuery += "'" + szColumnValue + "',";
	}
	szQuery.erase( szQuery.size() - 1, 1 );
	szQuery += ")";

	MYSQL_QUERY( pMySQL, szQuery.c_str(), szQuery.length() );
#endif
#endif
}


