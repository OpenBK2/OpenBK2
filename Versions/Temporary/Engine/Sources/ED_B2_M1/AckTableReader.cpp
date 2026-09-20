#include "stdafx.h"
#include "AckTableReader.h"

#include "MapEditorLib/MessageBoxes.h"
#include "Misc/StrProc.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace NAcks
{

namespace
{
	// The columns, by the names the spreadsheet's header row used.
	const char *const COLUMN_SITUATION = "Situation code name";
	const char *const COLUMN_RECORD = "RecordCode";
	const char *const COLUMN_FILE = "FileName";
	const char *const COLUMN_PROBABILITY = "Probability";
	const char *const COLUMN_SUBSET = "SUBSET Code";


	// One CSV record, which is not one line: a quoted field may contain the
	// separator and may contain newlines.
	//
	// Returns false at end of file with nothing read. A record with no fields
	// cannot happen -- an empty line is one empty field -- so the caller tells
	// a blank line by its single empty field.
	bool ReadRecord( std::istream &rStream, char cSeparator, std::vector<std::string> *pFields )
	{
		pFields->clear();
		std::string field;
		bool bInQuotes = false;
		bool bAny = false;
		for ( ;; )
		{
			const int nChar = rStream.get();
			if ( nChar == std::char_traits<char>::eof() )
			{
				if ( !bAny )
				{
					return false;
				}
				pFields->push_back( field );
				return true;
			}
			bAny = true;
			const char c = static_cast<char>( nChar );
			if ( bInQuotes )
			{
				if ( c != '"' )
				{
					field += c;
					continue;
				}
				// A doubled quote inside a quoted field is one quote; a single
				// one ends the field.
				if ( rStream.peek() == '"' )
				{
					rStream.get();
					field += '"';
					continue;
				}
				bInQuotes = false;
				continue;
			}
			if ( c == '"' )
			{
				bInQuotes = true;
				continue;
			}
			if ( c == cSeparator )
			{
				pFields->push_back( field );
				field.clear();
				continue;
			}
			if ( c == '\r' )
			{
				// CRLF or a lone CR both end the record.
				if ( rStream.peek() == '\n' )
				{
					rStream.get();
				}
				pFields->push_back( field );
				return true;
			}
			if ( c == '\n' )
			{
				pFields->push_back( field );
				return true;
			}
			field += c;
		}
	}


	// Comma or semicolon, whichever the header row uses more of. Excel writes
	// the system list separator, so this is not knowable in advance.
	char GuessSeparator( const std::string &rszHeaderLine )
	{
		size_t nCommas = 0, nSemicolons = 0;
		for ( const char c : rszHeaderLine )
		{
			if ( c == ',' ) { ++nCommas; }
			else if ( c == ';' ) { ++nSemicolons; }
		}
		return ( nSemicolons > nCommas ) ? ';' : ',';
	}


	int FindColumn( const std::vector<std::string> &rHeader, const char *pszName )
	{
		for ( size_t i = 0; i < rHeader.size(); ++i )
		{
			if ( NStr::IEquals( rHeader[i], pszName ) )
			{
				return static_cast<int>( i );
			}
		}
		return -1;
	}


	const std::string& Field( const std::vector<std::string> &rFields, int nColumn )
	{
		static const std::string szEmpty;
		return ( ( nColumn >= 0 ) && ( nColumn < static_cast<int>( rFields.size() ) ) )
						 ? rFields[nColumn] : szEmpty;
	}
}


bool LoadAcksTable( std::vector<SAckEntry> *pRes, const std::string &szFileName )
{
	std::ifstream file( szFileName.c_str(), std::ios::binary );
	if ( !file.is_open() )
	{
		NMessage::Error( "Could not open the acks table:\n" + szFileName );
		return false;
	}

	// The separator has to be known before the first record can be split, and
	// the header is a record, so read its line first and start over.
	std::string szHeaderLine;
	std::getline( file, szHeaderLine );
	const char cSeparator = GuessSeparator( szHeaderLine );
	file.clear();
	file.seekg( 0 );

	// A UTF-8 byte order mark, which Excel's "CSV UTF-8" writes and which is
	// not part of the first column's name.
	if ( ( file.peek() == 0xEF ) )
	{
		char bom[3] = {};
		file.read( bom, 3 );
		if ( ( static_cast<unsigned char>( bom[0] ) != 0xEF ) ||
				 ( static_cast<unsigned char>( bom[1] ) != 0xBB ) ||
				 ( static_cast<unsigned char>( bom[2] ) != 0xBF ) )
		{
			file.clear();
			file.seekg( 0 );
		}
	}

	std::vector<std::string> header;
	if ( !ReadRecord( file, cSeparator, &header ) )
	{
		NMessage::Error( "The acks table is empty:\n" + szFileName );
		return false;
	}

	const int nSituation = FindColumn( header, COLUMN_SITUATION );
	const int nRecord = FindColumn( header, COLUMN_RECORD );
	const int nFile = FindColumn( header, COLUMN_FILE );
	const int nProbability = FindColumn( header, COLUMN_PROBABILITY );
	const int nSubset = FindColumn( header, COLUMN_SUBSET );
	if ( ( nSituation < 0 ) || ( nRecord < 0 ) || ( nFile < 0 ) ||
			 ( nProbability < 0 ) || ( nSubset < 0 ) )
	{
		// Naming the columns it wanted beats "could not read the file": the
		// usual cause is a sheet exported with the wrong header row.
		NMessage::Error( std::string( "The acks table has no header row naming all of:\n  " ) +
										 COLUMN_SITUATION + "\n  " + COLUMN_RECORD + "\n  " + COLUMN_FILE +
										 "\n  " + COLUMN_PROBABILITY + "\n  " + COLUMN_SUBSET +
										 "\n\n" + szFileName );
		return false;
	}

	std::string szLastNormalSituationCode;
	std::vector<std::string> fields;
	while ( ReadRecord( file, cSeparator, &fields ) )
	{
		SAckEntry entry;
		entry.szSituationCode = Field( fields, nSituation );
		entry.szRecordCode = Field( fields, nRecord );
		entry.szFileName = Field( fields, nFile );
		// A row with nothing in it is the end of the sheet's content rather than
		// an entry; the ODBC reader never saw these because the driver stopped
		// at the used range.
		if ( entry.szRecordCode.empty() && entry.szFileName.empty() &&
				 entry.szSituationCode.empty() )
		{
			continue;
		}
		// A blank situation code continues the one above it, which is how the
		// spreadsheet's merged cells arrived through ODBC as well.
		if ( entry.szSituationCode.empty() )
		{
			entry.szSituationCode = szLastNormalSituationCode;
		}
		else
		{
			szLastNormalSituationCode = entry.szSituationCode;
		}
		entry.fProbability = static_cast<float>( atof( Field( fields, nProbability ).c_str() ) );
		const std::string &rszSubset = Field( fields, nSubset );
		entry.nSubsetCode = rszSubset.empty() ? 0 : atoi( rszSubset.c_str() );
		pRes->push_back( entry );
	}
	return true;
}

}
