#pragma once

#include "../MapEditorLib/Tools_MnemonicsCollector.h"

#define NDB_DEFAULT_SEASON NDb::SEASON_SUMMER
#define NDB_DEFAULT_SEASON_MNEMONIC "SEASON_SUMMER"
#define NDB_DEFAULT_SEASON_FILE_POSTFIX_MNEMONIC ""
#define NDB_DEFAULT_SEASON_OBJECT_POSTFIX_MNEMONIC "Summer"


class CSeasonMnemonics : public CMnemonicsCollector<int>
{
public:
	CSeasonMnemonics();
};


class CSeasonFilePostfixMnemonics : public CMnemonicsCollector<int>
{
public:
	CSeasonFilePostfixMnemonics();
};


class CSeasonFolderPostfixMnemonics : public CMnemonicsCollector<int>
{
public:
	CSeasonFolderPostfixMnemonics();
};


extern CSeasonMnemonics typeSeasonMnemonics;
extern CSeasonFilePostfixMnemonics typeSeasonFilePostfixMnemonics;
extern CSeasonFolderPostfixMnemonics typeSeasonFolderPostfixMnemonics;



