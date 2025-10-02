#if !defined(__TRENCH_MNEMONICS__)
#define __TRENCH_MNEMONICS__
#pragma once

#include "..\MapEditorLib\Tools_MnemonicsCollector.h"

class CEntrenchmentSegmentTypeMnemonics : public CMnemonicsCollector<int>
{
public:
	CEntrenchmentSegmentTypeMnemonics();
};

extern CEntrenchmentSegmentTypeMnemonics typeEntrenchmentSegment;

#endif //#if !defined(__TRENCH_MNEMONICS__)

