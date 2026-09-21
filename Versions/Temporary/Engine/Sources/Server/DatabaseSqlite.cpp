#include "stdafx.h"

#include "Database.h"

#include <sqlite3.h>

#include <string>
#include <vector>

// The embedded backend for IDatabase.
//
// What it is for is a server somebody can run without installing anything: no
// daemon, no service, no port, no credentials, just a file beside server.xml.
// See cmake/sqlite.cmake for why nothing in this server's use of a database
// argues for a server-class one.
//
// It creates its own schema on first open, from the statements below, which
// are dbstruct.sql translated. That is deliberate rather than shipping a
// second .sql to run by hand: the whole point of this backend is that there is
// no setup step.

namespace
{

// dbstruct.sql, in SQLite's spelling. Same tables, columns, order and defaults.
// What is dropped is what SQLite has no notion of: the storage engine, the
// charset, and the foreign keys, which the server never relied on to enforce
// anything since it inserts parents and children itself in order.
//
// auto_increment becomes INTEGER PRIMARY KEY AUTOINCREMENT, which is the only
// form SQLite assigns for, and int(10) unsigned becomes INTEGER, which is what
// every one of these is read back as.
const char * const SZ_SCHEMA =
#include "SqliteSchema.inc"
;

class CSqliteDatabase : public IDatabase
{
	OBJECT_NOCOPY_METHODS( CSqliteDatabase );

	sqlite3 *pDb;
	std::string szLastError;

	bool Exec( const char *pszSql );
	bool CreateSchema();
public:
	CSqliteDatabase() : pDb( 0 ) { }
	virtual ~CSqliteDatabase() { Close(); }

	virtual bool Connect( const SDbConnection &connection );
	virtual void Close();
	virtual bool IsAlive();
	virtual bool Execute( const std::string &szStatement );
	virtual bool Query( const std::string &szQuery, CDbResult *pResult );
	virtual std::string Escape( const std::string &szValue ) const;
	virtual std::unordered_set<std::string> GetColumns( const std::string &szTable );
	virtual std::string GetLastError() const { return szLastError; }
};

bool CSqliteDatabase::Exec( const char *pszSql )
{
	char *pszError = 0;
	if ( sqlite3_exec( pDb, pszSql, 0, 0, &pszError ) != SQLITE_OK )
	{
		szLastError = pszError ? pszError : "unknown error";
		sqlite3_free( pszError );
		return false;
	}
	return true;
}

bool CSqliteDatabase::CreateSchema()
{
	// IF NOT EXISTS throughout, so this is a no-op on an existing file and the
	// server does not have to know which case it is in.
	return Exec( SZ_SCHEMA );
}

bool CSqliteDatabase::Connect( const SDbConnection &connection )
{
	Close();

	// szDatabase is the file name here. Everything else in SDbConnection
	// describes a daemon and has nothing to say to this backend.
	const std::string szFile = connection.szDatabase.empty() ? std::string( "nivalnet.db" ) : connection.szDatabase;

	if ( sqlite3_open( szFile.c_str(), &pDb ) != SQLITE_OK )
	{
		szLastError = pDb ? sqlite3_errmsg( pDb ) : "cannot open database file";
		sqlite3_close( pDb );
		pDb = 0;
		return false;
	}

	// Wait rather than fail if something else holds the file. The server is one
	// thread, but nothing stops a second copy or a shell being open on it.
	sqlite3_busy_timeout( pDb, 5000 );

	// The server declares foreign keys in its schema and then inserts in an
	// order of its own; SQLite leaves them off by default and this keeps that,
	// which is also how the MySQL side behaves in practice.
	Exec( "PRAGMA foreign_keys = OFF;" );

	if ( !CreateSchema() )
	{
		Close();
		return false;
	}

	return true;
}

void CSqliteDatabase::Close()
{
	if ( pDb )
	{
		sqlite3_close( pDb );
		pDb = 0;
	}
}

bool CSqliteDatabase::IsAlive()
{
	// A file that is open is a file that is there. Nothing can have gone away
	// between two calls the way a connection to a daemon can.
	return pDb != 0;
}

bool CSqliteDatabase::Query( const std::string &szQuery, CDbResult *pResult )
{
	pResult->Clear();

	if ( pDb == 0 )
	{
		szLastError = "not connected";
		return false;
	}

	sqlite3_stmt *pStatement = 0;
	if ( sqlite3_prepare_v2( pDb, szQuery.c_str(), -1, &pStatement, 0 ) != SQLITE_OK )
	{
		szLastError = sqlite3_errmsg( pDb );
		return false;
	}

	const int nColumns = sqlite3_column_count( pStatement );
	if ( nColumns > 0 )
	{
		std::vector<std::string> names;
		names.reserve( nColumns );
		for ( int i = 0; i < nColumns; ++i )
		{
			const char *pszName = sqlite3_column_name( pStatement, i );
			names.push_back( pszName ? pszName : "" );
		}
		pResult->SetColumnNames( std::move( names ) );
	}

	for ( ;; )
	{
		const int nStep = sqlite3_step( pStatement );
		if ( nStep == SQLITE_ROW )
		{
			std::vector<std::string> values;
			values.reserve( nColumns );
			for ( int i = 0; i < nColumns; ++i )
			{
				// A SQL NULL reads back as an empty string, which is what the
				// MariaDB backend does and what CDbResult::Get documents.
				const unsigned char *pValue = sqlite3_column_text( pStatement, i );
				const int nBytes = sqlite3_column_bytes( pStatement, i );
				values.push_back( pValue ? std::string( reinterpret_cast<const char *>( pValue ), nBytes )
					: std::string() );
			}
			pResult->AddRow( std::move( values ) );
			continue;
		}

		if ( nStep == SQLITE_DONE )
		{
			break;
		}

		szLastError = sqlite3_errmsg( pDb );
		sqlite3_finalize( pStatement );
		return false;
	}

	sqlite3_finalize( pStatement );
	return true;
}

bool CSqliteDatabase::Execute( const std::string &szStatement )
{
	CDbResult ignored;
	return Query( szStatement, &ignored );
}

std::string CSqliteDatabase::Escape( const std::string &szValue ) const
{
	// What goes between the quotes the caller writes, so a quote is doubled and
	// nothing else changes. sqlite3_mprintf's %q is the same rule; doing it here
	// avoids allocating through the library for every field.
	std::string szEscaped;
	szEscaped.reserve( szValue.size() );
	for ( const char c : szValue )
	{
		if ( c == '\'' )
		{
			szEscaped += '\'';
		}
		szEscaped += c;
	}
	return szEscaped;
}

std::unordered_set<std::string> CSqliteDatabase::GetColumns( const std::string &szTable )
{
	std::unordered_set<std::string> columns;

	// PRAGMA table_info is this backend's answer to SHOW COLUMNS, which is the
	// reason GetColumns is a method on IDatabase rather than a query the
	// callers write. Its second column is the name.
	CDbResult result;
	if ( !Query( "PRAGMA table_info(`" + szTable + "`)", &result ) )
	{
		return columns;
	}

	for ( int i = 0; i < result.GetRowCount(); ++i )
	{
		columns.insert( result.Get( i, 1 ) );
	}
	return columns;
}

}

IDatabase* CreateSqliteDatabase()
{
	return new CSqliteDatabase();
}
