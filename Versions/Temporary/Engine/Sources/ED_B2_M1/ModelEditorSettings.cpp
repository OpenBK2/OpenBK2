
#include "StdAfx.h"
#include "ModelEditorSettings.h"

#include "libdb/Manipulator.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/XmlSaver.h"

const CVec3 CModelEditorSettings::vShift = CVec3( VIS_TILE_SIZE / 8.0f, VIS_TILE_SIZE / 8.0f, VIS_TILE_SIZE / 8.0f );
const float CModelEditorSettings::fDefaultDiff = VIS_TILE_SIZE / 64;
//
CModelEditorSettings::SCameraPlacement::SCameraPlacement()
	:	vAnchor( VNULL3 ),
		fDistance( VIS_TILE_SIZE * 16.0f ),
		fPitch( 45.0f ),
		fYaw( 45.0f ),
		fFOV( 26.0f ),
		bValid( false ) {}

int CModelEditorSettings::SCameraPlacement::operator&( IXmlSaver &xs )
{
	xs.Add( "Anchor", &vAnchor );
	xs.Add( "Distance", &fDistance );
	xs.Add( "Pitch", &fPitch );
	xs.Add( "Yaw", &fYaw );
	xs.Add( "FOV", &fFOV );
	xs.Add( "Valid", &bValid );
	return 0;
}

CModelEditorSettings::CModelEditorSettings()
	:	bShowToolbar( false ),
		bShowTool( true ),
		bDrawTerrain( true ),
		bDrawAnimations( true ),
		bDrawAIGeometry( false ),
		bDoubleSided( true ),
		bAnimationsCircle( true ),
		bShowGrid( true ),
		bShowSolidAIGeometry( false ),
		nGridSize( 32 ),
		vSceneColor( 0.0f, 0.0f, 0.0f ),
		vTerrainColor( 128.0f, 128.0f, 128.0f, 128.0f ),
		nMaxAnimationsCount( 16 ),
		fAnimationsCircleDistance( 12.0f ),
		fAnimationsBetweenDistance( 4.0f ),
		nGameTimerSpeed( 0 ) {}

int CModelEditorSettings::operator&( IXmlSaver &xs )
{

	xs.Add( "ShowToolbar", &bShowToolbar );
	xs.Add( "ShowTool", &bShowTool );
	xs.Add( "DrawTerrain", &bDrawTerrain );
	xs.Add( "DrawAnimations", &bDrawAnimations );
	xs.Add( "DrawAIGeometry", &bDrawAIGeometry );
	xs.Add( "DoubleSided", &bDoubleSided );
	xs.Add( "AnimationsCircle", &bAnimationsCircle );
	xs.Add( "ShowGrid", &bShowGrid );
	xs.Add( "bShowSolidAIGeometry", &bShowSolidAIGeometry );
	xs.Add( "DefaultCamera", &defaultCamera );
	xs.Add( "CurrentCamera", &currentCamera );
	xs.Add( "LightDBID", &lightDBID );
	xs.Add( "MapInfoDBID", &mapInfoDBID );
	xs.Add( "DefaultGeometryDBID", &defaultGeometryDBID );
	xs.Add( "DefaultMaterialDBID", &defaultMaterialDBID );
	xs.Add( "GridSize", &nGridSize );
	xs.Add( "SceneColor", &vSceneColor );
	xs.Add( "TerrainColor", &vTerrainColor );
	xs.Add( "MaxAnimationsCount", &nMaxAnimationsCount );
	xs.Add( "AnimationsCircleDistance", &fAnimationsCircleDistance );
	xs.Add( "AnimationsBetweenDistance", &fAnimationsBetweenDistance );
	xs.Add( "GameTimerSpeed", &nGameTimerSpeed );
	return 0;
}

// basement storage  


