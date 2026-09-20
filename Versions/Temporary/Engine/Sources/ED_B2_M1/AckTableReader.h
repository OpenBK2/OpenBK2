#pragma once

namespace NAcks
{

struct SAckEntry
{
	std::string szSituationCode;
	std::string szRecordCode;
	std::string szFileName;
	float fProbability;
	int nSubsetCode;
};

// The acks table, from a CSV with a header row.
//
// This was an .xls read through the Excel ODBC driver, and the whole apparatus
// -- sql.h, odbc32, odbccp32, a connection string naming a driver by whichever
// localised name the machine had -- existed to run one query over five text
// columns. Nothing Excel offers was used: no formulas, no second sheet, no
// types. It was a flat table in a spreadsheet's clothing, so it is a flat table
// now.
//
// The columns are taken by name from the header row, so their order may change:
//
//   Situation code name, RecordCode, FileName, Probability, SUBSET Code
//
// Deliberately simple, and these are the simplifications, all of which can be
// made configurable the day a file needs it:
//
//   * UTF-8, with a byte order mark skipped if present. Excel's plain "CSV"
//     export writes the system's ANSI code page instead, which for this file's
//     Russian would arrive as mojibake, so export as "CSV UTF-8".
//   * The separator is a comma or a semicolon, whichever the header row has
//     more of. Excel uses the system list separator, which is a semicolon on a
//     Russian Windows, so a file exported on one machine need not match a file
//     exported on another.
//   * Quoting is RFC 4180: a field may be wrapped in double quotes, and a
//     doubled quote inside those is one quote. Not backslash escapes.
bool LoadAcksTable( std::vector<SAckEntry> *pRes, const std::string &szFileName );

}
