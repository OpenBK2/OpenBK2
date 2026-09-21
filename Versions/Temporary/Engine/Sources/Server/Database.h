#pragma once

#include <string>
#include <unordered_set>
#include <vector>

// The database the dedicated server keeps its accounts, ladder and logs in.
//
// The server was written directly against the MySQL C API, with every call
// site holding a MYSQL* and every query going through two macros. This is that
// surface, named: eleven MySQL entry points and about thirty three statements,
// no prepared statements, no transactions, and a single thread. There is very
// little here because there was very little there.
//
// It exists so the same server can run on a database daemon or on an embedded
// file. See cmake/mariadb.cmake and cmake/sqlite.cmake for why in that
// order.

//! One row of a result set, indexed by column the way the C API's MYSQL_ROW is.
//!
//! A view onto the result that produced it, so it is only valid while that
//! result is, which matches how every caller here uses one.
class CDbRow
{
	const std::vector<std::string> *pValues;
public:
	explicit CDbRow( const std::vector<std::string> &values ) : pValues( &values ) { }

	const std::string& operator[]( const int nColumn ) const { return (*pValues)[nColumn]; }
};

//! One fully read result set.
//!
//! Materialised rather than streamed, because every caller here reads the whole
//! thing and then frees it, and because holding a cursor open would be the one
//! thing that made the two backends behave differently.
//!
//! Every value is text. That is what the MySQL C API hands back, and what the
//! call sites already do with it: NStr::ToInt, or straight into a std::string.
//!
//! Column names are carried, not just values. The server reads gamestats with
//! SELECT g.*, whose column set is decided at run time, and then walks the
//! fields to pair each name with its value. That is the only reason
//! mysql_fetch_field appears in the original at all.
class CDbResult
{
	std::vector<std::string> columnNames;
	std::vector<std::vector<std::string> > rows;
public:
	void Clear() { columnNames.clear(); rows.clear(); }

	//! Called by a backend as it reads a result set in.
	void SetColumnNames( std::vector<std::string> names ) { columnNames = std::move( names ); }
	void AddRow( std::vector<std::string> row ) { rows.push_back( std::move( row ) ); }

	int GetRowCount() const { return static_cast<int>( rows.size() ); }
	int GetColumnCount() const { return static_cast<int>( columnNames.size() ); }

	const std::string& GetColumnName( const int nColumn ) const { return columnNames[nColumn]; }

	//! The value at a cell.
	//!
	//! A SQL NULL reads back as an empty string. The MySQL API represents one as
	//! a null char pointer, and the call sites here assign row[i] straight into a
	//! std::string without checking, so a NULL was undefined behaviour rather
	//! than a value they handled. SELECT MAX(xp) on an empty gamestats is the
	//! case that reaches it.
	const std::string& Get( const int nRow, const int nColumn ) const { return rows[nRow][nColumn]; }

	CDbRow Row( const int nRow ) const { return CDbRow( rows[nRow] ); }
};

//! Where to connect. Ignored by an embedded backend apart from the database
//! name, which is the file it opens.
struct SDbConnection
{
	std::string szHost;
	std::string szUser;
	std::string szPassword;
	std::string szDatabase;
	int nPort;

	SDbConnection() : nPort( 0 ) { }
};

struct IDatabase : public CObjectBase
{
	virtual ~IDatabase() { }

	virtual bool Connect( const SDbConnection &connection ) = 0;
	virtual void Close() = 0;

	//! True if the connection is still usable, reconnecting if the backend can.
	//! The server calls this once a minute; an embedded backend has nothing to
	//! check and says yes.
	virtual bool IsAlive() = 0;

	//! Run a statement that returns no rows. False if the backend rejected it.
	virtual bool Execute( const std::string &szStatement ) = 0;

	//! Run a query and read its result set into pResult.
	virtual bool Query( const std::string &szQuery, CDbResult *pResult ) = 0;

	//! Quote the contents of a string literal being concatenated into a
	//! statement. Every backend has its own rules, and its own function for it.
	virtual std::string Escape( const std::string &szValue ) const = 0;

	//! The column names of a table.
	//!
	//! Its own method rather than a query the callers write, because this is
	//! where the backends diverge most: SHOW COLUMNS on MySQL, PRAGMA
	//! table_info on SQLite, information_schema elsewhere. The server needs it
	//! because it migrates its own schema: the statistics counter set decides
	//! the columns of gamestats and ResultsLog, and it adds the ones that are
	//! missing at run time.
	virtual std::unordered_set<std::string> GetColumns( const std::string &szTable ) = 0;

	//! The backend's last error, for the caller's log line.
	virtual std::string GetLastError() const = 0;
};

//! The MariaDB and MySQL backend. Talks to either, being wire compatible.
IDatabase* CreateMariaDbDatabase();
