// The .cll grammar against the case corpus Nival kept for it.
//
// Each directory under cases/ is one parse: every *.h in it (and in its
// subdirectories, which is where the include tests keep their targets) is fed
// to NLang::Parse, and the directory's suffix says what has to come of it,
// ".success" or ".fail". The corpus used to be driven by TestParsing, an
// executable that parsed one directory and returned the result as its exit
// code, because the parser kept its tree in globals that nothing reset. Parse
// now resets them itself, so every case runs in this one process.
//
// The real .cll files already prove that valid input parses (regenerate-db
// fails CI on any diff). What only this corpus checks is the other half: that
// the parser rejects what it should.
//
// Six cases contradicted the grammar Nival shipped, which is older than the
// corpus's last update of them; their expectations were flipped to match the
// grammar that parses every real .cll, and each carries a note.txt saying why.
//
// The cases are named *.h because that is what .cll files were called when the
// corpus was written; the grammar does not care.

#include <algorithm>
#include <filesystem>
#include <list>
#include <ostream>
#include <string>
#include <vector>

// LangNode.h names CObjectBase without including it, expecting Parser's
// stdafx.h to have got there first. This is the part of that prelude it needs.
#include "Misc/Asserts.h"
#include "System/System.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "Parser/LangNode.h"

#include <gtest/gtest.h>

namespace {

struct SCase
{
	std::string szName;			// the directory name, "007.fail"
	std::string szDir;			// its full path
	bool bExpectSuccess;
};

// without it a failure names the case by the bytes of the struct
void PrintTo( const SCase &c, std::ostream *pStream )
{
	*pStream << c.szName;
}

std::vector<SCase> CollectCases()
{
	std::vector<SCase> cases;
	std::error_code ec;
	for ( const auto &entry : std::filesystem::directory_iterator( PARSER_CASES_DIR, ec ) )
	{
		if ( !entry.is_directory() )
		{
			continue;
		}
		const std::filesystem::path &path = entry.path();
		const std::string szExt = path.extension().string();
		if ( szExt != ".success" && szExt != ".fail" )
		{
			continue;
		}
		cases.push_back( { path.filename().string(), path.generic_string(), szExt == ".success" } );
	}
	// directory order is up to the filesystem
	std::sort( cases.begin(), cases.end(), []( const SCase &a, const SCase &b ) { return a.szName < b.szName; } );
	return cases;
}

class CParserCase : public testing::TestWithParam<SCase>
{
};

TEST_P( CParserCase, Parses )
{
	const SCase &c = GetParam();
	const bool bSuccess = NLang::Parse( c.szDir, "*.h", true );
	if ( bSuccess != c.bExpectSuccess )
	{
		// Test mode is silent, which leaves an unexpected failure with nothing to
		// go on. Parse the case again with the errors printed.
		NLang::Parse( c.szDir, "*.h", false );
	}
	EXPECT_EQ( bSuccess, c.bExpectSuccess ) << c.szDir;
}

INSTANTIATE_TEST_SUITE_P( Corpus, CParserCase, testing::ValuesIn( CollectCases() ),
	[]( const testing::TestParamInfo<SCase> &info )
	{
		// "007.fail" -> "c007_fail"; a test name may only hold letters, digits and '_'
		std::string szName = "c" + info.param.szName;
		std::replace( szName.begin(), szName.end(), '.', '_' );
		return szName;
	} );

// Guards the instantiation above against a moved or empty corpus, which would
// otherwise pass by running nothing.
TEST( CParserCorpus, IsPresent )
{
	EXPECT_GE( CollectCases().size(), 100u ) << PARSER_CASES_DIR;
}

} // namespace
