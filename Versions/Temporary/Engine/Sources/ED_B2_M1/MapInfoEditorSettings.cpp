#include "StdAfx.h"
#include "MapInfoEditorSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMapInfoEditorSettings::CMapInfoEditorSettings()
	: bShowShortcutBar( true ),
		bShowMinimapBar( true ),
		bShowMoviesEditor( false ),
		bShowMapInfoToolsToolbar( true ),
		bShowMapInfoViewToolbar( false ),
		bFitToGrid( false ),
		bRotateTo90Degree( false ),
		bDrawShootAreas( false ),
		bDrawAIMap( false ),
		bDrawPassability( false ),
		nLastLoadedMap( 0 ),
		vLastMapCameraAnchor( VNULL3 ),
		nActiveStateIndex( 0 )
{
}

int CMapInfoEditorSettings::operator&( IXmlSaver &xs )
{
	xs.Add( "ShowShortcutBar", &bShowShortcutBar );
	xs.Add( "ShowMinimapBar", &bShowMinimapBar );
	xs.Add( "ShowMoviesEditor", &bShowMoviesEditor );
	xs.Add( "ShowMapInfoToolsToolbar", &bShowMapInfoToolsToolbar );
	xs.Add( "ShowMapInfoViewToolbar", &bShowMapInfoViewToolbar );
	xs.Add( "FitToGrid", &bFitToGrid );
	xs.Add( "RotateTo90Degree", &bRotateTo90Degree );
	xs.Add( "LastLoadedMap", &nLastLoadedMap );
	xs.Add( "LastMapCameraAnchor", &vLastMapCameraAnchor );
	xs.Add( "viewFilterData", &viewFilterData );
	//
	xs.Add( "ActiveStateMap", &activeStateMap );
	xs.Add( "ActiveStateIndex", &nActiveStateIndex );
	//
	xs.Add( "HeightStateV3", &epHeightStateV3 );
	xs.Add( "FieldState", &epFieldState );
	xs.Add( "MapObjectMultiState", &epMapObjectMultiState );
	xs.Add( "VSOMultiState", &epVSOMultiState );
	return 0;
}

/**

bool CMapInfoEditorSettings::MustOrientToNormale( const string &rsObjectTypeName )
{
	return true;
}


bool CMapInfoEditorSettings::MustFitToGrid( const string &rsObjectTypeName )
{
	return true;
}


bool CMapInfoEditorSettings::MustRotateTo90Degrees( const string &rsObjectTypeName )
{
	return true;
}
/**/

// basement storage  


