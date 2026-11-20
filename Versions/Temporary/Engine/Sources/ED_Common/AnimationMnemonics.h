#pragma once

#include "MapEditorLib/Tools_MnemonicsCollector.h"

#include "ED_Common_export.h"

#include <string>

namespace NDb
{
enum EAnimationType : int;
}

class CMayaAnimationMnemonics : public CMnemonicsCollector<int>
{
	static const char DECIMAL_NUMBERS[];

public:
	CMayaAnimationMnemonics();
	ED_COMMON_EXPORT NDb::EAnimationType
	Get( const std::string &rszMnemonicType, std::string *pszMnemonicLabel, unsigned *pnNumber );
};

class CAnimationMnemonics : public CMnemonicsCollector<int>
{
public:
	CAnimationMnemonics();
};

extern ED_COMMON_EXPORT CMayaAnimationMnemonics typeMayaAnimationMnemonics;
extern ED_COMMON_EXPORT CAnimationMnemonics typeAnimationMnemonics;
