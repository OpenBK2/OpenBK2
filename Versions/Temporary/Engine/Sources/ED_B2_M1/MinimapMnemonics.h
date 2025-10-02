#if !defined(__MINIMAP_MNEMONICS__)
#define __MINIMAP_MNEMONICS__
#pragma once

#include "..\MapEditorLib\Tools_MnemonicsCollector.h"

class CMinimapLayerMnemonics : public CMnemonicsCollector<int>
{
public:
	CMinimapLayerMnemonics();
};

class CImageScaleMethod : public CMnemonicsCollector<int>
{
public:
	CImageScaleMethod();
};

extern CMinimapLayerMnemonics typeMinimapLayer;
extern CImageScaleMethod typeImageScaleMethod;

#endif //#if !defined(__MINIMAP_MNEMONICS__)
