#pragma once

#include "StringResources.h"
#include "UnitDesignTypes.h"
#include "HeightStateV3.h"
#include "FieldState.h"
#include "MapObjectMultiState.h"
#include "VSOMultiState.h"
#include "System/XmlSaver.h"


class CMapInfoEditorSettings
{
public:
	typedef std::unordered_map<int,int> CActiveStateMap;
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
	// sightRangeMarkerSettings and fireRangeMarkerSettings were here, with the
	// AIMarkerSettings.h that declared their type included in the class body.
	// Nothing read them, wrote them or saved them; the only thing that would
	// have was CMapInfoAIMarkersSelectDlg, which nothing opened. All three are
	// gone.
	//
	CMapInfoEditorSettings();
	// serializing...
	int operator&( IXmlSaver &xs );
};


