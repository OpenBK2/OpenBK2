#pragma once

#include "MapEditorLib/Tools_MnemonicsCollector.h"
#include "Stats_B2_M1/AnimationType.h"

class CMayaAnimationMnemonics : public CMnemonicsCollector<int>
{
	static const char DECIMAL_NUMBERS[];
	public:
	CMayaAnimationMnemonics();
	NDb::EAnimationType Get( const std::string &rszMnemonicType, std::string *pszMnemonicLabel, unsigned *pnNumber );
};


class CAnimationMnemonics : public CMnemonicsCollector<int>
{
	public:
	CAnimationMnemonics();
};


extern CMayaAnimationMnemonics typeMayaAnimationMnemonics;
extern CAnimationMnemonics typeAnimationMnemonics;


