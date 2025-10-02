#if !defined(__DESIGN_TYPES__)
#define __DESIGN_TYPES__
#pragma once

#include "../MapEditorLib/Tools_MnemonicsCollector.h"

class CUnitDesignTypeMnemonics : public CMnemonicsCollector<int>
{
public:
	CUnitDesignTypeMnemonics();
};

extern CUnitDesignTypeMnemonics typeUnitDesignTypeMnemonics;

#endif //#if !defined(__DESIGN_TYPES__)


