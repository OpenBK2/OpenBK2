#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"

#include "MapEditorLib/EditorFactory.h"
#include "MapEditorLib/Interface_UserData.h"
#include "libdb/ResourceManager.h"

#include "EditorMethods.h"
#include "EditorTestEditor.h"

#include "EditorScene.h"
#include "SceneB2/Camera.h"

#include "EditorOptions.h"
#include "Stats_B2_M1/DBMapInfo.h"

#include <zconf.h>

//REGISTER_EDITOR_IN_DLL( EditorTest, CEditorTestEditor )

CEditorTestEditor::CEditorTestEditor() : pEditorTestState( 0 )
{
}

void CEditorTestEditor::Create()
{
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CEditorTestState::Enter(): pScene == 0" );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	// check, do we have desired MapInfo object
	std::string szMapName = NEditorOptions::GetBgMap( "SEASON_SUMMER" );
	if ( const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( CDBID( szMapName ) ) )
	{
		pScene->SetLight( pMapInfo->pLight );
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			//pTerrain->SetStreamPathes( pUserData->szExportDestinationFolder, pUserData->szExportSourceFolder );
			NEditor::LoadTerrain( pTerraManager, pMapInfo );
			terrainSize.x = pMapInfo->nNumPatchesX;
			terrainSize.y = pMapInfo->nNumPatchesY;
		}
	}
	else
	{
		terrainSize.x = 4;
		terrainSize.y = 4;
		EditorScene()->InitHeights4Editor( terrainSize.x * AI_TILES_IN_PATCH, terrainSize.y * AI_TILES_IN_PATCH );
	}
	// set camera anchor
	CVec3 vCameraAnchor = NEditorOptions::GetBgMapAnchor( "SEASON_SUMMER" );
	if ( vCameraAnchor == VNULL3 ) 
	{
		vCameraAnchor.x = terrainSize.x * 8.0f * AI_TILE_SIZE;
		vCameraAnchor.y = terrainSize.y * 8.0f * AI_TILE_SIZE;
		AI2Vis( &vCameraAnchor );
	}
	Camera()->SetAnchor( vCameraAnchor );
	//
	if ( pEditorTestState == 0 )
	{
		pEditorTestState = new CEditorTestState( this );
	}
}


void CEditorTestEditor::Destroy()
{
	if ( pEditorTestState )
	{
		delete pEditorTestState;
		pEditorTestState = 0;
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
}

// basement storage  


