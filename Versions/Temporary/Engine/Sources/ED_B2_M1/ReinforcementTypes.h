#if !defined(__REINFORCEMENT_TYPES__)
#define __REINFORCEMENT_TYPES__
#pragma once

#include "../MapEditorLib/Tools_MnemonicsCollector.h"

class CReinforcementTypeMnemonics : public CMnemonicsCollector<int>
{
public:
	CReinforcementTypeMnemonics();
};

extern CReinforcementTypeMnemonics typeReinforcementMnemonics;

#endif //#if !defined(__DESIGN_TYPES__)


