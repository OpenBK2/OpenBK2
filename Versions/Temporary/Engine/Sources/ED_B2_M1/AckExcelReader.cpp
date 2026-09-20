#include "stdafx.h"
#include "AckExcelReader.h"

#include "MapEditorLib/MessageBoxes.h"

#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>

#include <cstdint>
#include <cstdlib>

// The acks table, read from the Excel sheet through ODBC.
//
// This was an MFC CRecordset over a CDatabase. It is the plain ODBC API now,
// asking the same question the recordset did: the same connection string, the
// five columns RFX_Text bound, in that order, from [ACKS$], forward only and
// read only. A NULL cell reads as an empty string, as RFX_Text left it, and an
// ODBC failure is shown in a message box, as CDBException::ReportError did,
// with whatever was read before it kept.
//
// The narrow (A) ODBC calls, as MFC's MBCS build made: the driver converts the
// sheet's text to the process code page, which is UTF-8 here.

namespace NAcks
{

namespace
{
	// The first installed driver whose name mentions Excel, as before.
	bool GetExcelODBCDriverName( std::string *pszResult )
	{
		pszResult->clear();
		char szBuffer[2001] = {};
		WORD nBufferOut = 0;
		if ( !::SQLGetInstalledDrivers( szBuffer, 2000, &nBufferOut ) )
		{
			return false;
		}
		// A list of names, each ended by a zero, the list by an empty name.
		for ( const char *pszName = szBuffer; *pszName != '\0'; pszName += strlen( pszName ) + 1 )
		{
			if ( strstr( pszName, "Excel" ) != nullptr )
			{
				*pszResult = pszName;
				return true;
			}
		}
		return false;
	}


	// The first diagnostic record on hHandle, as ReportError showed it.
	void ReportError( SQLSMALLINT nHandleType, SQLHANDLE hHandle )
	{
		SQLCHAR szState[6] = {};
		SQLCHAR szMessage[SQL_MAX_MESSAGE_LENGTH] = {};
		SQLINTEGER nNativeError = 0;
		SQLSMALLINT nMessageLength = 0;
		std::string szText = "ODBC error";
		if ( SQL_SUCCEEDED( ::SQLGetDiagRecA( nHandleType, hHandle, 1, szState, &nNativeError, szMessage,
																				 sizeof( szMessage ), &nMessageLength ) ) )
		{
			szText = reinterpret_cast<const char*>( szMessage );
		}
		// Titled with the application name now, where a null title gave the system
		// default ("Error"); every other box in the editor is titled this way.
		NMessage::Warning( szText );
	}


	// Column nColumn of the current row as text; empty for NULL.
	bool GetText( SQLHSTMT hStatement, SQLUSMALLINT nColumn, std::string *pszText )
	{
		pszText->clear();
		char buffer[1024];
		for ( ;; )
		{
			SQLLEN nIndicator = 0;
			const SQLRETURN nResult = ::SQLGetData( hStatement, nColumn, SQL_C_CHAR, buffer, sizeof( buffer ), &nIndicator );
			if ( nResult == SQL_NO_DATA )
			{
				return true;
			}
			if ( !SQL_SUCCEEDED( nResult ) )
			{
				return false;
			}
			if ( nIndicator == SQL_NULL_DATA )
			{
				return true;
			}
			// Truncated: the buffer is full, less its terminator, and the rest
			// comes from the next call.
			if ( nResult == SQL_SUCCESS_WITH_INFO && ( nIndicator == SQL_NO_TOTAL || nIndicator >= SQLLEN( sizeof( buffer ) ) ) )
			{
				pszText->append( buffer, sizeof( buffer ) - 1 );
				continue;
			}
			pszText->append( buffer, static_cast<size_t>( nIndicator ) );
			return true;
		}
	}
}


bool LoadAcksTable( std::vector<SAckEntry> *pRes, const std::string &szFileName )
{
	std::string szDriver;
	GetExcelODBCDriverName( &szDriver );
	// CDatabase::Open took the "ODBC;" prefix off before connecting.
	const std::string szConnect = "DRIVER={" + szDriver + "};DSN='';DBQ=" + szFileName + ";MAXSCANROWS=0";

	SQLHENV hEnvironment = SQL_NULL_HENV;
	SQLHDBC hConnection = SQL_NULL_HDBC;
	SQLHSTMT hStatement = SQL_NULL_HSTMT;
	bool bConnected = false;
	if ( SQL_SUCCEEDED( ::SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnvironment ) ) &&
			 SQL_SUCCEEDED( ::SQLSetEnvAttr( hEnvironment, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>( SQL_OV_ODBC3 ), 0 ) ) &&
			 SQL_SUCCEEDED( ::SQLAllocHandle( SQL_HANDLE_DBC, hEnvironment, &hConnection ) ) )
	{
		// Read only, as the recordset was opened.
		::SQLSetConnectAttr( hConnection, SQL_ATTR_ACCESS_MODE, reinterpret_cast<SQLPOINTER>( SQL_MODE_READ_ONLY ), 0 );
		bConnected = SQL_SUCCEEDED( ::SQLDriverConnectA( hConnection, nullptr,
																										 reinterpret_cast<SQLCHAR*>( const_cast<char*>( szConnect.c_str() ) ),
																										 SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT ) );
		if ( !bConnected )
		{
			ReportError( SQL_HANDLE_DBC, hConnection );
		}
	}

	if ( bConnected && SQL_SUCCEEDED( ::SQLAllocHandle( SQL_HANDLE_STMT, hConnection, &hStatement ) ) )
	{
		// What CRecordset built from GetDefaultSQL and the RFX_Text columns.
		static const char SELECT[] =
			"SELECT [Situation code name],[RecordCode],[FileName],[Probability],[SUBSET Code] FROM [ACKS$]";
		if ( !SQL_SUCCEEDED( ::SQLExecDirectA( hStatement, reinterpret_cast<SQLCHAR*>( const_cast<char*>( SELECT ) ), SQL_NTS ) ) )
		{
			ReportError( SQL_HANDLE_STMT, hStatement );
		}
		else
		{
			std::string szLastNormalSituationCode;
			std::string szProbability;
			std::string szSubsetCode;
			for ( ;; )
			{
				const SQLRETURN nFetch = ::SQLFetch( hStatement );
				if ( nFetch == SQL_NO_DATA )
				{
					break;
				}
				if ( !SQL_SUCCEEDED( nFetch ) )
				{
					ReportError( SQL_HANDLE_STMT, hStatement );
					break;
				}
				SAckEntry entry;
				if ( !GetText( hStatement, 1, &entry.szSituationCode ) ||
						 !GetText( hStatement, 2, &entry.szRecordCode ) ||
						 !GetText( hStatement, 3, &entry.szFileName ) ||
						 !GetText( hStatement, 4, &szProbability ) ||
						 !GetText( hStatement, 5, &szSubsetCode ) )
				{
					ReportError( SQL_HANDLE_STMT, hStatement );
					break;
				}
				// A blank situation code continues the one above it.
				if ( entry.szSituationCode.empty() )
				{
					entry.szSituationCode = szLastNormalSituationCode;
				}
				else
				{
					szLastNormalSituationCode = entry.szSituationCode;
				}
				entry.fProbability = static_cast<float>( atof( szProbability.c_str() ) );
				entry.nSubsetCode = szSubsetCode.empty() ? 0 : atoi( szSubsetCode.c_str() );
				pRes->push_back( entry );
			}
		}
	}

	if ( hStatement != SQL_NULL_HSTMT )
	{
		::SQLFreeHandle( SQL_HANDLE_STMT, hStatement );
	}
	if ( hConnection != SQL_NULL_HDBC )
	{
		if ( bConnected )
		{
			::SQLDisconnect( hConnection );
		}
		::SQLFreeHandle( SQL_HANDLE_DBC, hConnection );
	}
	if ( hEnvironment != SQL_NULL_HENV )
	{
		::SQLFreeHandle( SQL_HANDLE_ENV, hEnvironment );
	}
	// True even when nothing could be read, as before; the caller treats an
	// empty table as the failure.
	return true;
}

}
