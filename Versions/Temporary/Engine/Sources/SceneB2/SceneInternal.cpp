#include "stdafx.h"

#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "3Dmotor/Gfx.h"
#include "3Dmotor/GInit.h"
#include "3Dmotor/GRTInfo.h"
#include "3Dmotor/GRTShare.h"
#include "3Dmotor/GSceneUtils.h"
#include "3Dmotor/aiMap.h"

#include "3Dmotor/GfxBuffers.h"
#include "3Dmotor/GObjectInfo.h"

#include "3DLib/Transform.h"
#include "UI/UI.h"
#include "Main/GameTimer.h"
#include "System/Commands.h"
#include "Image/Targa.h"
#include "SceneUIVisitor.h"
#include "B2_M1_Terrain/TracksManager.h"
#include "DBSceneConsts.h"
#include "Cursor.h"
#include "Camera.h"
#include "StatSystem.h"
#include "VisObjDesc.h"
#include "SceneInternal.h"
#include "TerrainManager.h"
#include "ShotTrace.h"
#include "VisObjIconsManager.h"
#include "SceneHoldQueue.h"
#include "CameraMapIterator.h"
#include "FullScreenFader.h"
#include "3DLib/MemObject.h"
#include "3DLib/GMemBuilder.h"
#include "Common_RTS_AI/TerraAIObserver.h"
#include "Misc/StrProc.h"
#include "System/VFSOperations.h"

#include "Sound/MusicSystem.h"

#include <cstdint>

CUIVisitor theUIVisitor;
void TakeScreenShotMsg( const SGameMessage &msg );

static float s_fCameraNearPlane = 5;
static float s_fCameraFarPlane = 500;
static bool s_bPerspectiveCamera = true;
static float s_fMinStaticFPS = 25;
static float s_fMinDynamicFPS = 1;
static float s_fMinDynamicFPSMeasurePeriods = 3;

const CVec4 vDefaultColor( 0.0f, 0.0f, 0.0f, 1.0f );

namespace NGamma
{
	static float s_fBrightness = 0.5f;
	static float s_fContrast = 0.5f;
	static float s_fGamma = 0.5f;
}

IScene* CreateScene()
{
	return new CScene();
}

// CScene

CScene::CScene()
: eScene( ES_UNKNOWN ), vBackgroundColor( vDefaultColor ), timeBadWeatherLeft( 0 ), bGetSizesFromTarget( false )
{
	observers.AddObserver( "scene_show_bboxes", &CScene::ToggleShowMsg, SCENE_SHOW_BBOXES );
	observers.AddObserver( "scene_show_grid", &CScene::ToggleShowMsg, SCENE_SHOW_GRID );
	observers.AddObserver( "wireframe", &CScene::ToggleShowMsg, SCENE_SHOW_WIREFRAME );
	observers.AddObserver( "scene_show_ui", &CScene::ToggleShowMsg, SCENE_SHOW_UI );
	observers.AddObserver( "scene_show_mip", &CScene::ToggleShowMsg, SCENE_SHOW_MIPMAPS );
	observers.AddObserver( "scene_show_overdraw", &CScene::ToggleShowMsg, SCENE_SHOW_OVERDRAW );
	observers.AddObserver( "scene_show_statistics", &CScene::ToggleShowMsg, SCENE_SHOW_STATISTICS );
	observers.AddObserver( "scene_show_shadows", &CScene::ToggleShowMsg, SCENE_SHOW_SHADOWS );
	observers.AddObserver( "scene_show_terrain", &CScene::ToggleShowMsg, SCENE_SHOW_TERRAIN );
	observers.AddObserver( "scene_show_ai_geom", &CScene::ToggleShowMsg, SCENE_SHOW_AI_GEOM );
	observers.AddObserver( "toggle_ai_passability", &CScene::ToggleShowMsg, SCENE_SHOW_PASS_MARKERS );
	observers.AddObserver( "make_map_shot", &CScene::MakeMapShot );
	observers.AddObserver( "screenshot", &TakeScreenShotMsg );
	observers.AddObserver( "measure_overdraw", &CScene::CalcAverageOverdrawMsg );

	bEditorMode = false;
}

void CScene::CreateGScene()
{
	if ( eScene == ES_UNKNOWN )
		SwitchScene( SCENE_MISSION );

	if ( eScene == SCENE_2D )
	{
		// create new GScene
		data[eScene]->p2DView = NGScene::CreateNew2DView();
		//
		const CVec2 vSize = NGfx::GetScreenRect();
		Cursor()->SetBounds( 0, 0, vSize.x, vSize.y );

		theUIVisitor.SetGView(  data[eScene]->p2DView );
		Singleton<IUIInitialization>()->Set2DGameView( data[eScene]->p2DView );

		return;
	}

	// create new GScene
	data[eScene]->SetGScene( NGScene::CreateNewView() );
	data[eScene]->p2DView = NGScene::CreateNew2DView();
	data[eScene]->pInterfaceView = NGScene::CreateNewView();
	data[eScene]->GetGScene()->WaitForLoad( true );
	data[eScene]->pInterfaceView->WaitForLoad( true );
	//
	data[eScene]->pSyncSrc = new CSceneSyncSrc();
	data[eScene]->pAIMap = NAI::CreateAIMap();
	data[eScene]->pAIMapVisitor = new CAIMapVisitor( data[eScene]->pSyncSrc, data[eScene]->pAIMap );
	//
	data[eScene]->pIconSyncSrc = new CIconSceneSyncSrc();
	data[eScene]->pIconAIMap = NAI::CreateAIMap();
	data[eScene]->pIconAIMapVisitor = new CIconAIMapVisitor( data[eScene]->pIconSyncSrc, data[eScene]->pIconAIMap );
	//
	data[eScene]->pVisObjIconsManager = 0;
	if ( data[eScene]->pSceneConsts && data[eScene]->pSceneConsts->pVisObjIconsSet )
	{
		data[eScene]->pVisObjIconsManager = new CVisObjIconsManager;
		data[eScene]->pVisObjIconsManager->Init( data[eScene]->pSceneConsts->pVisObjIconsSet );
		data[eScene]->pVisObjIconsManager->Attach2DView( data[eScene]->p2DView );
	}
	if ( data[eScene]->pTracksManager == 0 )
		data[eScene]->pTracksManager = new CTracksManager( data[eScene]->pGameTimer, data[eScene]->GetGScene() );
	if ( data[eScene]->pSceneConsts ) 
		data[eScene]->pTracksManager->SetTracksMaterial( data[eScene]->pSceneConsts->trackMaterials.pTrack );

	const CVec2 vSize = NGfx::GetScreenRect();
	Cursor()->SetBounds( 0, 0, vSize.x, vSize.y );

	theUIVisitor.SetGView( data[eScene]->p2DView );
	Singleton<IUIInitialization>()->Set2DGameView( data[eScene]->p2DView );

	data[eScene]->pScreenFader = CreateFullScreenFader();

	NGScene::SetLoadMode( NGScene::E_PURE_GRANNY );
}

void CScene::Init()
{
	if ( eScene == ES_UNKNOWN )
		SwitchScene( SCENE_MISSION );
	else
		data[eScene]->Init();
}

void CScene::SetSceneConsts( const NDb::SSceneConsts *_pSceneConsts )
{
	if ( (data.size() <= eScene) && (eScene != ES_UNKNOWN) )
		return;
	
	if ( _pSceneConsts )
	{
		if ( eScene == ES_UNKNOWN )
			SwitchScene( SCENE_MISSION );

		InitDebugMaterials( _pSceneConsts );

		data[eScene]->pSceneConsts = _pSceneConsts;
		if ( data[eScene]->pTracksManager ) 
			data[eScene]->pTracksManager->SetTracksMaterial( data[eScene]->pSceneConsts->trackMaterials.pTrack );
		//
		if ( data[eScene]->pInterfaceView && _pSceneConsts->pInterfaceLight )
			data[eScene]->pInterfaceView->SetAmbient( _pSceneConsts->pInterfaceLight, data[eScene]->pGameTimer );
			
		for ( SSceneData::CScreensData::iterator it = data[eScene]->screensData.begin(); it != data[eScene]->screensData.end(); ++it )
		{
			SSceneData::SScreenData &data = *it;
			data.SetSceneConsts( _pSceneConsts );
		}
	}
}

const NDb::SSceneConsts *CScene::GetSceneConsts()
{
	return data[eScene]->pSceneConsts;
}

bool CScene::SetupMode( ESceneMode eMode, bool _bEditorMode )
{
	bEditorMode = _bEditorMode;
	if ( eScene == ES_UNKNOWN )
		SwitchScene( SCENE_MISSION );

	//
	if ( eMode == SCENE_MODE_FULLSCREEN ) 
		NGlobal::SetVar( "gfx_fullscreen", 1 );
	else if ( eMode == SCENE_MODE_WINDOWED ) 
		NGlobal::SetVar( "gfx_fullscreen", 0 );
	//
	NGScene::SUserRTInfo rtInfo;
	if ( NGlobal::GetVar( "m1", 0 ) == 1 )
		rtInfo.AddTex( 1024, "EFFECT_RT" );
	if ( !NGScene::SetModeFromConfig(true, rtInfo) )
	{
		NI_ASSERT( false, "Can't set mode from config" ); // no mode found
		return false;
	}
	//
	const CVec2 vSize = NGfx::GetScreenRect();
	Cursor()->SetBounds( 0, 0, vSize.x, vSize.y );

	theUIVisitor.SetGView( data[eScene]->p2DView );
	Singleton<IUIInitialization>()->Set2DGameView( data[eScene]->p2DView );
	//
	return true;
}

void CScene::ClearScene( const EScene eScene2Clear )
{
	if ( eScene2Clear >= data.size() )
		return;
	ClearSceneHoldQueue();
	//
	CDBPtr<NDb::SSceneConsts> pSceneConsts = data[eScene2Clear]->pSceneConsts;
	SSceneData::CScreensData oldScreenData = data[eScene2Clear]->screensData;
	data[eScene2Clear] = 0;
	
	EScene eSceneOld = eScene;
	eScene = ES_UNKNOWN;
	
	SwitchScene( eScene2Clear );
	SetSceneConsts( pSceneConsts );

	SwitchScene( eSceneOld );
	data[eScene2Clear]->screensData = oldScreenData;
}

CScene::~CScene()
{
	ClearSceneHoldQueue();
}

void CScene::SwitchScene( const EScene _eScene )
{
	if ( eScene != _eScene )
	{
		eScene = _eScene;
		if ( data.size() <= eScene )
			data.resize( eScene + 1 );

		if ( data[eScene] == 0 )
		{
			data[eScene] = new SSceneData();
			data[eScene]->Init();
			CreateGScene();
		}

		const CVec2 vSize = NGfx::GetScreenRect();
		Cursor()->SetBounds( 0, 0, vSize.x, vSize.y );

		theUIVisitor.SetGView( data[eScene]->p2DView );
		Singleton<IUIInitialization>()->Set2DGameView( data[eScene]->p2DView );
	}
}

bool CScene::ToggleShow( ESceneShow eShow )
{
	data[eScene]->showModes[eShow] = !data[eScene]->showModes[eShow];
	switch ( eShow ) 
	{
		case SCENE_SHOW_SHADOWS:
		{
			NGlobal::SetVar( "gfx_noshadows", (data[eScene]->showModes[eShow] ? 0 : 1) );
			break;
		}
		//
		case SCENE_SHOW_WIREFRAME:
		{
			NGScene::SetWireframe( data[eScene]->showModes[eShow] );
			break;
		}
		//
		case SCENE_SHOW_UI:
		break;
		//
		case SCENE_SHOW_BBOXES:
		{
			for ( SSceneData::CVisObjectsMap::iterator it = data[eScene]->visObjects.begin(); it != data[eScene]->visObjects.end(); ++it )
			{
				if ( SModelVisObjDesc *pVOD = dynamic_cast_ptr<SModelVisObjDesc *>(it->second) )
				{
					if ( data[eScene]->showModes[eShow] )
						pVOD->UpdateBBPolyLine( data[eScene]->GetGScene() );
					else
						pVOD->pBBPolyLine = 0;
				}
			}
		}
		break;
		//
		case SCENE_SHOW_GRID:
		case SCENE_SHOW_AI_GRID:
		{
			ShowTerrainGrid( eShow );
			break;
		}
		//
		case SCENE_SHOW_OVERDRAW:
		{
			data[eScene]->GetGScene()->SetRenderMode( data[eScene]->showModes[eShow] ? NGScene::SRM_SHOWOVERDRAW : NGScene::SRM_BEST );
			break;
		}
		//
		case SCENE_SHOW_MIPMAPS:
		{
			bool bShow = NGlobal::GetVar("gfx_draw_mip").GetFloat() != 0;
			bShow = !bShow;
			NGlobal::SetVar( "gfx_draw_mip", bShow );
		}
		break;
		//
		case SCENE_SHOW_STATISTICS:
		{
			data[eScene]->bEnableStatistics = !data[eScene]->bEnableStatistics;
			if ( data[eScene]->bEnableStatistics ) 
				NGlobal::ProcessCommand( L"Stats 1" );
			else
				NGlobal::ProcessCommand( L"Stats 0" );
			break;
		}
		//
		case SCENE_SHOW_TERRAIN:
		{
			data[eScene]->pTerraManager->HideTerrain( data[eScene]->showModes[eShow] );
			break;
		}
		//		
		case SCENE_SHOW_AI_GEOM:
		{
			CycleAIGeometryModes();
			break;
		}
		//
		case SCENE_SHOW_PASS_MARKERS:
		{
			ToggleShowPassability();
			break;
		}
	}

	return data[eScene]->showModes[eShow];
}

bool CScene::IsShowOn( ESceneShow eShow )
{
	switch ( eShow ) 
	{
		case SCENE_SHOW_WIREFRAME:
		case SCENE_SHOW_BBOXES:
		case SCENE_SHOW_GRID:
		case SCENE_SHOW_AI_GRID:
		case SCENE_SHOW_TERRAIN:
		case SCENE_SHOW_OVERDRAW:
		case SCENE_SHOW_PASS_MARKERS:
			return data[eScene]->showModes[eShow];
		case SCENE_SHOW_MIPMAPS:
		{
			bool bShow = NGlobal::GetVar("gfx_draw_mip").GetFloat() != 0;
			return bShow;
		}
		case SCENE_SHOW_SHADOWS:
		{
			bool bShow = NGlobal::GetVar("gfx_noshadows").GetFloat() != 0;
			return bShow;
		}
		case SCENE_SHOW_STATISTICS:
		{
			return data[eScene]->bEnableStatistics;
		}
		break;
	}
	return false;
}

static void OutputStat( const char *pszName, const int nValue, uint32_t dwColor, IStatSystem *pSS )
{
	const std::string szValue = StrFmt( "%d", nValue );
	pSS->UpdateEntry( pszName, szValue, dwColor );
}

static void OutputStat( const char *pszName, const float fValue, uint32_t dwColor, IStatSystem *pSS )
{
	const std::string szValue = StrFmt( "%g", fValue );
	pSS->UpdateEntry( pszName, szValue, dwColor );
}

static void OutputStat( const char *pszName, bool bValue, IStatSystem *pSS )
{
	uint32_t dwColor = 0xff00ff00;
	const char *pszValue = "false";
	if ( bValue ) 
	{
		dwColor = 0xffff0000;
		pszValue = "true";
	}
	pSS->UpdateEntry( pszName, pszValue, dwColor );
}

static void OutputStat( const char *pszName, const CVec3 &vValue, uint32_t dwColor, IStatSystem *pSS )
{
	const std::string szValue = StrFmt( "%g %g %g", vValue.x, vValue.y, vValue.z );
	pSS->UpdateEntry( pszName, szValue, dwColor );
}

static void CalcOrthographicScreenSize( float *pfSizeX, float *pfSizeY, float fScreenSizeX, float fScreenSizeY )
{
	float fDist, fYaw, fPitch;
	Camera()->GetPlacement( &fDist, &fPitch, &fYaw );
	const float fFOV = Camera()->GetFOV();
	*pfSizeX = 2.0f * fDist * tan( ToRadian(fFOV) * 0.5f );
	*pfSizeY = fScreenSizeY / fScreenSizeX * (*pfSizeX);
}

// brightness, contrast and gamma are in range [-1..1]
static void CalcGammaRamp2Bounded( NGfx::SPixel8888 *pRamp, float fBrightness, float fContrast, float fGamma )
{
	// calculate equation params for Y = A*X + B
	// contrast: a*x + b
	// если contrast < 0, то a = 1/a (наклон <45 градусов)
	float fA = 1.0f + 4.0f*fabs( fContrast );
	if ( fContrast < 0 )
		fA = 1.0f / fA;
	float fB = 0.5f*( 1.0f - fA );
	// gamma: x^power
	float fPower = 1;
	if ( fGamma > 0 )
		fPower = 1.0f / ( 5.0f*fGamma + 1 );
	else if ( fGamma < 0 )
		fPower = 1.0f / ( 0.5f*fGamma + 1 );
	// brightness: x + b
	//
	for ( int i = 0; i < 256; ++i )
	{
		float fVal = float( i ) / 255.0f;
		float fGammaValue = pow( fVal, fPower );
		float fContrastValue = Clamp( fA*fGammaValue + fB, 0.0f, 1.0f );
		float fResult = Clamp( fContrastValue + fBrightness, 0.0f, 1.0f );
		pRamp[i].r = fResult * 255.0f;
		pRamp[i].g = fResult * 255.0f;
		pRamp[i].b = fResult * 255.0f;
	}
}

inline void CalcGammaRamp2( NGfx::SPixel8888 *pRamp, float fBrightness, float fContrast, float fGamma )
{
//	fBrightness = Clamp( fBrightness, -1.0f, 1.0f ) * 0.5f; // to avoid complete dark and complete white values
//	fContrast = Clamp( fContrast, -1.0f, 1.0f ) * 0.5f;
//	fGamma = Clamp( fGamma, -1.0f, 1.0f ) * 0.5f;
	//
	CalcGammaRamp2Bounded( pRamp, fBrightness, fContrast, fGamma );
}

inline void CalcGammaRamp( NGfx::SPixel8888 *pRamp, float fBrightness, float fContrast, float fGamma )
{
	static const float s_fBoundFactor = 0.5f;
	CalcGammaRamp2( pRamp, (fBrightness - 0.5f)*2.0f * s_fBoundFactor, 
												 (fContrast - 0.5f)*2.0f * s_fBoundFactor, 
												 (fGamma - 0.5f)*2.0f );
}

void CScene::Draw( NGScene::CRTPtr *pTargetTexture )
{
	static float s_fLastGamma = NGamma::s_fGamma;
	static float s_fLastContrast = NGamma::s_fContrast;
	static float s_fLastBrightness = NGamma::s_fBrightness;
	//
	if ( NGamma::s_fGamma != s_fLastGamma || NGamma::s_fContrast != s_fLastContrast || NGamma::s_fBrightness != s_fLastBrightness )
	{
		s_fLastGamma = NGamma::s_fGamma;
		s_fLastContrast = NGamma::s_fContrast;
		s_fLastBrightness = NGamma::s_fBrightness;
		std::vector<NGfx::SPixel8888> gammaRamp( 256 );
		CalcGammaRamp( &(gammaRamp[0]), NGamma::s_fBrightness, NGamma::s_fContrast, NGamma::s_fGamma );
		NGfx::SetGammaRamp( gammaRamp );
	}
	//
	if ( bEditorMode ) 
	{
		data[eScene]->pAbsTimer->Set( GameTimer()->GetAbsTime() );
		data[eScene]->pGameTimer->Set( GameTimer()->GetGameTime() );
	}
	else
	{
		const STime &timeAbs = GameTimer()->GetAbsTime();
		if ( data[eScene]->pAbsTimer->GetValue() != timeAbs )
			data[eScene]->pAbsTimer->Set( timeAbs );
		const STime &timeGame = GameTimer()->GetGameTime();
		if ( data[eScene]->pGameTimer->GetValue() != timeGame )
			data[eScene]->pGameTimer->Set( timeGame );
	}

	if ( NGScene::Is3DActive() )
	{
		NGfx::CTexture *pTarget = 0;

		CVec2 vScreenSize = NGfx::GetScreenRect();
		if ( pTargetTexture )
		{
			pTarget = pTargetTexture->GetTexture();

			if ( bGetSizesFromTarget )
			{
				CDynamicCast<NGfx::I2DBuffer> pBuffer( pTarget );
				if ( pBuffer )
					vScreenSize = CVec2( pBuffer->GetSizeX(), pBuffer->GetSizeY() );
			}
		}

		UpdateSelectionHandlers();

		ICamera *pCamera = Camera();

		if ( s_bPerspectiveCamera )
			pCamera->SetPerspectiveTransform( vScreenSize.x, vScreenSize.y, s_fCameraNearPlane, s_fCameraFarPlane );
		else
		{
			float fSizeX = 0, fSizeY = 0;
			CalcOrthographicScreenSize( &fSizeX, &fSizeY, vScreenSize.x, vScreenSize.y );
			pCamera->SetOrthographicTransform( fSizeX, fSizeY, s_fCameraNearPlane, s_fCameraFarPlane );
		}

		MarkNewDGFrame();

		if ( data[eScene]->pTerraManager || !data[eScene]->visObjects.empty() || !data[eScene]->polylines.empty() )
		{
			if ( data[eScene]->GetGScene() ) 
			{
				pCamera->Update();
				NGScene::IGameView::SDrawInfo drawInfo;
				drawInfo.pTS = &( pCamera->GetTransform() );

//				if ( NGlobal::GetVar( "m1", 0 ) != 0 )
//				{
//					// Process fog parameters and update scene by it
//					if ( data[eScene]->pSceneAmbientLight && data[eScene]->pSceneAmbientLight->pDistanceFog )
//						vBackgroundColor = data[eScene]->pSceneAmbientLight->pDistanceFog->vColor;
//				}

				if ( vBackgroundColor != vDefaultColor )
				{
					drawInfo.rtClear.vColor = vBackgroundColor;
					drawInfo.bUseDefaultClearColor = false;
				}
				drawInfo.pTarget = pTarget;
				data[eScene]->GetGScene()->Draw( drawInfo );

				// Render AI map to screen for optimizing search selected and obstacles units
				if ( data[eScene]->pAIMap )
				{
					data[eScene]->pAIMapVisitor->Sync();

					fastRender.Init( pCamera->GetTransform(), 32 );
					data[eScene]->pAIMap->TraceGrid( &fastRender, -1, NAI::IAIMap::STH_SORT_INTERVALS );
				}
			}
		}
		else
		{
			NGScene::ClearScreen( CVec3(0.25f, 0.25f, 0.25f) );
		}
		// delete all tracks, for which this was planned
		if ( data[eScene]->pTracksManager ) 
			data[eScene]->pTracksManager->ProcessDelQueue();

		// 2D part
		if ( NGlobal::GetVar( "m1", 0 ) == 0 )
		{
			if ( data[eScene]->pScreenFader )
				data[eScene]->pScreenFader->Draw( GameTimer()->GetAbsTime(), false );

			data[eScene]->p2DView->StartNewFrame( pTarget );
		}

		if ( data[eScene]->pVisObjIconsManager )
		{
			if ( Camera()->WasUpdated() )
				data[eScene]->pVisObjIconsManager->UpdateAllIcons();
			data[eScene]->pVisObjIconsManager->DrawIcons();
		}

		if ( NGlobal::GetVar( "m1", 0 ) == 0 )
		{
			if ( !data[eScene]->screensData.empty() && data[eScene]->p2DView )
			{
				if ( data[eScene]->showModes[SCENE_SHOW_UI] != false ) 
				{
					theUIVisitor.SetGView( data[eScene]->p2DView );
					for ( SSceneData::CScreensData::iterator it = data[eScene]->screensData.begin(); it != data[eScene]->screensData.end(); ++it )
					{
						SSceneData::SScreenData &screenData = *it;
						screenData.pScreen->Visit( &theUIVisitor );
						
						if ( !screenData.interfaceVisObjects.empty() )
						{
							data[eScene]->p2DView->Flush();

							NGScene::IGameView::SDrawInfo drawInfo;
							CTransformStack ts;
			//				ts.MakeParallel( vScreenSize.x, vScreenSize.y, 100, -50 );
							ts.MakeParallel( INTERFACE_3D_ELEMENT_WIDTH,// / fInterfaceElementScaleFactor, 
															INTERFACE_3D_ELEMENT_HEIGHT,// / fInterfaceElementScaleFactor, 
															100, -50 );
							drawInfo.pTS = &ts;
							drawInfo.pTarget = pTarget;
							if ( vBackgroundColor != vDefaultColor )
							{
								drawInfo.rtClear.vColor = vBackgroundColor;
								drawInfo.bUseDefaultClearColor = false;
							}
							drawInfo.rtClear.ct = NGScene::SRTClearParams::CT_NONE;
							drawInfo.bShadows = false;
							screenData.p3DView->Draw( drawInfo );
						}
						// upper 2D part
						if ( data[eScene]->p2DView )
						{
							theUIVisitor.SetLowerLevel();
							theUIVisitor.FlushUpperLevelData();
						}
					}
				}
			}

			data[eScene]->p2DView->Flush();
			theUIVisitor.ClearCommandsList();

			if ( data[eScene]->pScreenFader )
				data[eScene]->pScreenFader->Draw( GameTimer()->GetAbsTime(), true );
		}

		if ( data[eScene]->bEnableStatistics ) 
		{
			IStatSystem *pSS = Singleton<IStatSystem>();

			NGScene::SRenderStats renderStats;
			NGScene::GetRenderStats( &renderStats );

			data[eScene]->nStatNumVerts += renderStats.nVertices;
			data[eScene]->nStatNumTris += renderStats.nTris;
			data[eScene]->nStatNumDIPs += renderStats.nDIPs;

			++data[eScene]->nStatNumFrames;
			//
			NTimer::STime timeAbs = GameTimer()->GetAbsTime();
			if ( data[eScene]->timeStatLastShow == 0 ) 
				data[eScene]->timeStatLastShow = timeAbs;
			else if ( data[eScene]->timeStatLastShow + 1000 < timeAbs )
			{
				const float fTimeCoeff = 1000.0f / float( timeAbs - data[eScene]->timeStatLastShow );
				// minimal allowed dynamic FPS check
				const float fFPS = float( data[eScene]->nStatNumFrames ) * fTimeCoeff;

				if ( fFPS < s_fMinDynamicFPS && NGlobal::GetVar( "m1", 0 ) == 0 )
				{
					data[eScene]->nFPSDropCounter++;
					if ( data[eScene]->nFPSDropCounter > s_fMinDynamicFPSMeasurePeriods )
					{
						csSystem << CC_RED << "FPS Too Low!!!" << endl;
						csSystem << CC_RED << StrFmt("FPS = %g, but minimal allowed = %g", fFPS, s_fMinDynamicFPS ) << endl;
						data[eScene]->nFPSDropCounter = 0;
					}
				}
				else
					data[eScene]->nFPSDropCounter = 0;
				//
				OutputStat( "FPS", fFPS, 0xff00ffff, pSS );
				OutputStat( "Verts", int( float(data[eScene]->nStatNumVerts)/data[eScene]->nStatNumFrames ), 0xffffffff, pSS );
				OutputStat( "Tris", int( float(data[eScene]->nStatNumTris)/data[eScene]->nStatNumFrames ), 0xffffffff, pSS );
				OutputStat( "DIPs", int( float(data[eScene]->nStatNumDIPs)/data[eScene]->nStatNumFrames ), 0xffffffff, pSS );
				//
				float fCamDist, fCamPitch, fCamYaw;
				Camera()->GetPlacement( &fCamDist, &fCamPitch, &fCamYaw );
				OutputStat( "Camera Distance", fCamDist, 0xffffff00, pSS );
				OutputStat( "Camera Pitch", fCamPitch, 0xffffff00, pSS );
				OutputStat( "Camera Yaw", fCamYaw, 0xffffff00, pSS );

				CVec3 vCameraOrigin = Camera()->GetViewMatrix().GetTranslation();
				OutputStat( "Camera Origin", vCameraOrigin, 0xffffff00, pSS );

				data[eScene]->timeStatLastShow = timeAbs;
				data[eScene]->nStatNumFrames = 0;
				data[eScene]->nStatNumVerts = 0;
				data[eScene]->nStatNumTris = 0;
				data[eScene]->nStatNumDIPs = 0;

				if ( int nMapID = NGlobal::GetVar( "MapInfo.MapDBID", 0 ).GetFloat() )
					OutputStat( "Map DBID", nMapID, 0xffffff00, pSS );
			}
			// calc each frame
			OutputStat( "PointLights", renderStats.nScenePointLights, 0xffffffff, pSS );
			OutputStat( "Particles", renderStats.nParticles, 0xffffffff, pSS );
			//
			OutputStat( "TextureMem", float(NGScene::CalcTouchedTextureSize() / (1024.0f*1024.0f)), 0xffffffff, pSS );
			//
			OutputStat( "StaticGeometryThrashing", renderStats.bStaticGeometryThrashing, pSS );
			OutputStat( "2DTexturesThrashing", renderStats.b2DTexturesThrashing, pSS );
			OutputStat( "TransparentThrashing", renderStats.bTransparentThrashing, pSS );
			OutputStat( "LightmapThrashing", renderStats.bLightmapThrashing, pSS );
			OutputStat( "DynamicGeometryTrashing", renderStats.bDynamicGeometryTrashing, pSS );
			OutputStat( "PointLightShadowThrashing", renderStats.bPointLightShadowThrashing, pSS );
			//
			OutputStat( "Average overdraw", data[eScene]->fAveOverdraw, 0xffff00ff, pSS );
		}
		else
		{
			data[eScene]->timeStatLastShow = GameTimer()->GetAbsTime();
			data[eScene]->nStatNumFrames = 0;
			data[eScene]->nStatNumVerts = 0;
			data[eScene]->nStatNumTris = 0;
			data[eScene]->nStatNumDIPs = 0;
		}
		//

		if ( NGlobal::GetVar( "m1", 0 ) == 0 )
		{
			if ( !data[eScene]->showModes[SCENE_SHOW_NO_FLIP] && !pTarget ) 
				NGScene::Flip();
		}

		//
		static NTimer::STime timeLastDrawPassability = 0;
		if ( DoesTerraManagerExist() )
		{
			if ( ITerraAIObserver *pObserver = GetTerraManager()->GetAIObserver() )
			{
				if ( pObserver->IsPassabilityOn() )
				{
					const NTimer::STime timePassabilityDrawDelay = bEditorMode ? 10000 : 1000;
					if ( GameTimer()->GetAbsTime() > timeLastDrawPassability + timePassabilityDrawDelay )
					{
						if ( bEditorMode )
						{
							data[eScene]->pTerraManager->UpdateAIInfo();
							pObserver->ToggleShowPassability();
							pObserver->ToggleShowPassability();
						}
						pObserver->DrawPassabilities();
						timeLastDrawPassability = GameTimer()->GetAbsTime();
					}
				}
			}
		}

		// Updating weather parameters
		if ( IsValid(data[eScene]->pWeather) )
			data[eScene]->pWeather->Update();
	}
	else
		Sleep( 10 );
	//
	ProcessDebugInfoUpdates();
	//
	StepSceneHoldQueue( data[eScene]->pGameTimer->GetValue() );
}

const EScene CScene::GetCurrentScene() const
{
	return eScene;
}

NGScene::I2DGameView* CScene::GetG2DView()
{ 
	return  data[eScene]->p2DView;
}

NGScene::IGameView *CScene::GetGView()
{
	return data[eScene]->GetGScene();
}

NGScene::IGameView *CScene::GetInterfaceView()
{
	return data[eScene]->pInterfaceView;
}

CCSTime *CScene::GetAbsTimer()
{
	return data[eScene]->pAbsTimer;
}

IFullScreenFader *CScene::GetScreenFader()
{
	return data[eScene]->pScreenFader;
}

CCSTime *CScene::GetGameTimer()
{
	return data[eScene]->pGameTimer;
}

void CScene::ResetTimer( const NTimer::STime &time )
{
	GameTimer()->Reset( time );

	data[eScene]->pAbsTimer->Set( GameTimer()->GetAbsTime() );
	data[eScene]->pGameTimer->Set( GameTimer()->GetGameTime() );

	Singleton<IMusicSystem>()->OnResetTimer();
}

void CScene::AddScreen( struct IWindow *_pScreen )
{
	CObj<IWindow> pScreen( _pScreen );
	RemoveScreen( pScreen );
	data[eScene]->screensData.push_back( SSceneData::SScreenData( pScreen ) );
	SSceneData::SScreenData &screenData = data[eScene]->screensData.back();
	screenData.SetSceneConsts( data[eScene]->pSceneConsts );
}

void CScene::RemoveScreen( struct IWindow *pScreen )
{
	data[eScene]->screensData.remove( SSceneData::SScreenData( pScreen ) );
}

void CScene::RemoveAllScreens()
{
	data[eScene]->screensData.clear();
	Singleton<IDebugSingleton>()->Clear();
}

CVec2 CScene::GetScreenRect()
{
	return NGfx::GetScreenRect();
}

void CScene::OnSerialize( IBinSaver &saver )
{
	SerializeSceneHoldQueue( 127, saver );
}

static void CalcBigRect( CTRect<float> *pRect, float fCameraYaw, float fMapSizeX, float fMapSizeY )
{
	const float fAngle = -ToRadian( fCameraYaw );
	const float fSin = sin( fAngle );
	const float fCos = cos( fAngle );
	//
	CVec2 v1( 0, fMapSizeY );
	CVec2 v2( fMapSizeX, 0 );
	CVec2 v3( fMapSizeX, fMapSizeY );
	//
	v1.Set( v1.x*fCos - v1.y*fSin, v1.x*fSin + v1.y*fCos );
	v2.Set( v2.x*fCos - v2.y*fSin, v2.x*fSin + v2.y*fCos );
	v3.Set( v3.x*fCos - v3.y*fSin, v3.x*fSin + v3.y*fCos );
	CVec2 vMin( 0, 0 );
	vMin.Minimize( v1 );
	vMin.Minimize( v2 );
	vMin.Minimize( v3 );
	CVec2 vMax( 0, 0 );
	vMax.Maximize( v1 );
	vMax.Maximize( v2 );
	vMax.Maximize( v3 );
	//
	pRect->minx = vMin.x;
	pRect->miny = vMin.y;
	pRect->maxx = vMax.x;
	pRect->maxy = vMax.y;
}

static void CalcIterationParams( CVec3 *pvStart, CVec3 *pvStepX, CVec3 *pvStepY, 
													float fCameraYaw, float fMapSizeX, float fMapSizeY, const CTRect<float> &rcRect )
{
	const float fAngle = -ToRadian( fCameraYaw );
	const float fSin = sin( fAngle );
	const float fCos = cos( fAngle );
	const CVec3 vAxisX(	fCos, fSin, 0 );
	const CVec3 vAxisY( -fSin, fCos, 0 );
	pvStart->Set( rcRect.minx*vAxisX.x + rcRect.maxy*vAxisX.y, 
								rcRect.minx*vAxisY.x + rcRect.maxy*vAxisY.y, 0 );
	//
	pvStepX->Set(	vAxisX.x,	vAxisY.x, 0 );
	pvStepY->Set( -vAxisX.y, -vAxisY.y, 0 );
}

class CCameraPlacementGuard
{
	CVec3 vAnchor;
	float fYaw, fPitch, fDist;
	float fLimitsX, fLimitsY;
public:
	CCameraPlacementGuard( float _fLimitsX, float _fLimitsY )
		: fLimitsX( _fLimitsX ), fLimitsY( _fLimitsY )
	{
		Camera()->GetPlacement( &fDist, &fPitch, &fYaw );
		vAnchor = Camera()->GetAnchor();
		Camera()->SetAnchorLimits( CTRect<float>(0, 0, 0, 0) );
	}
	~CCameraPlacementGuard()
	{
		Camera()->SetPlacement( fDist, fPitch, fYaw );
		Camera()->SetAnchor( vAnchor );
		Camera()->SetAnchorLimits( CTRect<float>(0, 0, fLimitsX, fLimitsY) );
	}
};

bool CScene::MakeMapShot( const SGameMessage &msg )
{
	ICamera *pCamera = Camera();
	ITerraManager *pTerraManager = GetTerraManager();
	if ( pTerraManager == 0 ) 
		return false;
	const NDb::STerrain *pDesc = pTerraManager->GetDesc();
	const float fMaxSizeX = pDesc->nNumPatchesX * VIS_TILES_IN_PATCH * VIS_TILE_SIZE;
	const float fMaxSizeY = pDesc->nNumPatchesX * VIS_TILES_IN_PATCH * VIS_TILE_SIZE;
	CCameraPlacementGuard cameraGuard( fMaxSizeX, fMaxSizeY );
	//
//	pCamera->ResetToDefault();
	s_bPerspectiveCamera = false;
	//
	while ( ToggleShow(SCENE_SHOW_UI) != false );
	//
	{
		CArray2D<uint8_t> whiteWarFog;
		whiteWarFog.SetSizes( 1, 1 );
		whiteWarFog[0][0] = 255;
		SetWarFog( whiteWarFog, 1.0f );
		SetWarFog( whiteWarFog, 1.0f );
	}
	Cursor()->Show( false );
	//
	float fDist, fYaw, fPitch;
	pCamera->GetPlacement( &fDist, &fPitch, &fYaw );
	const float fFOV = pCamera->GetFOV();
	CCameraMapIterator itCamera( ToRadian(fFOV), ToRadian(fYaw), ToRadian(fPitch), fDist, 
															 NGfx::GetScreenRect(), fMaxSizeX, fMaxSizeY );
	//
	SYSTEMTIME systime;
	GetLocalTime( &systime );
	const std::string szFileName = StrFmt( "screenshots\\mapshot-%.4d.%.2d.%.2d-%.2d.%.2d.%.2d.tga",
		int(systime.wYear), int(systime.wMonth), int(systime.wDay),
		int(systime.wHour), int(systime.wMinute), int(systime.wSecond) );
	//
	try
	{
		//
		// TRICK: here we will use file mapping to create giant mapshot in order to save memory!
		//
		// prepare TGA file mapping
		// TGA file header
		NImage::STGAFileHeader hdr;
		Zero( hdr );
		hdr.imagespec.wImageWidth = itCamera.GetImageSizeX();
		hdr.imagespec.wImageHeight = itCamera.GetImageSizeY();
		hdr.imagespec.descriptor.cTopToBottomOrder = 1;
		hdr.imagespec.cPixelDepth = 8 * sizeof( SColor24 );
		NImage::__fill_tga_header<SColor24>( &hdr );
		// TGA file footer
		NImage::STGAFileFooter footer;
		Zero( footer );
		memcpy( footer.cSignature, "TRUEVISION-XFILE", 16 );
		footer.cReservedCharacter = '.';
		// file mapping
		CFileStream stream( NVFS::GetMainFileCreator(), szFileName );
		const int nWriteSizeInBytes = int(hdr.imagespec.wImageWidth) * int(hdr.imagespec.wImageHeight) * sizeof(SColor24);
		stream.SetSize( sizeof(hdr) + nWriteSizeInBytes + sizeof(footer) );
		uint8_t *pBuffer = stream.GetBufferForWrite();
		memcpy( pBuffer, &hdr, sizeof(hdr) );
		memcpy( pBuffer + sizeof(hdr) + nWriteSizeInBytes, &footer, sizeof(footer) );
		SColor24 *pImage = (SColor24 *)( pBuffer + sizeof(hdr) );
		const int nLineWidth = hdr.imagespec.wImageWidth;

		for ( ; !itCamera.IsEnd(); itCamera.Next() )
		{
			pCamera->SetAnchor( itCamera.GetAnchor() );

			if ( NGlobal::GetVar( "m1", 0 ) )
				data[eScene]->p2DView->StartNewFrame( 0 );
			//
			Draw( 0 );
			if ( NGlobal::GetVar( "m1", 0 ) )
			{
				data[eScene]->p2DView->Flush();
				NGScene::Flip();
			}

			// make screen shot
			CArray2D<NGfx::SPixel8888> data;
			NGfx::MakeScreenShot( &data, false );
			NImage::CRawColorConvertor<SColor24> convertor( 0, 0, 0 );
			for ( int y = 0; y < data.GetSizeY(); ++y	)
			{
				for ( int x = 0; x < data.GetSizeX(); ++x )
					pImage[(itCamera.GetImagePosY() + y)*nLineWidth + itCamera.GetImagePosX() + x] = convertor( *((uint32_t*)(&data[y][x])) );
			}
		}
//		// save result image
//		SYSTEMTIME systime;
//		GetLocalTime( &systime );
//		const string szFileName = StrFmt( "screenshots\\mapshot-%.4d.%.2d.%.2d-%.2d.%.2d.%.2d.tga", 
//																			int(systime.wYear), int(systime.wMonth), int(systime.wDay),
//																			int(systime.wHour), int(systime.wMinute), int(systime.wSecond) );
//		//
//		CFileStream stream( NVFS::GetMainFileCreator(), szFileName );
//		NImage::SaveAsTGA( image, &stream );
	}
	catch ( ... )
	{
	}
	// restore camera params
	while ( ToggleShow(SCENE_SHOW_UI) == false );
	Cursor()->Show( true );
	s_bPerspectiveCamera = true;
	//
	return true;
}

bool CScene::ToggleAIGeometryMode()
{
	int nMode = data[eScene]->eAIGeomMode;
	if ( nMode == SSceneData::EAI_GEOM_NONE )
		nMode = SSceneData::EAI_GEOM_OVER;
	else
		nMode = SSceneData::EAI_GEOM_NONE;
	data[eScene]->eAIGeomMode = (SSceneData::EAIGeomMode)(nMode);
	data[eScene]->showModes[SCENE_SHOW_AI_GEOM] = (nMode != SSceneData::EAI_GEOM_NONE);
	UpdateAIGeometry();
	return ( nMode == SSceneData::EAI_GEOM_OVER );
}

void CScene::CycleAIGeometryModes()
{
	int nMode = data[eScene]->eAIGeomMode + 1;
	if ( nMode >= SSceneData::EAI_GEOM_LAST	)
		nMode = SSceneData::EAI_GEOM_NONE;
	data[eScene]->eAIGeomMode = (SSceneData::EAIGeomMode)(nMode);
	data[eScene]->showModes[SCENE_SHOW_AI_GEOM] = (nMode != SSceneData::EAI_GEOM_NONE);
	UpdateAIGeometry();
}

void CScene::UpdateAIGeometry()
{
	data[eScene]->vAIMapMeshes.clear();
	SSceneData::EAIGeomMode eMode = data[eScene]->eAIGeomMode;
	if ( eMode == SSceneData::EAI_GEOM_NONE )
		return;

	CPtr<NGScene::IMaterial> pMaterial;
	if ( eMode == SSceneData::EAI_GEOM_OVER )
		pMaterial = data[eScene]->GetGScene()->CreateMaterial( CVec4( 1, 0, 0, 0.5f ) );
	else
		pMaterial = data[eScene]->GetGScene()->CreateMaterial( CVec4( 0.5f, 0.5f, 1, 1 ) );
		
	data[eScene]->pAIMapVisitor->Sync();
	data[eScene]->pAIMap->Sync();
	std::vector<NAI::SObjectInfo> ent;
	data[eScene]->pAIMap->GetEntities( &ent, -1 );
	
	SFBTransform idPos;
	Identity( &idPos.forward );
	Identity( &idPos.backward );
	for( std::vector<NAI::SObjectInfo>::iterator it = ent.begin(); it != ent.end(); ++it )
	{
		const NAI::SObjectInfo &obj = *it;
		CObj<CMemObject> pMemObject = new CMemObject;
		pMemObject->Create( obj.points, obj.tris );
		NGScene::IGameView::SMeshInfo mesh;
		mesh.parts.push_back( NGScene::IGameView::SPartInfo( NGScene::CreateObjectInfo( pMemObject ), pMaterial ) );
		data[eScene]->vAIMapMeshes.push_back( data[eScene]->GetGScene()->CreateMesh( mesh, idPos, 0, 0 ) );
	}
}

void CScene::AddShotTrace( const CVec3 &_vStart, const CVec3 &_vEnd, NTimer::STime _timeStart, const NDb::SWeaponRPGStats::SShell *pShell )
{
	const NDb::SMaterial *pShotTraceMaterial = pShell->pTraceMaterial ? pShell->pTraceMaterial : data[eScene]->pSceneConsts->pShotTraceMaterial;
	if ( pShotTraceMaterial )
	{	
		CShotTraceObj *pTrace = new CShotTraceObj( _vStart, _vEnd, _timeStart, pShell, data[eScene]->pGameTimer );
		//
		NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
		CPtr<CCSBound> pBound = new CCSBound();
		SBound bound;
		bound.BoxInit( _vStart, _vEnd );
		pBound->Set( bound );

		CObjectBase *pObj = data[eScene]->GetGScene()->CreateDynamicMesh( data[eScene]->GetGScene()->MakeMeshInfo(pTrace, pShotTraceMaterial), 0, pBound, NGScene::MakeLargeHintBound(), room );
		SetToSceneHoldQueue( pObj, false );
	}
}

void CScene::SwitchWeather( bool bActive, NTimer::STime timeLength )
{
	if ( data[eScene]->pTerraManager == 0 || data[eScene]->pTerraManager->GetDesc() == 0 )
		return;
	//
	timeBadWeatherLeft = bActive ? timeLength : 0;

	if ( bActive )	// Switch on
	{
		const NDb::SWeatherDesc *pWeatherDesc = data[eScene]->pTerraManager->GetDesc()->weather.pVisuals;
		if ( pWeatherDesc )
		{
			//GetAbsTimer()->Updated();
			if ( !IsValid(data[eScene]->pWeather) )
				data[eScene]->pWeather = new CWeatherVisual( pWeatherDesc, GetGameTimer(), data[eScene]->pScreenFader, data[eScene]->pTerraManager->GetDesc()->pLight, pWeatherDesc->pWeatherLight );
			//data[eScene]->pWeather = new CWeatherVisual( pWeatherDesc, GetAbsTimer() );

			NGScene::SFullRoomInfo room( NGScene::SRoomInfo(NGScene::LF_SKIP_LIGHTING, -100), 0, 0 );
			CPtr<CCSBound> pBound = new CCSBound();
			SBound bound;
			CVec3 vFarEnd( AI_TILES_IN_PATCH * AI_TILE_SIZE, AI_TILES_IN_PATCH * AI_TILE_SIZE, 100 );
			vFarEnd.x *= GetTerraManager()->GetDesc()->nNumPatchesX;
			vFarEnd.y *= GetTerraManager()->GetDesc()->nNumPatchesY;
			bound.BoxInit( VNULL3, vFarEnd );
			pBound->Set( bound );

			data[eScene]->pWeatherSceneObject = data[eScene]->GetGScene()->CreateDynamicMesh( data[eScene]->pWeather->MakeMeshInfo(),
				0, pBound, NGScene::MakeLargeHintBound(), room );
			
			data[eScene]->pWeather->SwitchOn( timeLength );
			if ( pWeatherDesc && pWeatherDesc->pWater )
				data[eScene]->pTerraManager->SetRainyWaters( pWeatherDesc->pWater );
		}
	}
	else	// Switch off
	{
		if ( data[eScene]->pWeather )
		{
			data[eScene]->pWeather->SwitchOff( timeLength );
			//SetToSceneHoldQueue( data[eScene]->pWeatherSceneObject );
		}

		const NDb::SWeatherDesc *pWeatherDesc = data[eScene]->pTerraManager->GetDesc()->weather.pVisuals;
		if ( pWeatherDesc && pWeatherDesc->pWater )
			data[eScene]->pTerraManager->SetRainyWaters( 0 );
	}
}

float CScene::GetZ( float x, float y ) const
{
	return data[eScene]->pTerraManager->GetZ( x, y );
}

uint32_t CScene::GetNormal( const CVec2 &vPoint ) const
{
	return data[eScene]->pTerraManager->GetNormal( vPoint );
}

bool CScene::GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	return data[eScene]->pTerraManager->GetIntersectionWithTerrain( pvResult, vBegin, vEnd );
}

bool CScene::GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	return data[eScene]->pTerraManager->GetIntersectionWithTerrainForEditor( pvResult, vBegin, vEnd );
}

void CScene::InitHeights4Editor( int nSizeX, int nSizeY )
{
	return data[eScene]->pTerraManager->InitHeights4Editor( nSizeY, nSizeY );
}

void CScene::UpdateZ( CVec3 *pvPos )
{
	data[eScene]->pTerraManager->UpdateZ( pvPos );
}

float CScene::GetTileHeight( int nX, int nY ) const
{
	return data[eScene]->pTerraManager->GetTileHeight( nX, nY );
}

CSyncSrc<IVisObj>* CScene::GetSyncSrc() const
{ 
	return data[eScene]->pSyncSrc; 
}

bool CScene::SetGSceneInternal( bool bIsInternal )
{
	return data[eScene]->SetGSceneInternal( bIsInternal );
}


void CScene::SetBackgroundColor( const CVec3 &rvBackgroundColor )
{
	vBackgroundColor = CVec4( rvBackgroundColor, 1.0f );
}

CVec4 CScene::SetBackgroundColor( const CVec4 &rvBackgroundColor )
{
	CVec4 vPrevColor = vBackgroundColor; 
	vBackgroundColor = rvBackgroundColor;
	return vPrevColor;
}

bool CScene::ToggleGetSizeFromTarget( bool _bGetSizesFromTarget )
{
	bool bPrev = bGetSizesFromTarget;
	bGetSizesFromTarget = _bGetSizesFromTarget;
	return bPrev;
}

// special script function to position camera for pwl-mapshot 
static void CmdSetPWLMapshotCamera( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( !(Scene()->DoesTerraManagerExist()) )
		return;
	int nSizeX = 0, nSizeY = 0;
	if ( ITerraManager *pTerraManager = Scene()->GetTerraManager() )
	{
		if ( const NDb::STerrain *pDesc = pTerraManager->GetDesc() )
		{
			nSizeX = pDesc->nNumPatchesX;
			nSizeY = pDesc->nNumPatchesY;
		}
	}
	//
	if ( nSizeX == 0 || nSizeY == 0 )
		return;
	//
	const float fYaw = paramsSet.empty() ? 0 : NStr::ToFloat( NStr::ToMBCS( paramsSet[0] ) );
	const float fPitch = ToDegree( asin( 3.0f/4.0f ) );
	float fOldDist, fOldPitch, fOldYaw;
	Camera()->GetPlacement( &fOldDist, &fOldPitch, &fOldYaw );
	Camera()->SetPlacement( fOldDist, fPitch, fYaw );
}

static void CmdPreparePWL( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	CmdSetPWLMapshotCamera( "pwlmapshot_set_camera", paramsSet, pContext );
	WriteToPipe( PIPE_SCRIPT_CMDS, "RemoveAllUnitsTmp()" );
	NGlobal::ProcessCommand( L"camera_perspective 0" );
}

START_REGISTER( CameraInternalVars )
REGISTER_VAR_EX( "camera_near_plane", NGlobal::VarFloatHandler, &s_fCameraNearPlane, 5.0f, STORAGE_SAVE );
REGISTER_VAR_EX( "camera_far_plane", NGlobal::VarFloatHandler, &s_fCameraFarPlane, 500.0f, STORAGE_SAVE );
REGISTER_VAR_EX( "camera_perspective", NGlobal::VarBoolHandler, &s_bPerspectiveCamera, true, STORAGE_SAVE );
REGISTER_VAR_EX( "min_static_fps", NGlobal::VarFloatHandler, &s_fMinStaticFPS, 25, STORAGE_NONE );
REGISTER_VAR_EX( "min_dynamic_fps", NGlobal::VarFloatHandler, &s_fMinDynamicFPS, 15, STORAGE_NONE );
REGISTER_VAR_EX( "min_dynamic_fps_measure_periods", NGlobal::VarFloatHandler, &s_fMinDynamicFPSMeasurePeriods, 3, STORAGE_NONE );

REGISTER_VAR_EX( "scene_brightness", NGlobal::VarFloatHandler, &NGamma::s_fBrightness, 0.5f, STORAGE_USER );
REGISTER_VAR_EX( "scene_contrast", NGlobal::VarFloatHandler, &NGamma::s_fContrast, 0.5f, STORAGE_USER );
REGISTER_VAR_EX( "scene_gamma", NGlobal::VarFloatHandler, &NGamma::s_fGamma, 0.5f, STORAGE_USER );

REGISTER_CMD( "pwlmapshot_set_camera", CmdSetPWLMapshotCamera );
REGISTER_CMD( "pwl_prepare", CmdPreparePWL );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10073C40, CScene );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x1009DCC1, CSceneSyncSrc );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x110B6B42, CAIMapVisitor );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x1311F302, CSceneIconInfo );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x13121380, CIconSceneSyncSrc );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x13121381, CIconAIMapVisitor );


