#include "stdafx.h"

#include "Database.h"

#include <mysql.h>

#include <vector>

// The MariaDB and MySQL backend for IDatabase.
//
// mysql.h here comes from MariaDB Connector/C, which is wire and API
// compatible with the MySQL client the server was written against; see
// cmake/mariadb.cmake for why that one. The sources used to include
// "vendor/MySQL/include/mysql.h", a vendored copy that is not in this tree and
// never was.

namespace
{

class CMariaDbDatabase : public IDatabase
{
	OBJECT_NOCOPY_METHODS( CMariaDbDatabase );

	MYSQL *pMySQL;
	SDbConnection connection;
	std::string szLastError;

	//! Read whatever the last statement produced into pResult.
	//!
	//! A statement that returns no rows leaves mysql_store_result null, which is
	//! also what a failure looks like, so the two are told apart by asking
	//! whether the statement was supposed to have columns at all. That is what
	//! mysql_field_count is for.
	bool StoreResult( CDbResult *pResult );
public:
	CMariaDbDatabase() : pMySQL( 0 ) { }
	virtual ~CMariaDbDatabase() { Close(); }

	virtual bool Connect( const SDbConnection &connection );
	virtual void Close();
	virtual bool IsAlive();
	virtual bool Execute( const std::string &szStatement );
	virtual bool Query( const std::string &szQuery, CDbResult *pResult );
	virtual std::string Escape( const std::string &szValue ) const;
	virtual std::unordered_set<std::string> GetColumns( const std::string &szTable );
	virtual std::string GetLastError() const { return szLastError; }
};

bool CMariaDbDatabase::Connect( const SDbConnection &_connection )
{
	Close();
	connection = _connection;

	pMySQL = mysql_init( 0 );
	if ( pMySQL == 0 )
	{
		szLastError = "mysql_init failed";
		return false;
	}

	// Let the client library reconnect on its own when a connection has gone
	// idle long enough for the server to drop it. The original relied on
	// mysql_ping doing this, which it only does with the option set.
	my_bool bReconnect = 1;
	mysql_options( pMySQL, MYSQL_OPT_RECONNECT, &bReconnect );

	if ( mysql_real_connect( pMySQL, connection.szHost.c_str(), connection.szUser.c_str(),
			connection.szPassword.c_str(), connection.szDatabase.c_str(),
			connection.nPort, 0, 0 ) == 0 )
	{
		szLastError = mysql_error( pMySQL );
		mysql_close( pMySQL );
		pMySQL = 0;
		return false;
	}

	return true;
}

void CMariaDbDatabase::Close()
{
	if ( pMySQL )
	{
		mysql_close( pMySQL );
		pMySQL = 0;
	}
}

bool CMariaDbDatabase::IsAlive()
{
	if ( pMySQL == 0 )
	{
		return false;
	}
	// Zero means the connection answered, or was re-established.
	return mysql_ping( pMySQL ) == 0;
}

bool CMariaDbDatabase::StoreResult( CDbResult *pResult )
{
	pResult->Clear();

	MYSQL_RES *pRaw = mysql_store_result( pMySQL );
	if ( pRaw == 0 )
	{
		// No columns means the statement simply had no result set, which is not
		// an error. Any other case is one.
		if ( mysql_field_count( pMySQL ) == 0 )
		{
			return true;
		}
		szLastError = mysql_error( pMySQL );
		return false;
	}

	const unsigned int nColumns = mysql_num_fields( pRaw );

	std::vector<std::string> names;
	names.reserve( nColumns );
	const MYSQL_FIELD *pFields = mysql_fetch_fields( pRaw );
	for ( unsigned int i = 0; i < nColumns; ++i )
	{
		names.push_back( pFields[i].name ? pFields[i].name : "" );
	}
	pResult->SetColumnNames( std::move( names ) );

	// Lengths rather than strlen, so a value carrying an embedded zero survives
	// the trip. mysql_fetch_lengths is per row and has to be read before the
	// next fetch.
	while ( MYSQL_ROW row = mysql_fetch_row( pRaw ) )
	{
		const unsigned long *pLengths = mysql_fetch_lengths( pRaw );
		std::vector<std::string> values;
		values.reserve( nColumns );
		for ( unsigned int i = 0; i < nColumns; ++i )
		{
			// A SQL NULL arrives as a null pointer and becomes an empty string;
			// see the note on CDbResult::Get.
			values.push_back( row[i] ? std::string( row[i], pLengths[i] ) : std::string() );
		}
		pResult->AddRow( std::move( values ) );
	}

	mysql_free_result( pRaw );
	return true;
}

bool CMariaDbDatabase::Execute( const std::string &szStatement )
{
	CDbResult ignored;
	return Query( szStatement, &ignored );
}

bool CMariaDbDatabase::Query( const std::string &szQuery, CDbResult *pResult )
{
	if ( pMySQL == 0 )
	{
		szLastError = "not connected";
		return false;
	}

	// mysql_real_query rather than mysql_query, because the statements here are
	// built by concatenation and can contain a zero byte if an escaped value
	// did.
	if ( mysql_real_query( pMySQL, szQuery.c_str(), static_cast<unsigned long>( szQuery.length() ) ) != 0 )
	{
		szLastError = mysql_error( pMySQL );
		return false;
	}

	return StoreResult( pResult );
}

std::string CMariaDbDatabase::Escape( const std::string &szValue ) const
{
	if ( pMySQL == 0 )
	{
		return szValue;
	}

	// Worst case is every character escaped, plus the terminator, which is what
	// the API asks callers to allocate for.
	std::string szEscaped;
	szEscaped.resize( szValue.length() * 2 + 1 );
	const unsigned long nLength = mysql_real_escape_string( pMySQL, &szEscaped[0],
		szValue.c_str(), static_cast<unsigned long>( szValue.length() ) );
	szEscaped.resize( nLength );
	return szEscaped;
}

std::unordered_set<std::string> CMariaDbDatabase::GetColumns( const std::string &szTable )
{
	std::unordered_set<std::string> columns;

	CDbResult result;
	if ( !Query( "SHOW COLUMNS FROM " + szTable, &result ) )
	{
		return columns;
	}

	// SHOW COLUMNS puts the column's own name first, whatever else it reports.
	for ( int i = 0; i < result.GetRowCount(); ++i )
	{
		columns.insert( result.Get( i, 0 ) );
	}
	return columns;
}

}

IDatabase* CreateMariaDbDatabase()
{
	return new CMariaDbDatabase();
}
