#pragma once

#include "AILogic_export.h"

// Headless simulation clients need the same map context as in-game path states.
AILOGIC_EXPORT class CAIMap* GetAIMap();
AILOGIC_EXPORT void SetAIMap( class CAIMap* pAIMap );

class CTerrain* GetTerrain();
class CStaticMapHeights* GetHeights();


