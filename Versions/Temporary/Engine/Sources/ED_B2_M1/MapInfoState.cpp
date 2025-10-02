#include "StdAfx.h"

#include "../mapeditorlib/resourcedefines.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../mapeditorlib/commoneditormethods.h"
#include "../sceneb2/scene.h"
#include "../mapeditorlib/objectcontroller.h"
#include "simpleobjectinfodata.h"

#include "../MapEditorLib/DefaultTabWindow.h"

#include "MapInfoInterface.h"
#include "MapInfoState.h"

#include "EditorScene.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Interface_Progress.h"
#include "../MapEditorLib/StringManager.h"
#include "../libdb/editorDB.h"

#include "ScriptAreaState.h"
#include "CameraPositionState.h"
#include "UnitStartCmdState.h"
#include "AdvClipboardState.h"
#include "AIGeneralState.h"
#include "ReinfPointsState.h"
#include "ScriptCameraState.h"

#include "MapObjectMultiState.h"

#include "HeightStateV3.h"
#include "FieldState.h"

#include "VSOMultiState.h"

#include "../libdb/ResourceManager.h"

//#include "../MapEditorLib/Interface_ProgressHook.h"
#include "../MapEditor/ProgressHook.h"
#include "../MapEditor/ProgressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const UINT CMapInfoState::INPUT_STATE_LABEL_ID[IS_COUNT] = 
{
	IDS_IS_TERRAIN_LABEL,
	IDS_IS_OBJECT_LABEL,
	IDS_IS_GAMEPLAY_LABEL,
	IDS_IS_SCRIPT_LABEL,
	//IDS_IS_ADVANCED_LABEL,
};


const UINT CMapInfoState::TERRAIN_INPUT_SUSBSTATE_LABEL_ID[TERRAIN_ISS_COUNT] =
{
	IDS_TERRAIN_ISS_HEIGHT_V3_LABEL,
	IDS_TERRAIN_ISS_FIELD_LABEL,
};


const UINT CMapInfoState::OBJECT_INPUT_SUSBSTATE_LABEL_ID[OBJECT_ISS_COUNT] =
{
	IDS_OBJECT_ISS_MAP_OBJECT_LABEL,
	IDS_OBJECT_ISS_VSO_LABEL,
};


const UINT CMapInfoState::GAMEPLAY_INPUT_SUSBSTATE_LABEL_ID[GAMEPLAY_ISS_COUNT] = 
{
	IDS_GP_ISS_REINF_POINTS_LABEL,
	IDS_GP_ISS_START_CAMERA_LABEL,
	IDS_GP_ISS_AIGENERAL_LABEL,
	IDS_GP_ISS_UNIT_START_CMD_LABEL
};


const UINT CMapInfoState::SCRIPT_INPUT_SUSBSTATE_LABEL_ID[SCRIPT_ISS_COUNT] = 
{
	IDS_SCRIPT_ISS_SCRIPT_AREAS_LABEL,
	IDS_SCRIPT_ISS_SCRIPT_MOVIES_LABEL
};


//const UINT CMapInfoState::MOV_EDITOR_INPUT_SUSBSTATE_LABEL_ID[MOV_EDITOR_ISS_COUNT] = 
//{
//	IDS_MOV_EDITOR_ISS_EDITOR_LABEL
//};

/**

const UINT CMapInfoState::ADV_INPUT_SUSBSTATE_LABEL_ID[ADV_ISS_COUNT] =
{
	IDS_ADV_ISS_CLIPBOARD
};
/**/


const UINT CMapInfoState::DEFAULT_INPUT_STATE = IS_OBJECT;


const UINT CMapInfoState::INPUT_SUBSTATE_COUNT[IS_COUNT] = 
{
	TERRAIN_ISS_COUNT,
	OBJECT_ISS_COUNT,
	GAMEPLAY_ISS_COUNT,
	SCRIPT_ISS_COUNT,
	//ADV_ISS_COUNT,
};


const UINT CMapInfoState::DEFAULT_INPUT_SUBSTATE[IS_COUNT] =
{
	TERRAIN_ISS_HEIGHT_V3,
	OBJECT_ISS_MAP_OBJECT,
	GAMEPLAY_ISS_START_CAMERA,
	SCRIPT_ISS_SCRIPT_AREAS,
	//ADV_ISS_CLIPBOARD,
};


CMapInfoState::CMapInfoState(  CMapInfoEditor *_pMapInfoEditor ) : pMapInfoEditor( _pMapInfoEditor )
{
	int nStateIndex = INVALID_INPUT_STATE_INDEX;

	// IS_TERRAIN
	//
	{
		CMultiInputState *pMultiInputState = new CMultiInputState();
		nStateIndex = AddInputState( pMultiInputState );
		NI_ASSERT( nStateIndex == IS_TERRAIN, StrFmt( "CMapInfoState(): Wrong state number IS_TERRAIN: %d (%d)", nStateIndex, IS_TERRAIN ) );
		// TERRAIN_ISS_HEIGHT_V3
		{
			CHeightStateV3 *pHeightStateV3 = new CHeightStateV3( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pHeightStateV3 );
			NI_ASSERT( nStateIndex == TERRAIN_ISS_HEIGHT_V3, StrFmt( "CMapInfoMainState(): Wrong state number TERRAIN_ISS_HEIGHT_V3: %d, (%d)", nStateIndex, TERRAIN_ISS_HEIGHT_V3 ) );
		}
		// TERRAIN_ISS_FIELD
		{
			CFieldState *pFieldState = new CFieldState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pFieldState );
			NI_ASSERT( nStateIndex == TERRAIN_ISS_FIELD, StrFmt( "CMapInfoMainState(): Wrong state number TERRAIN_ISS_FIELD: %d, (%d)", nStateIndex, TERRAIN_ISS_FIELD ) );
		}
	}

	// IS_OBJECT
	//
	{
		CMultiInputState *pMultiInputState = new CMultiInputState();
		nStateIndex = AddInputState( pMultiInputState );
		NI_ASSERT( nStateIndex == IS_OBJECT, StrFmt( "CMapInfoState(): Wrong state number IS_OBJECT: %d (%d)", nStateIndex, IS_OBJECT ) );
		// OBJECT_ISS_MAP_OBJECT
		{
			CMapObjectMultiState *pMapObjectMultiState = new CMapObjectMultiState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pMapObjectMultiState );
			NI_ASSERT( nStateIndex == OBJECT_ISS_MAP_OBJECT, StrFmt( "CMapInfoMainState(): Wrong state number OBJECT_ISS_MAP_OBJECT: %d, (%d)", nStateIndex, OBJECT_ISS_MAP_OBJECT ) );
		}
		// OBJECT_ISS_VSO
		{
			CVSOMultiState *pVSOMultiState = new CVSOMultiState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pVSOMultiState );
			NI_ASSERT( nStateIndex == OBJECT_ISS_VSO, StrFmt( "CMapInfoMainState(): Wrong state number OBJECT_ISS_VSO: %d, (%d)", nStateIndex, OBJECT_ISS_VSO ) );
		}
	}

	// IS_GAMEPLAY
	//
	{
		CMultiInputState *pMultiInputState = new CMultiInputState();
		nStateIndex = AddInputState( pMultiInputState );
		NI_ASSERT( nStateIndex == IS_GAMEPLAY, StrFmt( "CMapInfoState(): Wrong state number IS_GAMEPLAY: %d (%d)", nStateIndex, IS_GAMEPLAY ) );
		// GAMEPLAY_ISS_REINF_POINTS
		{
			CReinfPointsState *pState = new CReinfPointsState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == GAMEPLAY_ISS_REINF_POINTS, StrFmt( "CMapInfoMainState(): Wrong state number GAMEPLAY_ISS_REINF_POINTS: %d, (%d)", nStateIndex, GAMEPLAY_ISS_REINF_POINTS ) );
		}
		// GAMEPLAY_ISS_START_CAMERA
		{
			CCameraPositionState *pState = new CCameraPositionState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == GAMEPLAY_ISS_START_CAMERA, StrFmt( "CMapInfoMainState(): Wrong state number GAMEPLAY_ISS_START_CAMERA: %d, (%d)", nStateIndex, GAMEPLAY_ISS_START_CAMERA ) );
		}
		// GAMEPLAY_ISS_AIGENERAL
		{
			CAIGeneralPointsState *pState = new CAIGeneralPointsState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == GAMEPLAY_ISS_AIGENERAL, StrFmt( "CMapInfoMainState(): Wrong state number GAMEPLAY_ISS_AIGENERAL: %d, (%d)", nStateIndex, GAMEPLAY_ISS_AIGENERAL ) );
		}
		// GAMEPLAY_ISS_UNIT_START_CMD
		{
			CUnitStartCmdState *pState = new CUnitStartCmdState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == GAMEPLAY_ISS_UNIT_START_CMD, StrFmt( "CMapInfoMainState(): Wrong state number GAMEPLAY_ISS_UNIT_START_CMD: %d, (%d)", nStateIndex, GAMEPLAY_ISS_UNIT_START_CMD ) );
		}
	}

	// IS_SCRIPT
	//
	{
		CMultiInputState *pMultiInputState = new CMultiInputState();
		nStateIndex = AddInputState( pMultiInputState );
		NI_ASSERT( nStateIndex == IS_SCRIPT, StrFmt( "CMapInfoState(): Wrong state number IS_SCRIPT: %d (%d)", nStateIndex, IS_SCRIPT ) );
		// SCRIPT_ISS_SCRIPT_AREAS		
		{
			CScriptAreaState *pScriptAreaState = new CScriptAreaState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pScriptAreaState );
			NI_ASSERT( nStateIndex == SCRIPT_ISS_SCRIPT_AREAS, StrFmt( "CMapInfoMainState(): Wrong state number SCRIPT_ISS_SCRIPT_AREAS: %d, (%d)", nStateIndex, SCRIPT_ISS_SCRIPT_AREAS ) );
		}
		// SCRIPT_ISS_SCRIPT_MOVIES
		{
			CScriptCameraState *pState = new CScriptCameraState( pMapInfoEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == SCRIPT_ISS_SCRIPT_MOVIES, StrFmt( "CMapInfoState(): Wrong state number SCRIPT_ISS_SCRIPT_MOVIES: %d, (%d)", nStateIndex, SCRIPT_ISS_SCRIPT_MOVIES ) );
		}
	}
	/**
	// IS_ADVANCED
	//
	{
		{
			CMultiInputState *pMultiInputState = new CMultiInputState();
			nStateIndex = AddInputState( pMultiInputState );
			NI_ASSERT( nStateIndex == IS_ADVANCED, StrFmt( "CMapInfoState(): Wrong state number IS_ADVANCED: %d (%d)", nStateIndex, IS_ADVANCED ) );
			// ADV_ISS_CLIPBOARD
			{
				CAdvClipboardState *pState = new CAdvClipboardState( pMapInfoEditor );
				nStateIndex = pMultiInputState->AddInputState( pState );
				NI_ASSERT( nStateIndex == ADV_ISS_CLIPBOARD, StrFmt( "CMapInfoMainState(): Wrong state number ADV_ISS_CLIPBOARD: %d, (%d)", nStateIndex, ADV_ISS_CLIPBOARD ) );
			}
		}
	}
	/**/
}


bool CMapInfoState::IsMultiInputState( int nStateIndex )
{
	if ( ( nStateIndex == IS_TERRAIN ) ||
			 ( nStateIndex == IS_OBJECT ) ||
			 ( nStateIndex == IS_GAMEPLAY ) ||
			 ( nStateIndex == IS_SCRIPT ) )/** ||
			 ( nStateIndex == IS_ADVANCED ) )/**/
	{
		return true;
	}
	else
	{
		return false;
	}
}


void CMapInfoState::LoadEnterConfig()
{
	if ( pMapInfoEditor )
	{
		for ( int nStateIndex = IS_TERRAIN; nStateIndex < IS_COUNT; ++nStateIndex )
		{
			if ( CMultiInputState *pMultiInputState = dynamic_cast<CMultiInputState*>( GetInputState( nStateIndex ) ) )
			{
				CMapInfoEditorSettings::CActiveStateMap::const_iterator posActiveStateMap = pMapInfoEditor->editorSettings.activeStateMap.find( nStateIndex );
				if ( ( posActiveStateMap != pMapInfoEditor->editorSettings.activeStateMap.end() ) &&
						 ( posActiveStateMap->second >= 0 ) &&
						 ( posActiveStateMap->second < INPUT_SUBSTATE_COUNT[nStateIndex] ) )
				{
					pMultiInputState->SetActiveInputState( posActiveStateMap->second, false, false );
				}
				else
				{
					pMultiInputState->SetActiveInputState( DEFAULT_INPUT_SUBSTATE[nStateIndex], false, false );
				}
			}
		}
		if ( ( pMapInfoEditor->editorSettings.nActiveStateIndex >= 0 ) &&
				 ( pMapInfoEditor->editorSettings.nActiveStateIndex < IS_COUNT ) )
		{
			SetActiveInputState( pMapInfoEditor->editorSettings.nActiveStateIndex, false, false );
		}
		else
		{
			SetActiveInputState( DEFAULT_INPUT_STATE, false, false );
		}
	}
}


void CMapInfoState::SaveEnterConfig()
{
	if ( pMapInfoEditor )
	{
		for ( int nStateIndex = IS_TERRAIN; nStateIndex < IS_COUNT; ++nStateIndex )
		{
			if ( CMultiInputState *pMultiInputState = dynamic_cast<CMultiInputState*>( GetInputState( nStateIndex ) ) )
			{
				pMapInfoEditor->editorSettings.activeStateMap[nStateIndex] = pMultiInputState->GetActiveInputStateIndex();
			}
		}
		pMapInfoEditor->editorSettings.nActiveStateIndex = GetActiveInputStateIndex();
	}
}


void CMapInfoState::Enter2()
{
	int nLakesCount = 0;
	int nCoastsCount = 0;

	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IManipulator *pFolderMan = pResourceManager->CreateFolderManipulator( "MapInfo" );
	IEditorScene *pScene = EditorScene();
	ILogger *pLogger = NLog::GetLogger();

	for ( CPtr<IManipulatorIterator> pIt = pFolderMan->Iterate(true, ECT_CACHE_LOCAL); !pIt->IsEnd(); pIt->Next() )
	{
		if ( !pIt->IsFolder() )
		{
			string szName;
			if ( pIt->GetName(&szName) != false )
			{
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				//
				CDBID dbID( szName );
				//pMapInfoEditor->ClearMapInfoData();
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
				//
				if ( pMapInfoEditor->pMapInfo = NDb::Get<NDb::SMapInfo>(dbID) )
				{
					nLakesCount += pMapInfoEditor->pMapInfo->lakes.size();
					if ( pMapInfoEditor->pMapInfo->coast.points.size() > 1 )
					{
						++nCoastsCount;
					}

					continue;

					if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
					{
						try
						{
							NEditor::LoadTerrain( pTerraManager, pMapInfoEditor->pMapInfo );
							NEditor::SaveTerrain( pTerraManager );
						}
						catch ( ... )
						{
							pLogger->Log( LT_ERROR, StrFmt("Map %s caused error during export\n", szName.c_str()) );
						}
					}
					pLogger->Log( LT_NORMAL, StrFmt("Map %s was exported succesfully\n", szName.c_str()) );
					DebugTrace( "Map %s (%dx%d) : %g", szName.c_str(), pMapInfoEditor->pMapInfo->nNumPatchesX, pMapInfoEditor->pMapInfo->nNumPatchesY, NHPTimer::GetTimePassed(&time) );
				}
			}
		}
	}

	DebugTrace( "%d lakes, %d coasts", nLakesCount, nCoastsCount );
}

#include "EnterNameDialog.h"

void CMapInfoState::Enter()
{
	// TODO: progress bar
	//IProgressHook *pProgress = Singleton<IProgressHook>();
	//if ( pProgress )
	//{
	//	pProgress->Create( "Open MapInfo", Singleton<IMainFrameContainer>()->GetSECWorkbook() );
	//	//
	//	CProgressDlg *pProgressDlg = pProgress->GetProgressDialog();
	//	if ( pProgressDlg )
	//	{
	//		//pProgressDlg->ShowWindow( SW_SHOW );
	//		//pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
	//		//pwndProgressDialog->SetWindowText( _T( "Creating Statistics" ) );
	//	}
	//}
	//pProgress->SetProgressRange( 0, 1000 );

	NProgress::Create( true );
	NProgress::SetRange( 0, 20 );
	NProgress::IteratePosition();	// 1
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	NI_ASSERT( pMapInfoEditor != 0, "CMapInfoState::Enter(), pMapInfoEditor == 0" );
	NI_ASSERT( !( pMapInfoEditor->GetObjectSet().objectNameSet.empty() ), "CMapInfoState::Enter() GetObjectSet().objectNameSet is empty" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CMapInfoState::Enter(): pScene == 0" );
	IManipulator *pMapInfoManipulator = pMapInfoEditor->GetViewManipulator();
	NI_ASSERT( pMapInfoManipulator != 0, "CMapInfoState::Enter(): GetViewManipulator() == 0" );
	// Загружаем Terrain
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	NProgress::IteratePosition(); // 2
	NI_ASSERT( pMapInfoEditor != 0, "CMapInfoState::Enter(), pMapInfoEditor == 0" );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<DWORD>( new CMapInfoInterfaceCommand( new CMapInfoInterface( this ) ) ) );
	//
	pMapInfoEditor->ClearMapInfoData();
	NProgress::IteratePosition(); // 3
	//
	DebugTrace( "CMapInfoState::Enter(): clear data: %g", NHPTimer::GetTimePassed( &time ) );
	//
	DebugTrace( "CMapInfoState::Enter(): set dialogs: %g", NHPTimer::GetTimePassed( &time ) );
	//
	if ( pMapInfoEditor->pMapInfo = NDb::Get<NDb::SMapInfo>( pMapInfoEditor->GetObjectSet().objectNameSet.begin()->first ) )
	{
		NProgress::IteratePosition(); // 4
		DebugTrace( "CMapInfoState::Enter(): load NDb::MapInfo: %g", NHPTimer::GetTimePassed( &time ) );
		//
		//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( pMapInfoEditor->pMapInfo );
		// Заполняем информацию об terrain
		if ( pMapInfoEditor->pMapInfo )
		{
			pScene->SetLight( pMapInfoEditor->pMapInfo->pLight );
			if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
			{
				//SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
				//pTerrain->SetStreamPathes( pUserData->szExportDestinationFolder, pUserData->szExportSourceFolder );
				NEditor::LoadTerrain( pTerraManager, pMapInfoEditor->pMapInfo );
				if ( Camera() && !( pMapInfoEditor->pMapInfo->players.empty() ) )
				{
					Camera()->SetAnchor( pMapInfoEditor->pMapInfo->players[0].camera.vAnchor );
				}
			}
		}
		NProgress::IteratePosition(); // 5
		DebugTrace( "CMapInfoState::Enter(): load terrain: %g", NHPTimer::GetTimePassed( &time ) );
		//
		pMapInfoEditor->heightContainer.SetSize( ( pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH ) + 1, ( pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ) + 1, false );
		//
		SSWTParams swtParams;
		swtParams.dwFlags = SWT_PARAMS;
		swtParams.szParams = StrFmt( "%dx%d", pMapInfoEditor->pMapInfo->nNumPatchesX, pMapInfoEditor->pMapInfo->nNumPatchesY );
		Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
		NProgress::IteratePosition(); // 6
		//
		const int nCragCount = pMapInfoEditor->pMapInfo->crags.size();
		for ( int nCragIndex = 0; nCragIndex < nCragCount; ++nCragIndex )
		{
			const NDb::SVSOInstance &rCrag = pMapInfoEditor->pMapInfo->crags[nCragIndex];
			pMapInfoEditor->heightContainer.InsertVSO( rCrag );
		}
		NProgress::IteratePosition(); // 7
		DebugTrace( "CMapInfoState::Enter(): Fill heightContainer: %g", NHPTimer::GetTimePassed( &time ) );
		//pMapInfoEditor->heightContainer.Trace();
		//
		pMapInfoEditor->VSOCollector.Load( pMapInfoEditor->pMapInfo );
		NProgress::IteratePosition(); // 8
		DebugTrace( "CMapInfoState::Enter(): Fill VSOCollector: %g", NHPTimer::GetTimePassed( &time ) );
		//
		pMapInfoEditor->objectInfoCollector.SetMapInfoEditor( pMapInfoEditor );
		//
		DebugTrace( "CMapInfoState::Enter(): get object types: %g", NHPTimer::GetTimePassed( &time ) );
		//
		//CPtr<IManipulatorIterator> pIterator = pMapInfoManipulator->Iterate( true, ECT_CACHE_GLOBAL );
		//
		DebugTrace( "CMapInfoState::Enter(): get object Iteratior: %g", NHPTimer::GetTimePassed( &time ) );
		//
		const int nObjectCount = pMapInfoEditor->pMapInfo->objects.size();
		const int nSpotCount = pMapInfoEditor->pMapInfo->spots.size();
		//
		// Заполняем массив LinkID объектов
		for ( UINT nObjectIndex = 0; nObjectIndex < nObjectCount; ++nObjectIndex )
		{
			if ( pMapInfoEditor->pMapInfo->objects[nObjectIndex].pObject == 0 )
			{
				NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Map object %d on the map \"%s\" has no RPG stats. Skiping...\n", nObjectIndex, NDb::GetResName(pMapInfoEditor->pMapInfo) ) );
				continue;				
			}
			if ( !pMapInfoEditor->objectInfoCollector.linkIDCollector.LockID( pMapInfoEditor->pMapInfo->objects[nObjectIndex].link.nLinkID ) )
			{
				const int nLinkID = pMapInfoEditor->objectInfoCollector.linkIDCollector.LockID();
				CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController();
				const string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
				if ( pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nLinkID, pMapInfoManipulator ) )
				{
					pObjectController->Redo( false, true, pMapInfoEditor );
				}
			}
			//
			pMapInfoEditor->objectInfoCollector.linkIDToIndexCollector.Insert( pMapInfoEditor->pMapInfo->objects[nObjectIndex].link.nLinkID, nObjectIndex, false );
		}
		//
		NProgress::IteratePosition(); // 9
		DebugTrace( "CMapInfoState::Enter(): get object LinkIDs: %g", NHPTimer::GetTimePassed( &time ) );
		//
		//Споты
		for ( int nSpotIndex = 0; nSpotIndex < nSpotCount; ++nSpotIndex )
		{
			if ( pMapInfoEditor->pMapInfo->spots[nSpotIndex].pDescriptor == 0 )
			{
				NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Spot %d on the map \"%s\" has no RPG stats. Skiping...\n", nSpotIndex, NDb::GetResName(pMapInfoEditor->pMapInfo) ) );
				continue;
			}
			if ( !pMapInfoEditor->objectInfoCollector.linkIDCollector.LockID( pMapInfoEditor->pMapInfo->spots[nSpotIndex].nSpotID ) )
			{
				int nLinkID = pMapInfoEditor->objectInfoCollector.linkIDCollector.LockID();
				CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController();
				const string szObjectPrefix = StrFmt( "Spots.[%d]", nSpotIndex );
				if ( pObjectController->AddChangeOperation( szObjectPrefix + ".SpotID", nLinkID, pMapInfoManipulator ) )
				{
					pObjectController->Redo( false, true, pMapInfoEditor );
				}
			}
			//
			pMapInfoEditor->objectInfoCollector.spotIDToIndexCollector.Insert( pMapInfoEditor->pMapInfo->spots[nSpotIndex].nSpotID, nSpotIndex, false );
		}
		//
		NProgress::IteratePosition(); // 10
		DebugTrace( "CMapInfoState::Enter(): get spot LinkIDs: %g", NHPTimer::GetTimePassed( &time ) );
		//
		for ( UINT nObjectIndex = 0; nObjectIndex < nObjectCount; ++nObjectIndex )
		{
			if ( pMapInfoEditor->pMapInfo->objects[nObjectIndex].pObject == 0 )
			{
				continue;				
			}
			CDBID objectRPGStatsDBID = pMapInfoEditor->pMapInfo->objects[nObjectIndex].pObject->GetDBID();
			string szObjectRPGStatsTypeName = NDb::GetClassTypeName( objectRPGStatsDBID );
			// создаем записи в mapInfo в зависимости от типа объекта
			if ( ( szObjectRPGStatsTypeName == "MineRPGStats" )					||
					 ( szObjectRPGStatsTypeName == "BuildingRPGStats" )			||
					 ( szObjectRPGStatsTypeName == "MechUnitRPGStats" )			||
					 ( szObjectRPGStatsTypeName == "ObjectRPGStats" )				||
					 ( szObjectRPGStatsTypeName == "TerraObjSetRPGStats" )	||
					 ( szObjectRPGStatsTypeName == "FenceRPGStats" )				||
					 ( szObjectRPGStatsTypeName == "SquadRPGStats" ) )
			{
				NMapInfoEditor::SObjectLoadInfo objectLoadInfo;
				objectLoadInfo.nObjectIndex = nObjectIndex;
				objectLoadInfo.nLinkID = pMapInfoEditor->pMapInfo->objects[nObjectIndex].link.nLinkID;
				objectLoadInfo.nLinkWith = pMapInfoEditor->pMapInfo->objects[nObjectIndex].link.nLinkWith;
				objectLoadInfo.bSearchIndices = false;
				objectLoadInfo.bAdditionalDataFilled = true;
				objectLoadInfo.szRPGStatsTypeName = szObjectRPGStatsTypeName;
				objectLoadInfo.rpgStatsDBID = objectRPGStatsDBID;
				objectLoadInfo.nFrameIndex = pMapInfoEditor->pMapInfo->objects[nObjectIndex].nFrameIndex;
				objectLoadInfo.nPlayer = pMapInfoEditor->pMapInfo->objects[nObjectIndex].nPlayer;
				objectLoadInfo.fHP = pMapInfoEditor->pMapInfo->objects[nObjectIndex].fHP;
				objectLoadInfo.vPosition = pMapInfoEditor->pMapInfo->objects[nObjectIndex].vPos;
				objectLoadInfo.fDirection = AI2VisRad( pMapInfoEditor->pMapInfo->objects[nObjectIndex].nDir );
				UINT nSimpleObjectInfoID = INVALID_NODE_ID;
				if ( NMapInfoEditor::SSimpleObjectInfo *pSimpleObjectInfo = pMapInfoEditor->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSimpleObjectInfo*>( 0 ), &nSimpleObjectInfoID ) )
				{
					pSimpleObjectInfo->Load( &objectLoadInfo, pScene, pMapInfoManipulator );
				}
			}
		}
		//
		NProgress::IteratePosition(); // 11
		DebugTrace( "CMapInfoState::Enter(): process objects: %g", NHPTimer::GetTimePassed( &time ) );
		//
		//Споты
		for ( int nSpotIndex = 0; nSpotIndex < nSpotCount; ++nSpotIndex )
		{
			if ( pMapInfoEditor->pMapInfo->spots[nSpotIndex].pDescriptor == 0 )
			{
				continue;
			}
			NMapInfoEditor::SSpotLoadInfo spotLoadInfo;
			spotLoadInfo.nObjectIndex = nSpotIndex;
			spotLoadInfo.nLinkID = pMapInfoEditor->pMapInfo->spots[nSpotIndex].nSpotID;
			spotLoadInfo.bAdditionalDataFilled = true;
			spotLoadInfo.rpgStatsDBID = pMapInfoEditor->pMapInfo->spots[nSpotIndex].pDescriptor->GetDBID();
			spotLoadInfo.szRPGStatsTypeName = NDb::GetClassTypeName( spotLoadInfo.rpgStatsDBID );
			spotLoadInfo.spotSquare = pMapInfoEditor->pMapInfo->spots[nSpotIndex].points;
			spotLoadInfo.bSearchIndices = false;
			UINT nSpotInfoID = INVALID_NODE_ID;
			if ( NMapInfoEditor::SSpotInfo *pSpotInfo = pMapInfoEditor->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSpotInfo*>( 0 ), &nSpotInfoID ) )
			{
				pSpotInfo->Load( &spotLoadInfo, pScene, pMapInfoManipulator );
			}
		}
		//
		NProgress::IteratePosition(); // 12
		DebugTrace( "CMapInfoState::Enter(): process spots: %g", NHPTimer::GetTimePassed( &time ) );
		//
		// Мосты
		int nBidgeCount = 0;
		CManipulatorManager::GetValue( &nBidgeCount, pMapInfoManipulator, "Bridges" );
		for ( UINT nBridgeIndex = 0; nBridgeIndex < nBidgeCount; ++nBridgeIndex )
		{
			NMapInfoEditor::SObjectLoadInfo objectLoadInfo;
			objectLoadInfo.nObjectIndex = nBridgeIndex;
			objectLoadInfo.bSearchIndices = false;
			UINT nBridgeInfoID = INVALID_NODE_ID;
			if ( NMapInfoEditor::SBridgeInfo *pBridgeInfo = pMapInfoEditor->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SBridgeInfo*>( 0 ), &nBridgeInfoID ) )
			{
				pBridgeInfo->Load( &objectLoadInfo, pScene, pMapInfoManipulator );
			}
		}
		//
		NProgress::IteratePosition(); // 13
		DebugTrace( "CMapInfoState::Enter(): process bridges: %g", NHPTimer::GetTimePassed( &time ) );
		//
		// Окопы
		const int nEntrenchmentCount = pMapInfoEditor->pMapInfo->entrenchments.size();
		for ( UINT nEntrenchmentIndex = 0; nEntrenchmentIndex < nEntrenchmentCount; ++nEntrenchmentIndex )
		{
			NMapInfoEditor::SObjectLoadInfo objectLoadInfo;
			objectLoadInfo.nObjectIndex = nEntrenchmentIndex;
			objectLoadInfo.bSearchIndices = false;
			UINT nEntrenchmentInfoID = INVALID_NODE_ID;
			if ( NMapInfoEditor::SEntrenchmentInfo *pEntrenchmentInfo = pMapInfoEditor->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SEntrenchmentInfo*>( 0 ), &nEntrenchmentInfoID ) )
			{
				pEntrenchmentInfo->Load( &objectLoadInfo, pScene, pMapInfoManipulator );
			}
		}
		//
		NProgress::IteratePosition(); // 14
		DebugTrace( "CMapInfoState::Enter(): process entrenches: %g", NHPTimer::GetTimePassed( &time ) );
		//
		pMapInfoEditor->objectInfoCollector.PostLoad( pScene, pMapInfoManipulator );
		//
		NProgress::IteratePosition(); // 15
		const string szDebugParam = Singleton<IUserDataContainer>()->Get()->szDebugParam;
		const bool bShowMiniMap = CStringManager::GetBoolValueFromString( szDebugParam, "ShowMiniMap", 0, ";: ,|\t", true );
		if ( bShowMiniMap )
		{
			pMapInfoEditor->wndMiniMap.LoadMap( pMapInfoEditor->pMapInfo );
			Singleton<ICommandHandlerContainer>()->Set( CHID_MAP_INFO_EDITOR, pMapInfoEditor );
		}
		//
		NProgress::IteratePosition(); // 16
		DebugTrace( "CMapInfoState::Enter(): process minimap: %g", NHPTimer::GetTimePassed( &time ) );
		//
		//Singleton<IAILogic>()->InitAfterMapLoad( pMapInfo );
	}
	// TODO: progress bar
	//pProgress->SetProgressPosition( 100 );

	// загружаем данные в панели
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE,			ID_UPDATE_EDIT_PARAMETERS, MITFEP_ALL );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_SET_EDIT_PARAMETERS, MITHV3EP_ALL );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,		ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_ALL );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_VSO_MULTI_STATE,					ID_UPDATE_EDIT_PARAMETERS, MIVSOSEP_ALL );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_STATE, this );
	// Обновляем сцену
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_RESET_CAMERA, false );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	// Очищаем буфер Undo / Redo
	Singleton<IControllerContainer>()->RemoveTemporaryControllers( CMapInfoController::TEMPORARY_LABEL );
	pMapInfoEditor->SetModified( false );
		NProgress::IteratePosition(); // 17
	// Loading view filter from 
	pMapInfoEditor->ApplyViewFilter();
	// загружаем состояние панелей
	LoadEnterConfig();
	NProgress::IteratePosition(); // 18
	//
	// TODO: progress bar
	//pProgress->SetProgressPosition( 500 );
	CMultiInputState::Enter();
	// TODO: progress bar
	//pProgress->SetProgressPosition( 1000 );
	NProgress::IteratePosition(); // 19
	NProgress::Destroy();
}


void CMapInfoState::Leave()
{
	CMultiInputState::Leave();
	// сохраняем состояние панелей
	SaveEnterConfig();
	//
	NI_ASSERT( pMapInfoEditor != 0, "CMapInfoState::Leave(), pMapInfoEditor == 0" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CMapInfoState::Leave(): pScene == 0" );
	// store last loaded map and camera's anchor
	pMapInfoEditor->editorSettings.nLastLoadedMap = pMapInfoEditor->pMapInfo->GetRecordID();
	pMapInfoEditor->editorSettings.vLastMapCameraAnchor = Camera()->GetAnchor();
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_STATE );
	// Выгружаем MapInfo
	pMapInfoEditor->ClearMapInfoData();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	//
	const string szDebugParam = Singleton<IUserDataContainer>()->Get()->szDebugParam;
	const bool bShowMiniMap = CStringManager::GetBoolValueFromString( szDebugParam, "ShowMiniMap", 0, ";: ,|\t", true );
	if ( bShowMiniMap )
	{
		Singleton<ICommandHandlerContainer>()->Remove( CHID_MAP_INFO_EDITOR );
		pMapInfoEditor->wndMiniMap.LoadMap( 0 );
	}

	// Обновляем сцену
	// Очищаем буфер Undo / Redo
	Singleton<IControllerContainer>()->RemoveTemporaryControllers( CMapInfoController::TEMPORARY_LABEL );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
}


bool CMapInfoState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_MIS_CHANGE_STATE:
		{
			UINT nShortcutIndex = HIWORD( dwData );
			UINT nTabIndex = LOWORD( dwData );
			if ( ( nShortcutIndex != INVALID_SHORTCUT_INDEX ) &&
					 ( nShortcutIndex >= 0 ) &&
					 ( nShortcutIndex < GetCount() ) )
			{
				SetActiveInputState( nShortcutIndex, true, false );
			}
			if ( ( nTabIndex != INVALID_TAB_INDEX ) &&
					 IsMultiInputState( nShortcutIndex ) )
			{
				CMultiInputState *pMultiInputState = checked_cast<CMultiInputState*>( GetInputState( nShortcutIndex ) );
				NI_ASSERT( pMultiInputState != 0, "CMapInfoState::HandleCommand(), pMultiInputState == 0" );
				if ( ( nTabIndex >= 0 ) && 
						 ( nTabIndex < pMultiInputState->GetCount() ) )
				{
					pMultiInputState->SetActiveInputState( nTabIndex, true, false );
				}
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CMapInfoState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapInfoState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapInfoState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_MIS_CHANGE_STATE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


void CMapInfoState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 1 );
	CMultiInputState::OnLButtonDown( nFlags, rMousePoint );
}


void CMapInfoState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 0 );
	CMultiInputState::OnLButtonUp( nFlags, rMousePoint );
}


void CMapInfoState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 1 );
	CMultiInputState::OnRButtonDown( nFlags, rMousePoint );
}


void CMapInfoState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 0 );
	CMultiInputState::OnRButtonUp( nFlags, rMousePoint );
}


void CMapInfoState::OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 1 );
	CMultiInputState::OnMButtonDown( nFlags, rMousePoint );
}


void CMapInfoState::OnMButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_MOUSE_CAPTURE, 0 );
	CMultiInputState::OnMButtonUp( nFlags, rMousePoint );
}

void CMapInfoState::CancelSelection()
{
	
}


// basement storage  

/**
namespace NTest
{
	class CBase
	{
	public:
		int nTest;
		virtual ~CBase();
	};
	//
	class CDerived : public CBase
	{
	public:
		int nDerived;
	};
	static CDerived derived;
	CDerived *GetDerived() { return &derived; }
	CBase *GetBase() { return &derived; }
	int GetDerivedValue( CDerived *pDerived ) { return pDerived->nDerived; }
	void Test()
	{
		{
			CDerived *pBase = GetBase();
			int nValue = GetDerivedValue( pBase );
		}
		{
			CBase *pBase = GetDerived();
			int nValue = GetDerivedValue( pBase );
		}
	}
};
/**/
				/**
				CVec3 vPos = pMapInfo->objects[nObjectIndex].vPos;
				vPos.z += GetTerrainHeight( vPos.x, vPos.y );
				CQuat qRot = CQuat( AI2VisRad( pMapInfo->objects[nObjectIndex].nDir ), V3_AXIS_Z );
				MakeOrientation( &qRot, DWORDToVec3( EditorScene()->GetHeights()->GetNormal( vPos.x, vPos.y ) ) );

				UINT nSceneObjectID = 0;
				// add through MOObj
				// the reason is we need to execute some special functions for artists inside of CMapObj::Create
				if ( szObjectTypeName == "BuildingRPGStats" || szObjectTypeName == "ObjectRPGStats" )
				{
					CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate();
					// fill all because MOObjects don't check sometimes info.bNewFormat flag
					pUpdate->info.fResize = 1.0f;
					pUpdate->info.bNewFormat = true;
					pUpdate->info.vPlacement = vPos;
					pUpdate->info.rotation = qRot;
					pUpdate->info.nTypeID = nObjectRPGStatsTypeID;
					pUpdate->info.nRecordID = nObjectRPGStatsID;
					pUpdate->info.center = CVec2( vPos.x, vPos.y );
					pUpdate->info.z = vPos.z;
					pUpdate->info.dir = pMapInfo->objects[nObjectIndex].nDir;
					pUpdate->info.fHitPoints = pMapInfo->objects[nObjectIndex].fHP * pMapInfo->objects[nObjectIndex].pObject->fMaxHP;
					pUpdate->info.fFuel = 0.0f;
					pUpdate->info.eDipl = EDI_FRIEND;
					pUpdate->info.nPlayer = pMapInfo->objects[nObjectIndex].nPlayer;
					pUpdate->info.nFrameIndex = pMapInfo->objects[nObjectIndex].nFrameIndex;


					CPtr<CMapObj> pObj;
					if ( szObjectTypeName == "BuildingRPGStats" )
						pObj = new CMOBuilding();
					else
						pObj = new CMOObject();

					if ( !pObj->Create( nObjectIndex, pUpdate, pMapInfo->eSeason ) )
						continue;

					// because if nObjectIndex is given to Create, then id of scene object == nObjectIndex
					nSceneObjectID = nObjectIndex;
				}
				else
				{
					const NDb::SModel *pModel = ChooseModelWithHP( pMapInfo->objects[nObjectIndex].pObject, pMapInfo->objects[nObjectIndex].fHP, pMapInfo->eSeason );
					//const NDb::SModel *pModel = GetModel( pMapInfo->objects[nObjectIndex].pObject->pvisualObject, pMapInfo->eSeason ); 
					if ( pModel == 0 )
						continue;
					nSceneObjectID = pScene->AddObject( nObjectIndex, pModel, vPos, qRot, CVec3( 1, 1, 1 ), false );
				}
				//
				pMapInfoEditor->sceneToMap_ObjectIDMap[nSceneObjectID] = nObjectID;
				pMapInfoEditor->mapToScene_ObjectIDMap[nObjectID] = nSceneObjectID;
				/**/
/**
	int nID = 0;
	bool bLockID = true;
	CFreeIDCollector freeIDCollector;	
	DebugTrace( "Empty:" );
	freeIDCollector.Trace();
	//
	nID = 5;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 5;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 6;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 4;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 4;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 8;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	//
	nID = 9;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 1;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 2;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 3;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 20;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 16;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 17;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 19;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
	//
	nID = 18;
	bLockID = freeIDCollector.LockID( nID );
	DebugTrace( "LockID( %d ): %d", nID, bLockID );
	freeIDCollector.Trace();
/**/
			/**
			string szRPGStatsTypeName;
			string szRPGStatsName;
			UINT nObjectRPGStatsTypeID = INVALID_NODE_ID;
			UINT nObjectRPGStatsID = INVALID_NODE_ID;
			CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pMapInfoManipulator, &szRPGStatsTypeName, &szRPGStatsName, &nObjectRPGStatsTypeID, &nObjectRPGStatsID, 0 );
			if ( szRPGStatsTypeName.empty() || szRPGStatsName.empty() )
			{
				NI_ASSERT( 0, StrFmt( "Empty object %d", nObjectIndex ) );
				continue;
			}
			/**/
			/**
			CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pMapInfoManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0, 0, 0 );
			if ( szRPGStatsTypeName.empty() || szRPGStatsName.empty() )
			{
				NI_ASSERT( 0, StrFmt( "Empty object %d", nObjectIndex ) );
				continue;
			}
			/**/
			/**
			else if ( nObjectRPGStatsTypeID == NDb::SEntrenchmentRPGStats::typeID )
			{
			}
			else if ( nObjectRPGStatsTypeID == NDb::SBridgeRPGStats::typeID )
			{
			}
			/**/
