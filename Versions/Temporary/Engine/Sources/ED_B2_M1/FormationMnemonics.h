#if !defined(__FORMATION_MNEMONICS__)
#define __FORMATION_MNEMONICS__
#pragma once

#include "../MapEditorLib/Tools_MnemonicsCollector.h"

class CFormationMnemonics : public CMnemonicsCollector<int>
{
public:
	CFormationMnemonics();
};

extern CFormationMnemonics typeFormationMnemonics;

#endif // #if !defined(__FORMATION_MNEMONICS__)


