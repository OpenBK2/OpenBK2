#include "stdafx.h"
#include "../Stats_B2_M1/Season.h"
#include "SeasonMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSeasonMnemonics::CSeasonMnemonics() : CMnemonicsCollector<int>( NDB_DEFAULT_SEASON, NDB_DEFAULT_SEASON_MNEMONIC )
{
	Insert( NDb::SEASON_WINTER, "SEASON_WINTER" );
	Insert( NDb::SEASON_SPRING, "SEASON_SPRING" );
	Insert( NDb::SEASON_SUMMER, "SEASON_SUMMER" );
	Insert( NDb::SEASON_AUTUMN, "SEASON_AUTUMN" );
	Insert( NDb::SEASON_AFRICA, "SEASON_AFRICA" );
	Insert( NDb::SEASON_ASIA,   "SEASON_ASIA"   );
}


CSeasonFilePostfixMnemonics::CSeasonFilePostfixMnemonics() : CMnemonicsCollector<int>( NDB_DEFAULT_SEASON, NDB_DEFAULT_SEASON_FILE_POSTFIX_MNEMONIC )
{
	Insert( NDb::SEASON_WINTER,	"w" );
	Insert( NDb::SEASON_SPRING,	"s" );
	Insert( NDb::SEASON_SUMMER,	"" );
	Insert( NDb::SEASON_AUTUMN,	"u" );
	Insert( NDb::SEASON_AFRICA,	"a" );
	Insert( NDb::SEASON_ASIA,		"i" );
}


CSeasonFolderPostfixMnemonics::CSeasonFolderPostfixMnemonics() : CMnemonicsCollector<int>( NDB_DEFAULT_SEASON, NDB_DEFAULT_SEASON_OBJECT_POSTFIX_MNEMONIC )
{
	Insert( NDb::SEASON_WINTER,	"Winter" );
	Insert( NDb::SEASON_SPRING,	"Spring" );
	Insert( NDb::SEASON_SUMMER,	"Summer" );
	Insert( NDb::SEASON_AUTUMN,	"Autumn" );
	Insert( NDb::SEASON_AFRICA,	"Africa" );
	Insert( NDb::SEASON_ASIA,		"Asia" );
}


CSeasonMnemonics typeSeasonMnemonics;
CSeasonFilePostfixMnemonics typeSeasonFilePostfixMnemonics;
CSeasonFolderPostfixMnemonics typeSeasonFolderPostfixMnemonics;


