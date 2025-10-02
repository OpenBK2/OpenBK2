#include "StdAfx.h"

#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../3DMotor/Gfx.h"
#include "../3DMotor/G2DView.h"
#include "../Main/GameTimer.h"
#include "../UI/UI.h"
#include "../System/BinaryResources.h"
#include "DBSceneConsts.h"
#include "TerrainManager.h"
#include "Camera.h"
#include "../B2_M1_Terrain/TracksManager.h"
#include "SceneHoldQueue.h"
#include "AttachedObj.h"
#include "../Stats_B2_M1/TerraAIObserver.h"

#include "SceneInternal.h"

ITerraManager *CScene::GetTerraManager()
{
	if ( !DoesTerraManagerExist() ) 
	{
		data[eScene]->pTerraManager = new CTerrainManager();
		data[eScene]->pTerraManager->Setup( data[eScene]->GetGScene(), GetGameTimer() );
		//data[eScene]->pTerraManager->Setup( data[eScene]->GetGScene(), GetAbsTimer() );
	}
	return data[eScene]->pTerraManager;
}

void CScene::SetSoundSceneMode( const enum ESoundSceneMode eMode )
{

}

void CScene::AddTrack( const int nID, const float fFadingSpeed,
											 const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4,
											 const CVec2 &vNorm, const float _fWidth, const float fAplha )
{
	if ( data[eScene]->pTracksManager ) 
		data[eScene]->pTerraManager->AddTrack( nID, fFadingSpeed, _v1, _v2, _v3, _v4, vNorm, _fWidth, fAplha, data[eScene]->pTracksManager );
}

void CScene::AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial )
{
	data[eScene]->pTerraManager->AddExplosion( _vMin, _vMax, pMaterial );
}

void CScene::AddDebris( const CVec2 &vSize, const CVec2 &vCenter, float fAngle, float fWidth, const NDb::SMaterial *pMaterial )
{
	data[eScene]->pTerraManager->AddDynamicDebris( vCenter, vSize, fAngle, fWidth, pMaterial );
}

void CScene::SetWarFog( const CArray2D<unsigned char> &fog, float fScale )
{
	data[eScene]->GetGScene()->SetWarFog( fog, fScale );
	data[eScene]->GetGScene()->SetWarFogBlend( 0.0f );
}

void CScene::SetWarFogBlend( const float fBlend )
{
	data[eScene]->GetGScene()->SetWarFogBlend( fBlend );
}

void CScene::SetLight( const NDb::SAmbientLight *pLight )
{
	if ( !pLight )
		return;

	data[eScene]->GetGScene()->SetAmbient( pLight, data[eScene]->pGameTimer );
	data[eScene]->pSceneAmbientLight = pLight;
	if ( data[eScene]->pSceneConsts && data[eScene]->pSceneConsts->pInterfaceLight )
	{
		data[eScene]->pInterfaceView->SetAmbient( data[eScene]->pSceneConsts->pInterfaceLight );
		data[eScene]->pInterfaceAmbientLight = data[eScene]->pSceneConsts->pInterfaceLight;
	}
	else
	{
		data[eScene]->pInterfaceView->SetAmbient( pLight );
		data[eScene]->pInterfaceAmbientLight = pLight;
	}
	for ( SSceneData::CScreensData::iterator iData = data[eScene]->screensData.begin(); iData != data[eScene]->screensData.end(); ++iData )
	{
		SSceneData::SScreenData &screenData = *iData;

		screenData.p3DView->SetAmbient( data[eScene]->pSceneConsts->pInterfaceLight );
	}
}

int CScene::AddPointLight( const int nID, const CVec3 &ptColor, const CVec3 &ptOrigin, float fR )
{
	const int nObjectID = GetID( nID );	
	NI_ASSERT( data[eScene]->visObjects.find(nObjectID) == data[eScene]->visObjects.end(), StrFmt("Object 0x%.8x already exist", nObjectID) );

	if ( CObjectBase *pObj = data[eScene]->GetGScene()->AddPointLight( ptColor, ptOrigin, fR ) )
	{
		data[eScene]->visObjects[nObjectID] = new SStaticVisObjDesc();
		data[eScene]->visObjects[nObjectID]->pObj = pObj;
		data[eScene]->visObjects[nObjectID]->nID = nObjectID;
		/*
		const CVec2 vAngle = CVec2( cos(2.0f * PI / 10),	sin(2.0f * PI / 10) );
		CVec2 vDir = CVec2( 0.2f, 0 );
		vector<CVec3> points;
		points.push_back( ptOrigin + CVec3( vDir, 0.0f ) );

		for ( int i = 1; i < 10; ++i )
		{
		const CVec2 vDir = CVec2( points[i-1].x, points[i-1].y ) - CVec2( ptOrigin.x, ptOrigin.y );
		const CVec2 vRes = vDir ^ vAngle;

		CVec3 vResult( ptOrigin + CVec3(vRes.x, vRes.y, 0.0f ) );
		points.push_back( vResult );
		}
		AddPolyline( -1, points, CVec4( 1, ptColor.x, ptColor.y, ptColor.z ), false );
		*/
		return nObjectID;
	}
	else
		return -1;
}

void CScene::ToggleShowMsg( const SGameMessage &msg, ESceneShow eShow ) 
{ 
	ToggleShow( eShow ); 
}

bool CScene::ToggleShowPassability()
{
	if ( ITerraManager *pTerraManager = GetTerraManager() )
	{
		if ( ITerraAIObserver *pAIObserver = pTerraManager->GetAIObserver() )
		{
			pAIObserver->ToggleShowPassability();

			if ( bEditorMode )
			{
				pAIObserver->FinalizeUpdates();

				pAIObserver->ToggleShowPassability();
				pAIObserver->ToggleShowPassability();
			}
		}
	}
	return true;
}

bool CScene::ProcessEvent( const SGameMessage &msg )
{
	if ( Camera()->ProcessEvent( msg ) )
		return true;
	return observers.ProcessEvent( msg, this );
}

void CScene::AfterLoad()
{
	// check for after-load already done
	if ( (data.size() <= eScene) || (data[eScene]->p2DView != 0) && (data[eScene]->GetGScene() != 0) ) 
		return;

	data[eScene]->pAbsTimer->Set( GameTimer()->GetAbsTime() );
	data[eScene]->pGameTimer->Set( GameTimer()->GetGameTime() );

	CreateGScene();
	
	for ( SSceneData::CVisObjectsMap::iterator it = data[eScene]->visObjects.begin(); it != data[eScene]->visObjects.end(); ++it )
	{
		it->second->ClearObject();
		it->second->ReCreateObject( data[eScene]->GetGScene(), data[eScene]->pSyncSrc, data[eScene]->pGameTimer, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
	}

	for ( SSceneData::CScreensData::iterator iData = data[eScene]->screensData.begin(); iData != data[eScene]->screensData.end(); ++iData )
	{
		SSceneData::SScreenData &screenData = *iData;

		screenData.p3DView = NGScene::CreateNewView();
		screenData.p3DView->WaitForLoad( true );

		screenData.p3DView->SetAmbient( data[eScene]->pSceneConsts->pInterfaceLight );

		screenData.pScreen->AfterLoad();
		for ( SSceneData::CVisObjectsMap::iterator it = screenData.interfaceVisObjects.begin(); it != screenData.interfaceVisObjects.end(); ++it )
		{
			it->second->ClearObject();
			it->second->ReCreateObject( screenData.p3DView, 0, data[eScene]->pAbsTimer, false );
		}
	}

	// Setting Light
	if ( data[eScene]->pSceneAmbientLight )
		SetLight( data[eScene]->pSceneAmbientLight );
	else if ( IsValid( data[eScene]->pTerraManager ) )
		SetLight( data[eScene]->pTerraManager->GetDesc()->pLight );
	else
		SetLight( 0 );

	// holded objects
	if ( eScene == SCENE_MISSION )
	{
		list<CObjectBase*> holdedObjects;
		GetSceneHoldedObjects( &holdedObjects, true );
		for ( list<CObjectBase*>::iterator it = holdedObjects.begin(); it != holdedObjects.end(); ++it )
		{
			if ( SVisObjDescBase *pVOD = dynamic_cast<SVisObjDescBase*>(*it) )
			{
				pVOD->ClearObject();
				pVOD->ReCreateObject( data[eScene]->GetGScene(), data[eScene]->pSyncSrc, data[eScene]->pGameTimer, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
			}
			else if ( IAttachedObject *pAO = dynamic_cast<IAttachedObject*>(*it) )
				pAO->ReCreate( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
		}
	}

	if ( data[eScene]->pTracksManager )
		data[eScene]->pTracksManager->AfterLoad( data[eScene]->pGameTimer, data[eScene]->GetGScene() );

	if ( data[eScene]->pTerraManager && data[eScene]->pTerraManager->GetDesc() ) 
	{
		data[eScene]->pTerraManager->Setup( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
		const string szTerrainFilePath = NDb::GetFolderName( data[eScene]->pTerraManager->GetDesc()->GetDBID() );
		NScene::LoadTerrain( data[eScene]->pTerraManager, data[eScene]->pTerraManager->GetDesc(), szTerrainFilePath );
		data[eScene]->pTerraManager->RestoreFromHistory();
	}

	// after load for icons
	NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
	for ( SSceneData::CSceneIconsMap::iterator it = data[eScene]->iconsMap.begin(); it != data[eScene]->iconsMap.end(); ++it )
	{
		if ( it->second.pPatch )
		{
			it->second.pPatch->AttachSyncSrc( data[eScene]->pIconSyncSrc );
			it->second.pHolder = data[eScene]->GetGScene()->CreateDynamicMesh( data[eScene]->GetGScene()->MakeMeshInfo( it->second.pPatch,
				it->second.pPatch->GetMaterial() ), 0, it->second.pPatch->GetBound(), NGScene::MakeLargeHintBound(), room );
		}
	}

	if ( timeBadWeatherLeft > 0 )
		SwitchWeather( true, timeBadWeatherLeft );
}

void CScene::SwitchSceneAfterLoad( const EScene _eScene )
{
	eScene = _eScene;
}

bool CScene::GetVisObjPlacement( const int nID, SFBTransform *pTransform ) const
{
	SSceneData::CVisObjectsMap::const_iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return false;
	(*pTransform).forward = pos->second->GetPlacement().forward;
	(*pTransform).backward = pos->second->GetPlacement().backward;
	return true;
}

namespace NScene
{
static const DWORD overdrawColors[] = 
{
	0x00000055,
	0x000000AA,
	0x000000FF,
	0x000055FF,
	0x0000AAFF,
	0x0000FFFF,
	0x0000FFAA,
	0x0000FF55,
	0x0000FF00,
	0x0055FF00,
	0x00AAFF00,
	0x00FFFF00,
	0x00FFAA00,
	0x00FF7F00,
	0x00FF5500,
	0x00FF0000,
	0x00FF002A,
	0x00FF0055,
	0x00FF00AA,
	0x00FF00FF
};
};

void CScene::CalcAverageOverdrawMsg( const SGameMessage &msg )
{
	hash_map<DWORD, int> colors;
	for ( int i = 0; i < ARRAY_SIZE(NScene::overdrawColors); ++i )
		colors[ NScene::overdrawColors[i] ] = i + 1;
	// turn UI off, overdraw on
	while ( ToggleShow(SCENE_SHOW_UI) != false );
	while ( ToggleShow(SCENE_SHOW_OVERDRAW) == false );

	// draw and make screenshot
	if ( NGlobal::GetVar( "m1", 0 ) )
		data[eScene]->p2DView->StartNewFrame( 0 );
	Draw( 0 );
	if ( NGlobal::GetVar( "m1", 0 ) )
	{
		data[eScene]->p2DView->Flush();
		NGScene::Flip();
	}

	CArray2D<NGfx::SPixel8888> image;
	NGfx::MakeScreenShot( &image, false );
	// analyze
	int nTotalOverdraw = 0;
	int nNumPixels = 0;
	for ( int y = 0; y < image.GetSizeY(); ++y )
	{
		for ( int x = 0; x < image.GetSizeX(); ++x )
		{
			const DWORD color = image[y][x].dwColor & 0x00ffffff;
			hash_map<DWORD, int>::const_iterator posOverdraw = colors.find( color );
			if ( posOverdraw != colors.end() )
			{
				nTotalOverdraw += posOverdraw->second;
			}
			else
			{
				int nOverdraw = 0;
				float fMaxDiff2 = 1000000;
				float r = ( color >> 16 ) & 0xff;
				float g = ( color >>	8 ) & 0xff;
				float b = (		color		) & 0xff;
				for ( int i = 0; i < ARRAY_SIZE(NScene::overdrawColors); ++i )
				{
					DWORD localColor = NScene::overdrawColors[i];
					float lr = ( localColor >> 16 ) & 0xff;
					float lg = ( localColor >>	8 ) & 0xff;
					float lb = (		localColor		) & 0xff;
					float fDiff2 = fabs2( r - lr, g - lg, b - lb );
					if ( fDiff2 < fMaxDiff2 )
					{
						fMaxDiff2 = fDiff2;
						nOverdraw = i + 1;
					}
				}
				//
				nTotalOverdraw += nOverdraw;
			}
		}
	}
	//
	data[eScene]->fAveOverdraw = float( nTotalOverdraw ) / float( image.GetSizeX() * image.GetSizeY() );
	//
	// turn UI on, overdraw off
	while ( ToggleShow(SCENE_SHOW_UI) == false );
	while ( ToggleShow(SCENE_SHOW_OVERDRAW) != false );
}

