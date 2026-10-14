#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "AIClasses.h"
#include "AIMap.h"
#include "DebugTools/DebugInfoManager.h"
#include "Terrain.h"

#include "Common_RTS_AI_export.h"

#define MARKERS_COLORS_COUNT 3

class COMMON_RTS_AI_EXPORT CPassMarkersDraw : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CPassMarkersDraw )

	CPtr<CTerrain> pTerrain;
	CPtr<CAIMap> pAIMap;

	// markers variables
	int markers_id[MARKERS_COLORS_COUNT]{};
	int markers_radiuses[MARKERS_COLORS_COUNT]{};
	EAIClasses markers_aiClasses[MARKERS_COLORS_COUNT]{};
	EFreeTileInfo markers_freeClass[MARKERS_COLORS_COUNT]{};
	NDebugInfo::EColor markers_colors[MARKERS_COLORS_COUNT]{};
	int nWaterMarker = 0;
	bool bDrawMarkers = false;

	NTimer::STime lastPassabilityUpdate = 0;

	void DrawPassabilities1();
	void Reset();

public:
	CPassMarkersDraw();

	~CPassMarkersDraw() override {}

	void Init( CAIMap *_pAIMap );
	void Clear();
	void DrawPassabilities();

	void UpdatePassMarkers();
	void ToggleDrawPassMarkers();
	void ToggleDrawPassMarkers1();
	void SetPassMarkers( const NDebugInfo::EColor color,
	                     const EAIClasses aiClass,
	                     const EFreeTileInfo freeClass,
	                     const int nBoundTileRadius );
};
