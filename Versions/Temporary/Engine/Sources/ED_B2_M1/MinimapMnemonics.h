#pragma once

#include "../MapEditorLib/Tools_MnemonicsCollector.h"

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


