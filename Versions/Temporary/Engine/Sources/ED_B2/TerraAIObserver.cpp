#include "stdafx.h"

#include "../Stats_B2_M1/Vis2AI.h"
#include "TerraAIObserver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CTerraAIObserverInEditor::CTerraAIObserverInEditor( const int nAIMapSizeX, const int nAIMapSizeY )
{
	CTerraAIObserver::CTerraAIObserver();

	pAIMap = new CAIMap( nAIMapSizeX, nAIMapSizeY, AI_TILE_SIZE, MAXIMUM_UNIT_TILE_RADIUS, MAXIMUM_MAP_SIZE );
	pTerrain = pAIMap->GetTerrain();
	pHeights = pAIMap->GetHeights();

	InitPassMarkers( pAIMap );
}


