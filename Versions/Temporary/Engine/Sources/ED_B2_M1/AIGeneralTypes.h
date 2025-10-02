#if !defined(__AI_GENERAL_TYPES__)
#define __AI_GENERAL_TYPES__
#pragma once

#include "..\MapEditorLib\Tools_MnemonicsCollector.h"

class CAIGeneralParcelTypeMnemonics : public CMnemonicsCollector<int>
{
public:
	CAIGeneralParcelTypeMnemonics();
};

extern CAIGeneralParcelTypeMnemonics typeAIGeneralParcel;

#endif //#if !defined(__AI_GENERAL_TYPES__)


