#include "stdafx.h"

#include "../Common_RTS_AI/AIMap.h"

static CAIMap* pAIMap;
void SetAIMap( class CAIMap* _pAIMap )
{
	pAIMap = _pAIMap;
}

class CAIMap* GetAIMap()
{
	return pAIMap;
}

class CTerrain* GetTerrain()
{
	return pAIMap->GetTerrain();
}

class CStaticMapHeights* GetHeights()
{
	return pAIMap->GetHeights();
}

