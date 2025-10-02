#pragma once

#include "StringResources.h"
#include "UnitDesignTypes.h"
#include "HeightStateV3.h"
#include "FieldState.h"
#include "MapObjectMultiState.h"
#include "VSOMultiState.h"
#include "../System/XmlSaver.h"


class CMapInfoEditorSettings
{
public:
	typedef hash_map<int,int> CActiveStateMap;
	//
	bool bFitToGrid;
	bool bRotateTo90Degree;
	int nLastLoadedMap;
	CVec3 vLastMapCameraAnchor;
	//	
	bool bShowShortcutBar;
	bool bShowMinimapBar;
	bool bShowMoviesEditor;
	bool bShowMapInfoToolsToolbar;
	bool bShowMapInfoViewToolbar;
	bool bDrawShootAreas;
	bool bDrawAIMap;
	bool bDrawPassability;
	//
	CActiveStateMap activeStateMap;
	int nActiveStateIndex;
	//
	CHeightStateV3::SEditParameters epHeightStateV3;
	CFieldState::SEditParameters epFieldState;
	CMapObjectMultiState::SEditParameters epMapObjectMultiState;
	CVSOMultiState::SEditParameters epVSOMultiState;
	//
#include "ViewFilterData.h"
	SViewFilterData viewFilterData;
#include "AIMarkerSettings.h"
	SAIMarkerSettings sightRangeMarkerSettings;
	SAIMarkerSettings fireRangeMarkerSettings;
	//
	CMapInfoEditorSettings();
	// serializing...
	int operator&( IXmlSaver &xs );
};


