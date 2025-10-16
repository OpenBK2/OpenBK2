#include "stdafx.h"

#include "3Dmotor/Gfx.h"
#include "Image/Targa.h"
#include "System/VFSOperations.h"
#include "WindController.h"
#include "SceneInternal.h"
#include "DBSceneConsts.h"
#include "VisObjSelection.h"

#include <fmt/format.h>

namespace NImage
{
	template <> void __fill_tga_header<NGfx::SPixel8888>( STGAFileHeader *pHdr )
	{
		pHdr->cImageType = TGAIT_TRUE_COLOR;
		pHdr->imagespec.descriptor.cAlphaChannelBits = 8;
	}
};

void TakeScreenShotMsg( const SGameMessage &msg )
{
	CArray2D<NGfx::SPixel8888> data;
	NGfx::MakeScreenShot( &data, false );
	for ( int x = 0; x < data.GetSizeX(); ++x )
		for ( int y = 0; y < data.GetSizeY(); ++y )
			data[y][x].a = 0xFF;

	SYSTEMTIME systime;
	GetLocalTime( &systime );
	const std::string szFileName =
		fmt::format( "screenshots\\shot-{:04d}.{:02d}.{:02d}-{:02d}.{:02d}.{:02d}.tga",
		int(systime.wYear), int(systime.wMonth), int(systime.wDay),
		int(systime.wHour), int(systime.wMinute), int(systime.wSecond) );

	CFileStream stream( NVFS::GetMainFileCreator(), szFileName );
	NImage::SaveAsTGA( data, &stream );
}

// SSceneData::SScreenData

SSceneData::SScreenData::SScreenData( IWindow *_pScreen ) : 
	pScreen( _pScreen )
{
	p3DView = NGScene::CreateNewView();
	p3DView->WaitForLoad( true );
}

void SSceneData::SScreenData::SetSceneConsts( const NDb::SSceneConsts *pSceneConsts )
{
	if ( p3DView && pSceneConsts->pInterfaceLight )
		p3DView->SetAmbient( pSceneConsts->pInterfaceLight );
}

// SSceneData

SSceneData::SSceneData() : 
	bIsInternal( false ), 
	nFreeSelectionID( 0 )
{
	eAIGeomMode = EAI_GEOM_NONE;
	timeStatLastShow = 0;
	nStatNumFrames = 0;
	nStatNumVerts = 0;
	nStatNumTris = 0;
	fAveOverdraw = 0;
	nFPSDropCounter = 0;
}

void SSceneData::Init()
{
	showModes[SCENE_SHOW_UI] = true;
	showModes[SCENE_SHOW_TERRAIN] = true;
	showModes[SCENE_SHOW_SHADOWS] = ( NGlobal::GetVar( "gfx_noshadows", 0 ) == 0 );
	showModes[SCENE_SHOW_BBOXES] = false;
	showModes[SCENE_SHOW_PASS_MARKERS] = false;
	//
	nLastFreeID = 1;
	//
	bEnableStatistics = false;
	timeStatLastShow = 0;
	nStatNumFrames = 0;
	nStatNumTris = 0;
	nStatNumVerts = 0;
	fAveOverdraw = 0;
	nFPSDropCounter = 0;

	pAbsTimer = new CCSTime();
	pGameTimer = new CCSTime();
	//
	markers.resize( _ESMT_ENUM_COUNTER );

	pWindController = new CWindController( GameTimer(), 0, 0 );	//CRAP need to take parameters from MapInfo
	
	pAbsTimer->Set( GameTimer()->GetAbsTime() );
	pGameTimer->Set( GameTimer()->GetGameTime() );

	pWeather = 0;
}

SVisObjSelectionHandler* SSceneData::GetNewSelection()
{
	CPtr<SVisObjSelectionHandler> pHandler = new SVisObjSelectionHandler();
	selectionHandlers[nFreeSelectionID] = pHandler;
	pHandler->nID = nFreeSelectionID;
	nFreeSelectionID++;
	return pHandler;
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3013C400, SSceneData );


