#include "StdAfx.h"

#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"

#include "..\mapeditorlib\commoneditormethods.h"
#include "modelstate.h"
#include "ResourceDefines.h"
#include "CommandHandlerDefines.h"

#include "..\3DLib\MemObject.h"
#include "..\3DLib\GMemBuilder.h"
#include "..\3DLib\Transform.h"
#include "EditorScene.h"
#include "..\SceneB2\Camera.h"
#include "..\Main\GameTimer.h"

#include "..\MapEditorLib\Interface_Logger.h"
#include "../libdb/ResourceManager.h"
#include "..\Misc\PlaneGeometry.h"
#include "..\3DMotor\GAnimation.hpp"

#include "EditorMethods.h"

#include "ModelInterface.h"
#include "ModelState.h"
#include "ModelEditor.h"

#include "EditorOptions.h"
#include "ED_B2_M1Dll.h"
#include "../Stats_B2_M1/AnimModes.h"
#include "../Stats_B2_M1/SceneModes.h"
#include "../Stats_B2_M1/Vis2AI.h"
#include "../Stats_B2_M1/dbvisobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CModelState::CModelState( CModelEditor *_pModelEditor ) : nModelSceneID( INVALID_NODE_ID ), pModelEditor( _pModelEditor ), pMutableModel( 0 ), eModelEditorType( ET_MODEL )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MODEL_STATE, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MODEL_STATE, ID_MODEL_RELOAD_EDITOR, ID_MODEL_SPEED_UP );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CModelState::~CModelState()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MODEL_STATE );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MODEL_STATE );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::Enter()
{
	if ( pModelEditor != 0 )
	{
		// Сначала грузим файл с установками редактора
		{
			SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			if ( pModelEditor->GetObjectSet().szObjectTypeName == "MechUnitRPGStats" )
			{
				// Default setting for stats is not as usual
				pModelEditor->editorSettings.bDrawAnimations = false;
				pModelEditor->editorSettings.bDrawTerrain = false;
				pUserData->SerializeSettings( pModelEditor->editorSettings, "UnitStats", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );

				if ( ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) <= 0 )
				{
					pModelEditor->RestoreCameraParametersFromStats();
				}
				eModelEditorType = ET_RPGSTATS;
				pModelEditor->CreateTarget();
			}
			else
			{
				pUserData->SerializeSettings( pModelEditor->editorSettings, "Model", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
				eModelEditorType = ET_MODEL;
			}
		}
		if ( pModelEditor->pwndTool != 0 )
		{
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pModelEditor->pwndTool, pModelEditor->editorSettings.bShowTool, true );
		}
		if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( pModelEditor->nModelToolbarID ) )
		{
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, pModelEditor->editorSettings.bShowToolbar, true );
		}
		UpdateTerrain();
		UpdateTime( false );
		UpdateSceneColor( false );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_ALL );

		NI_ASSERT( !( pModelEditor->GetObjectSet().objectNameSet.empty() ), "CModelState::Enter() GetObjectSet().objectNameSet is empty" );
		IEditorScene *pScene = EditorScene();
		NI_ASSERT( pScene != 0, "CModelState::Enter(): pScene == 0" );
		//
		ResetCamera( false );
		UpdateModels( true );
		UpdateLight();
		// Обновляем сцену
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<DWORD>( new CModelInterfaceCommand( new CModelInterface() ) ) );
	}
	//
	CDefaultInputState::Enter();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::Leave()
{
	CDefaultInputState::Leave();
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
	if ( pModelEditor != 0 )
	{
		if ( pModelEditor->pwndTool != 0 )
		{
			pModelEditor->editorSettings.bShowTool = pModelEditor->pwndTool->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pModelEditor->pwndTool, false, true );
		}
		if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( pModelEditor->nModelToolbarID ) )
		{
			pModelEditor->editorSettings.bShowToolbar = pToolbar->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
		}
		SaveCamera( false );
		UpdateAIGeometry( true );
		ClearScene( true );
		// Записываем файл с установками (его могли поменять во время работы редактора)
		{
			SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			if ( pModelEditor->GetObjectSet().szObjectTypeName == "MechUnitRPGStats" )
			{
				pUserData->SerializeSettings( pModelEditor->editorSettings, "UnitStats", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
			}
			else
			{
				pUserData->SerializeSettings( pModelEditor->editorSettings, "Model", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
			}
		}
		pModelEditor->DeleteTarget();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::ClearScene( bool bClearAll )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( bClearAll )
		{
			if ( nModelSceneID != INVALID_NODE_ID )
			{
				pScene->RemoveObject( nModelSceneID );
				nModelSceneID = INVALID_NODE_ID;
			}
		}
		for ( list<int>::const_iterator itAnimModelSceneID = animModelSceneIDList.begin(); itAnimModelSceneID != animModelSceneIDList.end(); ++itAnimModelSceneID )
		{
			if ( ( *itAnimModelSceneID ) != INVALID_NODE_ID )
			{
				pScene->RemoveObject( *itAnimModelSceneID );
			}
		}
		animModelSceneIDList.clear();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::UpdateTerrain()
{
	if ( pModelEditor != 0 )
	{
		if ( pModelEditor->editorSettings.bDrawTerrain )
		{
			const int nGridSize = Clamp<int>( pModelEditor->editorSettings.nGridSize, 8, 64 );
			const CVec2 vGridSize = CVec2( nGridSize * VIS_TILE_SIZE, nGridSize * VIS_TILE_SIZE );
			const CVec4 vColorWithAlpha( Clamp( pModelEditor->editorSettings.vTerrainColor.r / 255.0f, 0.0f, 1.0f ),
																	 Clamp( pModelEditor->editorSettings.vTerrainColor.g / 255.0f, 0.0f, 1.0f ),
																	 Clamp( pModelEditor->editorSettings.vTerrainColor.b / 255.0f, 0.0f, 1.0f ),
																	 Clamp( pModelEditor->editorSettings.vTerrainColor.a / 255.0f, 0.0f, 1.0f ) );
			const float fDefaultDiff = pModelEditor->editorSettings.bShowGrid ? pModelEditor->editorSettings.fDefaultDiff : 0.0f;
			pPlane = BuildPlane( pModelEditor->editorSettings.vShift, vGridSize, vColorWithAlpha, nGridSize, nGridSize, fDefaultDiff, pModelEditor->editorSettings.bDoubleSided );
		}
		else
		{
			pPlane = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::UpdateModels( bool bUpdateAll )
{
	if ( pModelEditor != 0 )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			UpdateAIGeometry( true );
			ClearScene( bUpdateAll );
			if ( !pModelEditor->GetObjectSet().objectNameSet.empty() )
			{
				if ( pModelEditor->GetObjectSet().szObjectTypeName == "Effect" )
				{
					if ( const NDb::SEffect *pEffect = NDb::Get<NDb::SEffect>( pModelEditor->GetObjectSet().objectNameSet.begin()->first ) )
					{
						const int nGridSize = Clamp<int>( pModelEditor->editorSettings.nGridSize, 8, 64 );
						const CVec3 vModelPos = CVec3( nGridSize * VIS_TILE_SIZE / 2.0f, nGridSize * VIS_TILE_SIZE / 2.0f, 0.0f ); 
						if ( bUpdateAll )
						{
							CVec3 vPos = vModelPos + pModelEditor->editorSettings.vShift;
							Vis2AIFast( &vPos );
							nModelSceneID = pScene->AddEffect( -1, pEffect, Singleton<IGameTimer>()->GetGameTime(), vPos, CQuat( 0, V3_AXIS_Z ) );
						}
						if ( pModelEditor->editorSettings.bDrawAnimations )
						{
							const int nMaxAnimationsCount = Clamp<int>( pModelEditor->editorSettings.nMaxAnimationsCount, 0, 64 );
							for ( int nAnimationIndex = 0; nAnimationIndex < nMaxAnimationsCount; ++nAnimationIndex )
							{
								const float fAngle = nAnimationIndex * FP_2PI / nMaxAnimationsCount;
								CVec3 vAnimationPos = VNULL3;
								if ( pModelEditor->editorSettings.bAnimationsCircle )
								{
									vAnimationPos = vModelPos + CVec3( pModelEditor->editorSettings.fAnimationsCircleDistance * VIS_TILE_SIZE, 0.0f, 0.0f );
									RotatePoint( &vAnimationPos, fAngle, vModelPos );
								}
								else
								{
									vAnimationPos = vModelPos + CVec3( ( nAnimationIndex + 1 ) * pModelEditor->editorSettings.fAnimationsBetweenDistance * VIS_TILE_SIZE, 0.0f, 0.0f );
								}
								CVec3 vPos = vAnimationPos + pModelEditor->editorSettings.vShift;
								Vis2AIFast( &vPos );
								const int nAnimObjectSceneID = pScene->AddEffect( -1, pEffect, Singleton<IGameTimer>()->GetGameTime(), vPos, CQuat( fAngle, V3_AXIS_Z ) );
								if ( nAnimObjectSceneID != INVALID_NODE_ID )
								{
									animModelSceneIDList.push_back( nAnimObjectSceneID );
								}
							}
						}
					}
				}
				else if ( pModelEditor->GetObjectSet().szObjectTypeName == "ComplexEffect" )
				{
					if ( const NDb::SComplexEffect *pComplexEffect = NDb::Get<NDb::SComplexEffect>( pModelEditor->GetObjectSet().objectNameSet.begin()->first ) )
					{
						const NDb::SEffect *pEffect = 0;
						if ( pComplexEffect->sceneEffects.empty() )
						{
							pEffect = pComplexEffect->pSceneEffect;
						}
						else
						{
							pEffect = pComplexEffect->sceneEffects[0];
						}
						//
						const int nGridSize = Clamp<int>( pModelEditor->editorSettings.nGridSize, 8, 64 );
						const CVec3 vModelPos = CVec3( nGridSize * VIS_TILE_SIZE / 2.0f, nGridSize * VIS_TILE_SIZE / 2.0f, 0.0f ); 
						if ( ( pEffect != 0 ) && bUpdateAll )
						{
							CVec3 vPos = vModelPos + pModelEditor->editorSettings.vShift;
							Vis2AIFast( &vPos );
							nModelSceneID = pScene->AddEffect( -1, pEffect, Singleton<IGameTimer>()->GetGameTime(), vPos, CQuat( 0, V3_AXIS_Z ) );
						}
						if ( pModelEditor->editorSettings.bDrawAnimations )
						{
							const int nMaxAnimationsCount = Clamp<int>( pModelEditor->editorSettings.nMaxAnimationsCount, 0, 64 );
							for ( int nAnimationIndex = 0; nAnimationIndex < nMaxAnimationsCount; ++nAnimationIndex )
							{
								if ( pComplexEffect->sceneEffects.empty() )
								{
									pEffect = pComplexEffect->pSceneEffect;
								}
								else
								{
									pEffect = pComplexEffect->sceneEffects[ ( nAnimationIndex + pModelEditor->editorSettings.bAnimationsCircle ? 0 : 1 ) % pComplexEffect->sceneEffects.size()];
								}
								if ( pEffect != 0 )
								{
									float fAngle = 0.0f;
									CVec3 vAnimationPos = VNULL3;
									if ( pModelEditor->editorSettings.bAnimationsCircle )
									{
										fAngle = nAnimationIndex * FP_2PI / nMaxAnimationsCount;
										vAnimationPos = vModelPos + CVec3( pModelEditor->editorSettings.fAnimationsCircleDistance * VIS_TILE_SIZE, 0.0f, 0.0f );
										RotatePoint( &vAnimationPos, fAngle, vModelPos );
									}
									else
									{
										vAnimationPos = vModelPos + CVec3( ( nAnimationIndex + 1 ) * pModelEditor->editorSettings.fAnimationsBetweenDistance * VIS_TILE_SIZE, 0.0f, 0.0f );
									}
									CVec3 vPos = vAnimationPos + pModelEditor->editorSettings.vShift;
									Vis2AIFast( &vPos );
									const int nAnimObjectSceneID = pScene->AddEffect( -1, pEffect, Singleton<IGameTimer>()->GetGameTime(), vPos, CQuat( fAngle, V3_AXIS_Z ) );
									if ( nAnimObjectSceneID != INVALID_NODE_ID )
									{
										animModelSceneIDList.push_back( nAnimObjectSceneID );
									}
								}
							}
						}
					}
				}
				else
				{
					const NDb::SModel *pModel = 0;
					if ( pModelEditor->GetObjectSet().szObjectTypeName == "Model" )
					{
						pModel = NDb::Get<NDb::SModel>( pModelEditor->GetObjectSet().objectNameSet.begin()->first );
					}
					else if ( pModelEditor->GetObjectSet().szObjectTypeName == "VisObj" )
					{
						if ( const NDb::SVisObj *pVisObj = NDb::Get<NDb::SVisObj>( pModelEditor->GetObjectSet().objectNameSet.begin()->first ) )
						{
							for ( int nModelIndex = 0; nModelIndex < pVisObj->models.size(); ++nModelIndex )
							{
								if ( pVisObj->models[nModelIndex].eSeason == NDb::SEASON_SUMMER )
								{
									pModel = pVisObj->models.begin()->pModel;
									break;
								}
							}
							if ( ( pModel == 0 ) && !pVisObj->models.empty() )
							{
								pModel = pVisObj->models.begin()->pModel;
							}
						}
					}
					else if ( pModelEditor->GetObjectSet().szObjectTypeName == "Geometry" )
					{
						const NDb::SMaterial *pMaterial = NDb::Get<NDb::SMaterial>( pModelEditor->editorSettings.defaultMaterialDBID );
						const NDb::SGeometry *pGeometry = NDb::Get<NDb::SGeometry>( pModelEditor->GetObjectSet().objectNameSet.begin()->first );
						if ( ( pMaterial != 0 ) && ( pGeometry != 0 ) )
						{
							pMutableModel = new NDb::SModel();
							pMutableModel->materials.clear();
							pMutableModel->materials.push_back( pMaterial );
							pMutableModel->pGeometry = pGeometry;
							pMutableModel->pSkeleton = 0;
							pMutableModel->animations.clear();
							pMutableModel->fWindPower = 1.0f;
							pModel = pMutableModel;
						}
					}
					else if ( pModelEditor->GetObjectSet().szObjectTypeName == "Material" )
					{
						const NDb::SMaterial *pMaterial = NDb::Get<NDb::SMaterial>( pModelEditor->GetObjectSet().objectNameSet.begin()->first );
						const NDb::SGeometry *pGeometry = NDb::Get<NDb::SGeometry>( pModelEditor->editorSettings.defaultGeometryDBID );
						if ( ( pMaterial != 0 ) && ( pGeometry != 0 ) )
						{
							pMutableModel = new NDb::SModel();
							pMutableModel->materials.clear();
							pMutableModel->materials.push_back( pMaterial );
							pMutableModel->pGeometry = pGeometry;
							pMutableModel->pSkeleton = 0;
							pMutableModel->animations.clear();
							pMutableModel->fWindPower = 1.0f;
							pModel = pMutableModel;
						}
					}
					else if ( pModelEditor->GetObjectSet().szObjectTypeName == "MechUnitRPGStats" )
					{
						const NDb::SMechUnitRPGStats *pUnitStats = NDb::Get<NDb::SMechUnitRPGStats>( pModelEditor->GetObjectSet().objectNameSet.begin()->first );
						if ( pUnitStats->pvisualObject )
						{
							for ( int iModel = 0; iModel < pUnitStats->pvisualObject->models.size(); ++iModel )
							{
								if ( pUnitStats->pvisualObject->models[iModel].eSeason == NDb::SEASON_SUMMER )
								{
									pModel = pUnitStats->pvisualObject->models[iModel].pModel;
									break;
								}
							}

							if ( !pModel && !pUnitStats->pvisualObject->models.empty() )
								pModel = pUnitStats->pvisualObject->models.begin()->pModel;
						}
					}

					if ( pModel != 0 )
					{
						const int nGridSize = Clamp<int>( pModelEditor->editorSettings.nGridSize, 8, 64 );
						const CVec3 vModelPos = CVec3( nGridSize * VIS_TILE_SIZE / 2.0f, nGridSize * VIS_TILE_SIZE / 2.0f, 0.0f ); 
						if ( bUpdateAll || ( nModelSceneID == INVALID_NODE_ID ) )
						{
							CVec3 vPos = vModelPos + pModelEditor->editorSettings.vShift;
							Vis2AIFast( &vPos );
							nModelSceneID = pScene->AddObject( -1, pModel, vPos, QNULL, CVec3( 1.0f, 1.0f, 1.0f ), OBJ_ANIM_MODE_DEFAULT, 0 );
						}
						//
						if ( ( pModel->pSkeleton != 0 ) && ( pModelEditor->editorSettings.bDrawAnimations ) )
						{
							const int nMaxAnimationsCount = Clamp<int>( pModelEditor->editorSettings.nMaxAnimationsCount, 0, 64 );
							int nAnimationIndex = 0;
							for ( vector<CDBPtr<NDb::SAnimBase> >::const_iterator itAnimation = pModel->pSkeleton->animations.begin(); itAnimation != pModel->pSkeleton->animations.end(); ++itAnimation )
							{
								if ( nAnimationIndex >= nMaxAnimationsCount )
								{
									break;
								}
								CVec3 vAnimationPos = VNULL3;
								if ( pModelEditor->editorSettings.bAnimationsCircle )
								{
									vAnimationPos = vModelPos + CVec3( pModelEditor->editorSettings.fAnimationsCircleDistance * VIS_TILE_SIZE, 0.0f, 0.0f );
									RotatePoint( &vAnimationPos, nAnimationIndex * FP_2PI / nMaxAnimationsCount, vModelPos );
								}
								else
								{
									vAnimationPos = vModelPos + CVec3( ( nAnimationIndex + 1 ) * pModelEditor->editorSettings.fAnimationsBetweenDistance * VIS_TILE_SIZE, 0.0f, 0.0f );
								}
								CVec3 vPos = vAnimationPos + pModelEditor->editorSettings.vShift;
								Vis2AIFast( &vPos );
								const int nAnimObjectSceneID = pScene->AddObject( -1, pModel, vPos, QNULL, CVec3( 1.0f, 1.0f, 1.0f ), OBJ_ANIM_MODE_DEFAULT, 0 );
								if ( nAnimObjectSceneID != INVALID_NODE_ID )
								{
									const int nAnimationIndexInFile = 0;
									if ( *itAnimation && pScene->GetAnimator( nAnimObjectSceneID ) )
									{
										pScene->GetAnimator( nAnimObjectSceneID )->AddAnimation( 0, NAnimation::SAnimHandle( *itAnimation, nAnimationIndexInFile ), true );
										animModelSceneIDList.push_back( nAnimObjectSceneID );
									}
									else
									{
										pScene->RemoveObject( nAnimObjectSceneID );
									}
								}
								++nAnimationIndex;
							}
						}
					}
				}
			}
			UpdateAIGeometry( false );
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::UpdateLight()
{
	if ( pModelEditor != 0 )
	{
		const NDb::SAmbientLight *pLight = 0;
		if ( !pModelEditor->editorSettings.lightDBID.IsEmpty() )
		{
			pLight = NDb::Get<NDb::SAmbientLight>( pModelEditor->editorSettings.lightDBID );
		}
		if ( pLight == 0 )
		{
			if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( "AmbientLight" ) )
			{
				string szLight = NEditorOptions::GetMiscString( "Light" );
				if ( szLight.empty() ) 
				{
					szLight = NEditorOptions::GetLight( "SEASON_SUMMER", "DAY_DAY" );
					NLog::GetLogger()->Log( LT_IMPORTANT, "Can't get light from options - trying summer daylight\n" );
				}
				pLight = NDb::Get<NDb::SAmbientLight>( CDBID( szLight ) );
			}
		}
		if ( pLight == 0 )
		{
			NLog::GetLogger()->Log( LT_ERROR, "Can't find any light in options - possible bugs\n" );
		}
		else
		{
			EditorScene()->SetLight( pLight );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::UpdateTime( bool bReset )
{
	if ( pModelEditor != 0 )
	{
		if ( bReset )
		{
			Singleton<IGameTimer>()->SetSpeed( 0 );
		}
		else
		{
			int nGameTimerSpeed = Clamp<int>( pModelEditor->editorSettings.nGameTimerSpeed, -10, +10 );
			Singleton<IGameTimer>()->SetSpeed( nGameTimerSpeed );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::UpdateSceneColor( bool bReset )
{
	if ( pModelEditor != 0 )
	{
		if ( bReset )
		{
			EditorScene()->SetBackgroundColor( VNULL3 );
		}
		else
		{
			const CVec3 vBackgroundColor( Clamp( pModelEditor->editorSettings.vSceneColor.r / 255.0f, 0.0f, 1.0f ),
																		Clamp( pModelEditor->editorSettings.vSceneColor.g / 255.0f, 0.0f, 1.0f ),
																		Clamp( pModelEditor->editorSettings.vSceneColor.b / 255.0f, 0.0f, 1.0f ) );
			EditorScene()->SetBackgroundColor( vBackgroundColor );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::UpdateAIGeometry( bool bReset )
{
	if ( pModelEditor != 0 )
	{
		if ( bReset )
		{
			if ( pModelEditor->editorSettings.bDrawAIGeometry )
			{
				while( EditorScene()->ToggleShow( SCENE_SHOW_AI_GEOM ) != false );
			}
		}
		else
		{
			if ( pModelEditor->editorSettings.bDrawAIGeometry )
			{
				while( EditorScene()->ToggleAIGeometryMode() != pModelEditor->editorSettings.bShowSolidAIGeometry );
			}
			while( EditorScene()->ToggleShow( SCENE_SHOW_AI_GEOM ) != pModelEditor->editorSettings.bDrawAIGeometry );
		}
	}
}

/**
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::SetTerrain()
{
	if ( pModelEditor != 0 )
	{
		int nObjectID = INVALID_NODE_ID;
		string szObjectTypeName = "MapInfo";
		if ( Singleton<IMainFrameContainer>()->Get()->BrowseForObject( &nObjectID, &szObjectTypeName, false, true ) )
		{
			pModelEditor->editorSettings.nMapInfoID = nObjectID;
			UpdateTerrain();
		}
	}
}
/**/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::SetLight()
{
	if ( pModelEditor != 0 )
	{
		string szObjectTypeName = "AmbientLight";
		if ( Singleton<IMainFrameContainer>()->Get()->BrowseForObject( &( pModelEditor->editorSettings.lightDBID ), &szObjectTypeName, false, true ) )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_LIGHT_INDEX );
			UpdateLight();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::CenterCamera()
{
	if ( pModelEditor != 0 )
	{
		const int nGridSize = Clamp<int>( pModelEditor->editorSettings.nGridSize, 8, 64 );
		const CVec3 vModelPos = CVec3( nGridSize * VIS_TILE_SIZE / 2.0f, nGridSize * VIS_TILE_SIZE / 2.0f, 0.0f ); 
		Singleton<ICamera>()->SetAnchor( vModelPos + pModelEditor->editorSettings.vShift );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::SaveCamera( bool bDefaultCamera )
{
	if ( pModelEditor != 0 )
	{
		CModelEditorSettings::SCameraPlacement *pCameraPlacement = 0;
		if ( bDefaultCamera )
		{
			CString strMessage;
			AfxSetResourceHandle( theEDB2M1Instance );
			strMessage.LoadString( IDS_MODEL_SAVE_CAMERA_MESSAGE );
			AfxSetResourceHandle( AfxGetInstanceHandle() );
			if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
			{
				pCameraPlacement = &( pModelEditor->editorSettings.defaultCamera );
			}
		}
		else
		{
			pCameraPlacement = &( pModelEditor->editorSettings.currentCamera );
		}
		if ( pCameraPlacement != 0 )
		{
			pCameraPlacement->vAnchor = Singleton<ICamera>()->GetAnchor();
			Singleton<ICamera>()->GetPlacement( &( pCameraPlacement->fDistance ),
																					&( pCameraPlacement->fPitch ),
																					&( pCameraPlacement->fYaw ) );
			pCameraPlacement->fFOV = Singleton<ICamera>()->GetFOV();
			pCameraPlacement->SetValid( true );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::SaveCameraForRPGStats()
{
	if ( !pModelEditor )
	{
		return;
	}

	pModelEditor->CreateIcon();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::ResetCamera( bool bDefaultCamera )
{
	if ( pModelEditor != 0 )
	{
		CModelEditorSettings::SCameraPlacement *pCameraPlacement = 0;
		if ( bDefaultCamera )
		{
			pCameraPlacement = &( pModelEditor->editorSettings.defaultCamera );
		}
		else
		{
			pCameraPlacement = &( pModelEditor->editorSettings.currentCamera );
		}
		//
		if ( !pCameraPlacement->IsValid() )
		{
			const int nGridSize = Clamp<int>( pModelEditor->editorSettings.nGridSize, 8, 64 );
			const CVec3 vModelPos = CVec3( nGridSize * VIS_TILE_SIZE / 2.0f, nGridSize * VIS_TILE_SIZE / 2.0f, 0.0f ); 
			pCameraPlacement->vAnchor = vModelPos + pModelEditor->editorSettings.vShift;
			pCameraPlacement->fDistance = VIS_TILE_SIZE * 16.0f;
			pCameraPlacement->fPitch = 45.0f;
			pCameraPlacement->fYaw = 45.0f;
			pCameraPlacement->fFOV = 26.0f;
		}
		Singleton<ICamera>()->SetAnchor( pCameraPlacement->vAnchor );
		Singleton<ICamera>()->SetPlacement( pCameraPlacement->fDistance,
																				pCameraPlacement->fPitch,
																				pCameraPlacement->fYaw );
		Singleton<ICamera>()->SetFOV( pCameraPlacement->fFOV );
		//EditorScene()->SetFOV( pCameraPlacement->fFOV );
		SaveCamera( false );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::OnContextMenu( const CTPoint<int> &rMousePoint )
{
	if ( pModelEditor != 0 )
	{
		CMenu mainPopupMenu;
		AfxSetResourceHandle( theEDB2M1Instance );
		mainPopupMenu.LoadMenu( IDM_MODEL_CONTEXT_MENU );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		CMenu *pMenu = mainPopupMenu.GetSubMenu( MCM_STATE );
		if ( pMenu )
		{
			pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, rMousePoint.x, rMousePoint.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
		mainPopupMenu.DestroyMenu();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::GetEditParameters( UINT nFlags ) // editParameters -> editorSettings
{
	if ( pModelEditor == 0 )
	{
		return;
	}
	bool bCenterCamera = false;
	bool bUpdateLight = false;
	bool bUpdateTerrain = false;
	bool bUpdateModels = false;
	bool bUpdateAllModels = false;
	bool bUpdateTime = false;
	bool bUpdateSceneColor = false;
	bool bUpdateAIGeometry = false;
	bool bUpdateScene = false;
	bool bUpdateCurrentCamera = false;
	editParameters.nFlags = nFlags;
	if ( editParameters.nFlags & MODEL_EP_LIGHT_COUNT )
	{
		// no code here
	}
	if ( editParameters.nFlags & MODEL_EP_LIGHT_INDEX )
	{
		if ( ( editParameters.nLightIndex >= 0 ) && ( editParameters.nLightIndex < editParameters.lightList.size() ) )
		{
			const CDBID lightDBID = CDBID( editParameters.lightList[editParameters.nLightIndex] );
			if ( pModelEditor->editorSettings.lightDBID != lightDBID )
			{
				pModelEditor->editorSettings.lightDBID = lightDBID;
				bUpdateLight = true;
				bUpdateScene = true;
				//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_LIGHT_INDEX ): success");
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_COLOR )
	{
		if ( pModelEditor->editorSettings.vSceneColor != editParameters.vColor )
		{
			pModelEditor->editorSettings.vSceneColor = editParameters.vColor;
			bUpdateSceneColor = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_COLOR ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_FOV )
	{
		if ( (int)( pModelEditor->editorSettings.currentCamera.fFOV ) != editParameters.nFOV )
		{
			SaveCamera( false );
			pModelEditor->editorSettings.currentCamera.fFOV = editParameters.nFOV;
			bUpdateCurrentCamera = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_FOV ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN )
	{
		if ( pModelEditor->editorSettings.bDrawTerrain != editParameters.bTerrain )
		{
			pModelEditor->editorSettings.bDrawTerrain = editParameters.bTerrain;
			bUpdateTerrain = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_TERRAIN ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_SIZE_COUNT )
	{
		// no code here
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_SIZE_INDEX )
	{
		if ( ( editParameters.nTerrainSizeIndex >= 0 ) && ( editParameters.nTerrainSizeIndex < editParameters.terrainSizeList.size() ) )
		{
			if ( pModelEditor->editorSettings.nGridSize != ( editParameters.nTerrainSizeIndex + 8 ) )
			{
				pModelEditor->editorSettings.nGridSize = editParameters.nTerrainSizeIndex + 8;
				bUpdateTerrain = true;
				bUpdateModels = true;
				bUpdateAllModels = true;
				bUpdateScene = true;
				bCenterCamera = true;
				//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_TERRAIN_SIZE_INDEX ): success");
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_COLOR )
	{
		if ( ( pModelEditor->editorSettings.vTerrainColor.r != editParameters.vTerrainColor.r ) ||
				 ( pModelEditor->editorSettings.vTerrainColor.g != editParameters.vTerrainColor.g ) ||
				 ( pModelEditor->editorSettings.vTerrainColor.b != editParameters.vTerrainColor.b ) )
		{
			pModelEditor->editorSettings.vTerrainColor.r = editParameters.vTerrainColor.r;
			pModelEditor->editorSettings.vTerrainColor.g = editParameters.vTerrainColor.g;
			pModelEditor->editorSettings.vTerrainColor.b = editParameters.vTerrainColor.b;
			bUpdateTerrain = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_TERRAIN_COLOR ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_COLOR_OPACITY )
	{
		if ( (int)( pModelEditor->editorSettings.vTerrainColor.a ) != editParameters.nTerrainColorOpacity )
		{
			pModelEditor->editorSettings.vTerrainColor.a = editParameters.nTerrainColorOpacity;
			bUpdateTerrain = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_TERRAIN_COLOR_OPACITY ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_DOUBLESIDED )
	{
		if ( pModelEditor->editorSettings.bDoubleSided != editParameters.bTerrainDoubleSided )
		{
			pModelEditor->editorSettings.bDoubleSided = editParameters.bTerrainDoubleSided;
			bUpdateTerrain = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_TERRAIN_DOUBLESIDED ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_GRID )
	{
		if ( pModelEditor->editorSettings.bShowGrid != editParameters.bTerrainGrid )
		{
			pModelEditor->editorSettings.bShowGrid = editParameters.bTerrainGrid;
			bUpdateTerrain = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_TERRAIN_GRID ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM )
	{
		if ( pModelEditor->editorSettings.bDrawAnimations != editParameters.bAnim )
		{
			pModelEditor->editorSettings.bDrawAnimations = editParameters.bAnim;
			bUpdateModels = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_ANIM ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_COUNT_COUNT )
	{
		// no code here
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_COUNT_INDEX )
	{
		if ( ( editParameters.nAnimCountIndex >= 0 ) && ( editParameters.nAnimCountIndex < editParameters.animCountList.size() ) )
		{
			if ( pModelEditor->editorSettings.nMaxAnimationsCount != editParameters.nAnimCountIndex )
			{
				pModelEditor->editorSettings.nMaxAnimationsCount = editParameters.nAnimCountIndex;
				bUpdateModels = true;
				bUpdateScene = true;
				//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_ANIM_COUNT_INDEX ): success");
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_SPEED_COUNT )
	{
		// no code here
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_SPEED_INDEX )
	{
		if ( ( editParameters.nAnimSpeedIndex >= 0 ) && ( editParameters.nAnimSpeedIndex < editParameters.animSpeedList.size() ) )
		{
			if ( pModelEditor->editorSettings.nGameTimerSpeed != ( editParameters.nAnimSpeedIndex - 10 ) )
			{
				pModelEditor->editorSettings.nGameTimerSpeed = editParameters.nAnimSpeedIndex - 10;
				bUpdateTime = true;
				bUpdateScene = true;
				//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_ANIM_SPEED_INDEX ): success");
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_TYPE )
	{
		if ( pModelEditor->editorSettings.bAnimationsCircle != ( editParameters.eAnimType == SEditParameters::AT_CIRCLE ) )
		{
			pModelEditor->editorSettings.bAnimationsCircle = ( editParameters.eAnimType == SEditParameters::AT_CIRCLE );
			bUpdateModels = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_ANIM_TYPE ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_RADIUS_COUNT )
	{
		// no code here
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_RADIUS_INDEX )
	{
		if ( ( editParameters.nAnimRadiusIndex >= 0 ) && ( editParameters.nAnimRadiusIndex < editParameters.animRadiusList.size() ) )
		{
			if ( (int)( pModelEditor->editorSettings.fAnimationsCircleDistance ) != ( editParameters.nAnimRadiusIndex + 2 ) )
			{
				pModelEditor->editorSettings.fAnimationsCircleDistance = editParameters.nAnimRadiusIndex + 2;
				bUpdateModels = true;
				bUpdateScene = true;
				//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_ANIM_RADIUS_INDEX ): success");
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_DISTANCE_COUNT )
	{
		// no code here
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_DISTANCE_INDEX )
	{
		if ( ( editParameters.nAnimDistanceIndex >= 0 ) && ( editParameters.nAnimDistanceIndex < editParameters.animDistanceList.size() ) )
		{
			if ( (int)( pModelEditor->editorSettings.fAnimationsBetweenDistance ) != ( editParameters.nAnimDistanceIndex + 2 ) )
			{
				pModelEditor->editorSettings.fAnimationsBetweenDistance = editParameters.nAnimDistanceIndex + 2;
				bUpdateModels = true;
				bUpdateScene = true;
				//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_ANIM_DISTANCE_INDEX ): success");
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_AI_GEOMETRY )
	{
		if ( pModelEditor->editorSettings.bDrawAIGeometry != editParameters.bAIGeometry )
		{
			pModelEditor->editorSettings.bDrawAIGeometry = editParameters.bAIGeometry;
			bUpdateAIGeometry = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_AI_GEOMETRY ): success");
		}
	}
	if ( editParameters.nFlags & MODEL_EP_AI_GEOMETRY_TYPE )
	{
		if ( pModelEditor->editorSettings.bShowSolidAIGeometry != ( editParameters.eAIGeometryType == SEditParameters::AIGT_SOLID ) )
		{
			pModelEditor->editorSettings.bShowSolidAIGeometry = ( editParameters.eAIGeometryType == SEditParameters::AIGT_SOLID );
			bUpdateAIGeometry = true;
			bUpdateScene = true;
			//DebugTrace( "CModelState::GetEditParameters( MODEL_EP_AI_GEOMETRY_TYPE ): success");
		}
	}
	if ( bCenterCamera )
	{
		CenterCamera();
		//DebugTrace( "CModelState::GetEditParameters(): CenterCamera()");
	}
	if ( bUpdateLight )
	{
		UpdateLight();
		//DebugTrace( "CModelState::GetEditParameters(): UpdateLight()");
	}
	if ( bUpdateTerrain )
	{
		UpdateTerrain();
		//DebugTrace( "CModelState::GetEditParameters(): UpdateTerrain()");
	}
	if ( bUpdateModels )
	{
		UpdateModels( bUpdateAllModels );
		//DebugTrace( "CModelState::GetEditParameters(): UpdateModels()");
	}
	if ( bUpdateTime )
	{
		UpdateTime( false );
		//DebugTrace( "CModelState::GetEditParameters(): UpdateTime()");
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
	}
	if ( bUpdateSceneColor )
	{
		UpdateSceneColor( false );
		//DebugTrace( "CModelState::GetEditParameters(): UpdateSceneColor()");
	}
	if ( bUpdateAIGeometry )
	{
		UpdateAIGeometry( false );
		//DebugTrace( "CModelState::GetEditParameters(): UpdateAIGeometry()");
	}
	if ( bUpdateCurrentCamera )
	{
		ResetCamera( false );
		//DebugTrace( "CModelState::GetEditParameters(): ResetCamera()");
	}
	if ( bUpdateScene )
	{
		//DebugTrace( "CModelState::GetEditParameters(): UpdateScene()");
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelState::SetEditParameters( UINT nFlags ) // editorSettings -> editParameters
{
	if ( pModelEditor == 0 )
	{
		return;
	}
	editParameters.nFlags = nFlags;
	if ( editParameters.nFlags & MODEL_EP_LIGHT_COUNT )
	{
		editParameters.lightList.clear();
		if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( "AmbientLight" ) )
		{
			if ( CPtr<IManipulatorIterator> pFolderIterator = pFolderManipulator->Iterate( true, ECT_CACHE_GLOBAL ) )
			{
				while ( !pFolderIterator->IsEnd() )
				{
					if ( !pFolderIterator->IsFolder() )
					{
						string szName;
						if ( pFolderIterator->GetName( &szName ) )
						{
							editParameters.lightList.push_back( szName );
						}
					}
					pFolderIterator->Next();
				}
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_LIGHT_INDEX )
	{
		const string szName = pModelEditor->editorSettings.lightDBID.ToString();
		if ( !szName.empty() )
		{
			const int nLightCount = editParameters.lightList.size();
			for ( int nLightIndex = 0; nLightIndex < nLightCount; ++nLightIndex )
			{
				if ( editParameters.lightList[nLightIndex] == szName )
				{
					editParameters.nLightIndex = nLightIndex;
					break;
				}
			}
		}
	}
	if ( editParameters.nFlags & MODEL_EP_COLOR )
	{
		editParameters.vColor = pModelEditor->editorSettings.vSceneColor;
	}
	if ( editParameters.nFlags & MODEL_EP_FOV )
	{
		editParameters.nFOV = pModelEditor->editorSettings.currentCamera.fFOV;
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN )
	{
		editParameters.bTerrain = pModelEditor->editorSettings.bDrawTerrain;
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_SIZE_COUNT )
	{
		editParameters.terrainSizeList.clear();
		for ( int nTerrainSizeIndex = 8; nTerrainSizeIndex <= 64; ++nTerrainSizeIndex )
		{
			editParameters.terrainSizeList.push_back( StrFmt( "%d", nTerrainSizeIndex ) );
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_SIZE_INDEX )
	{
		editParameters.nTerrainSizeIndex = pModelEditor->editorSettings.nGridSize - 8;
		if ( editParameters.nTerrainSizeIndex < 0 )
		{
			editParameters.nTerrainSizeIndex = 0;
		}
		if (	editParameters.nTerrainSizeIndex >= editParameters.terrainSizeList.size() )
		{
			editParameters.nTerrainSizeIndex = editParameters.terrainSizeList.size() - 1;
		}
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_COLOR )
	{
		editParameters.vTerrainColor.r = pModelEditor->editorSettings.vTerrainColor.r;
		editParameters.vTerrainColor.g = pModelEditor->editorSettings.vTerrainColor.g;
		editParameters.vTerrainColor.b = pModelEditor->editorSettings.vTerrainColor.b;
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_COLOR_OPACITY )
	{
		editParameters.nTerrainColorOpacity = pModelEditor->editorSettings.vTerrainColor.a;
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_DOUBLESIDED )
	{
		editParameters.bTerrainDoubleSided = pModelEditor->editorSettings.bDoubleSided;
	}
	if ( editParameters.nFlags & MODEL_EP_TERRAIN_GRID )
	{
		editParameters.bTerrainGrid = pModelEditor->editorSettings.bShowGrid;
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM )
	{
		editParameters.bAnim = pModelEditor->editorSettings.bDrawAnimations;
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_COUNT_COUNT )
	{
		editParameters.animCountList.clear();
		for ( int nAnimCountIndex = 0; nAnimCountIndex <= 64; ++nAnimCountIndex )
		{
			editParameters.animCountList.push_back( StrFmt( "%d", nAnimCountIndex ) );
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_COUNT_INDEX )
	{
		editParameters.nAnimCountIndex = pModelEditor->editorSettings.nMaxAnimationsCount;
		if ( editParameters.nAnimCountIndex < 0 )
		{
			editParameters.nAnimCountIndex = 0;
		}
		if (	editParameters.nAnimCountIndex >= editParameters.animCountList.size() )
		{
			editParameters.nAnimCountIndex = editParameters.animCountList.size() - 1;
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_SPEED_COUNT )
	{
		editParameters.animSpeedList.clear();
		for ( int nAnimSpeedIndex = -10; nAnimSpeedIndex <= 10; ++nAnimSpeedIndex )
		{
			editParameters.animSpeedList.push_back( StrFmt( "%d", nAnimSpeedIndex ) );
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_SPEED_INDEX )
	{
		editParameters.nAnimSpeedIndex = pModelEditor->editorSettings.nGameTimerSpeed + 10;
		if ( editParameters.nAnimSpeedIndex < 0 )
		{
			editParameters.nAnimSpeedIndex = 0;
		}
		if (	editParameters.nAnimSpeedIndex >= editParameters.animSpeedList.size() )
		{
			editParameters.nAnimSpeedIndex = editParameters.animSpeedList.size() - 1;
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_TYPE )
	{
		editParameters.eAnimType = pModelEditor->editorSettings.bAnimationsCircle ? SEditParameters::AT_CIRCLE : SEditParameters::AT_LINE;
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_RADIUS_COUNT )
	{
		editParameters.animRadiusList.clear();
		for ( int nAnimRadiusIndex = 2; nAnimRadiusIndex <= 32; ++nAnimRadiusIndex )
		{
			editParameters.animRadiusList.push_back( StrFmt( "%d", nAnimRadiusIndex ) );
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_RADIUS_INDEX )
	{
		editParameters.nAnimRadiusIndex = pModelEditor->editorSettings.fAnimationsCircleDistance - 2.0f;
		if ( editParameters.nAnimRadiusIndex < 0 )
		{
			editParameters.nAnimRadiusIndex = 0;
		}
		if (	editParameters.nAnimRadiusIndex >= editParameters.animRadiusList.size() )
		{
			editParameters.nAnimRadiusIndex = editParameters.animRadiusList.size() - 1;
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_DISTANCE_COUNT )
	{
		editParameters.animDistanceList.clear();
		for ( int nAnimDistanceIndex = 2; nAnimDistanceIndex <= 16; ++nAnimDistanceIndex )
		{
			editParameters.animDistanceList.push_back( StrFmt( "%d", nAnimDistanceIndex ) );
		}
	}
	if ( editParameters.nFlags & MODEL_EP_ANIM_DISTANCE_INDEX )
	{
		editParameters.nAnimDistanceIndex = pModelEditor->editorSettings.fAnimationsBetweenDistance - 2.0f;
		if ( editParameters.nAnimDistanceIndex < 0 )
		{
			editParameters.nAnimDistanceIndex = 0;
		}
		if (	editParameters.nAnimDistanceIndex >= editParameters.animDistanceList.size() )
		{
			editParameters.nAnimDistanceIndex = editParameters.animDistanceList.size() - 1;
		}
	}
	if ( editParameters.nFlags & MODEL_EP_AI_GEOMETRY )
	{
		editParameters.bAIGeometry = pModelEditor->editorSettings.bDrawAIGeometry;
	}
	if ( editParameters.nFlags & MODEL_EP_AI_GEOMETRY_TYPE )
	{
		editParameters.eAIGeometryType = pModelEditor->editorSettings.bShowSolidAIGeometry ? SEditParameters::AIGT_SOLID : SEditParameters::AIGT_TRANSPARENT;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CModelState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	if ( pModelEditor == 0 )
	{
		return false;
	}
	//
	switch( nCommandID ) 
	{
		case ID_MODEL_DRAW_TERRAIN:
		{
			pModelEditor->editorSettings.bDrawTerrain = !pModelEditor->editorSettings.bDrawTerrain;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_TERRAIN );
			UpdateTerrain();
			break;
		}
		case ID_MODEL_DRAW_ANIMATIONS:
		{
			pModelEditor->editorSettings.bDrawAnimations = !pModelEditor->editorSettings.bDrawAnimations;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_ANIM );
			UpdateModels( false );
			break;
		}
		case ID_MODEL_DRAW_AI_GEOMETRY:
		{
			pModelEditor->editorSettings.bDrawAIGeometry = !pModelEditor->editorSettings.bDrawAIGeometry;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_AI_GEOMETRY );
			UpdateAIGeometry( false );
			break;
		}
		case ID_MODEL_SET_LIGHT:
		{
			SetLight();
			break;
		}
		case ID_MODEL_CENTER_CAMERA:
		{
			CenterCamera();
			break;
		}
		case ID_MODEL_SAVE_CAMERA:
		{
			if ( eModelEditorType == ET_RPGSTATS )
			{
				SaveCameraForRPGStats();
			}
			else
			{
				SaveCamera( true );
			}
			break;
		}
		case ID_MODEL_RESET_CAMERA:
		{
			ResetCamera( true );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_FOV );
			break;
		}
		case ID_MODEL_RELOAD_EDITOR:
		{
			//Singleton<IEditorContainer>()->ReloadActiveEditor( true );
			Leave();
			Enter();
			//UpdateTerrain();
			//UpdateTime( false );
			//UpdateSceneColor( false );
			//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_ALL );
			break;
		}
		case ID_MODEL_SPEED_DOWN:
		{
			if ( pModelEditor->editorSettings.nGameTimerSpeed > ( -10 ) )
			{
				--( pModelEditor->editorSettings.nGameTimerSpeed );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_ANIM_SPEED_INDEX );
				UpdateTime( false );
			}
			break;
		}
		case ID_MODEL_SPEED_UP:
		{
			if ( pModelEditor->editorSettings.nGameTimerSpeed < 10 )
			{
				++( pModelEditor->editorSettings.nGameTimerSpeed );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_SET_EDIT_PARAMETERS, MODEL_EP_ANIM_SPEED_INDEX );
				UpdateTime( false );
			}
			break;
		}
		case ID_GET_EDIT_PARAMETERS:
		{
			editParameters.nFlags = dwData;
			::GetEditParameters( &editParameters, CHID_MODEL_WINDOW );
			GetEditParameters( dwData );
			return true;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			editParameters.nFlags = dwData;
			SetEditParameters( dwData );
			::SetEditParameters( editParameters, CHID_MODEL_WINDOW );
			return true;
		}
		default:
			break;
	} 
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CModelState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CModelState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CModelState::UpdateCommand(), pbCheck == 0" );
	//
	if ( pModelEditor == 0 )
	{
		return false;
	}
	//
	( *pbEnable ) = false;
	( *pbCheck ) = false;
	switch( nCommandID ) 
	{
		case ID_MODEL_DRAW_TERRAIN:
		{
			if ( pModelEditor != 0 )
			{
				( *pbEnable ) = true;
				( *pbCheck ) = pModelEditor->editorSettings.bDrawTerrain;
			}
			return true;
		}
		case ID_MODEL_DRAW_ANIMATIONS:
		{
			if ( pModelEditor != 0 )
			{
				( *pbEnable ) = true;
				( *pbCheck ) = pModelEditor->editorSettings.bDrawAnimations;
			}
			return true;
		}
		case ID_MODEL_DRAW_AI_GEOMETRY:
		{
			if ( pModelEditor != 0 )
			{
				( *pbEnable ) = true;
				( *pbCheck ) = pModelEditor->editorSettings.bDrawAIGeometry;
			}
			return true;
		}
		case ID_MODEL_SET_LIGHT:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MODEL_CENTER_CAMERA:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MODEL_SAVE_CAMERA:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MODEL_RESET_CAMERA:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MODEL_RELOAD_EDITOR:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MODEL_SPEED_DOWN:
		{
			( *pbEnable ) = ( pModelEditor->editorSettings.nGameTimerSpeed > ( -10 ) );
			( *pbCheck ) = false;
			return true;
		}
		case ID_MODEL_SPEED_UP:
		{
			( *pbEnable ) = ( pModelEditor->editorSettings.nGameTimerSpeed < 10 );
			( *pbCheck ) = false;
			return true;
		}
		case ID_GET_EDIT_PARAMETERS:
		case ID_SET_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CObjectBase* CModelState::BuildPlane( const CVec3 &vStart, const CVec2 &vSize, const CVec4 &color, int nXStripCount, int nYStripCount, bool bDoubleSided )
{
	if ( ( nXStripCount * nYStripCount ) > 0 )
	{
		if ( CObj<CMemObject> pMemObject = new CMemObject() )
		{
			vector<CVec3> points;
			points.reserve( ( nXStripCount + 1 ) * ( nYStripCount + 1 ) );
			//
			vector<STriangle> tris;
			tris.reserve( nXStripCount * nYStripCount * ( bDoubleSided ? 4 : 2 ) );
			//
			const float fStepX = vSize.x / nXStripCount;
			const float fStepY = vSize.y / nYStripCount;
			//
			for ( int nY = 0; nY <= nYStripCount; ++nY )
			{
				for ( int nX = 0; nX <= nXStripCount; ++nX )
				{
					points.push_back( CVec3( nX * fStepX, nY * fStepY, 0.0f ) );
				}
			}
			//
			for ( int nY = 0; nY < nYStripCount; ++nY )
			{
				for ( int nX = 0; nX < nXStripCount; ++nX )
				{
					tris.push_back( STriangle( nX + nY * ( nXStripCount + 1 ),
																		 nX + nY * ( nXStripCount + 1 ) + 1,
																		 nX + ( nY + 1 ) * ( nXStripCount + 1 ) + 1 ) );
					tris.push_back( STriangle( nX + nY * ( nXStripCount + 1 ),
																		 nX + ( nY + 1 ) * ( nXStripCount + 1 ) + 1,
																		 nX + ( nY + 1 ) * ( nXStripCount + 1 ) ) );
					if ( bDoubleSided )
					{
						tris.push_back( STriangle( nX + nY * ( nXStripCount + 1 ),
																			 nX + ( nY + 1 ) * ( nXStripCount + 1 ) + 1,
																			 nX + nY * ( nXStripCount + 1 ) + 1  ) );
						tris.push_back( STriangle( nX + nY * ( nXStripCount + 1 ),
																			 nX + ( nY + 1 ) * ( nXStripCount + 1 ),
																			 nX + ( nY + 1 ) * ( nXStripCount + 1 ) + 1 ) );
					}
				}
			}
			//
			pMemObject->Create( points, tris );
			NGScene::IGameView *pGameView = EditorScene()->GetGView();
			NGScene::IGameView::SMeshInfo meshInfo;
			meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( NGScene::CreateObjectInfo( pMemObject ), pGameView->CreateMaterial( color, true ) ) );
			return pGameView->CreateMesh( meshInfo, MakeTransform( vStart ), 0, 0 );
		}
	}
	else
	{
		if ( CObj<CMemObject> pMemObject = new CMemObject() )
		{
			vector<CVec3> points;
			points.reserve( 4 );
			//
			vector<STriangle> tris;
			tris.reserve( bDoubleSided ? 4 : 2 );
			//
			points.push_back( CVec3( 0.0f, 0.0f, 0.0f ) );
			points.push_back( CVec3( vSize.x, 0.0f, 0.0f ) );
			points.push_back( CVec3( vSize.x, vSize.y, 0.0f ) );
			points.push_back( CVec3( 0.0f, vSize.y, 0.0f ) );
			//
			tris.push_back( STriangle( 0, 1, 2 ) );
			tris.push_back( STriangle( 0, 2, 3 ) );
			if ( bDoubleSided )
			{
				tris.push_back( STriangle( 0, 2, 1 ) );
				tris.push_back( STriangle( 0, 3, 2 ) );
			}
			//
			pMemObject->Create( points, tris );
			NGScene::IGameView *pGameView = EditorScene()->GetGView();
			NGScene::IGameView::SMeshInfo meshInfo;
			meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( NGScene::CreateObjectInfo( pMemObject ), pGameView->CreateMaterial( color, true ) ) );
			return pGameView->CreateMesh( meshInfo, MakeTransform( vStart ), 0, 0 );
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CObjectBase* CModelState::BuildPlane( const CVec3 &vStart, const CVec2 &vSize, const CVec4 &color, int nXStripCount, int nYStripCount, float fDiff, bool bDoubleSided )
{
	NI_ASSERT( nXStripCount * nYStripCount, "Zero nStrip value" );
	if ( ( nXStripCount * nYStripCount ) == 0 )
	{
		return 0;
	}
	if ( fDiff == 0.0f )
	{
		return BuildPlane( vStart, vSize, color, nXStripCount, nYStripCount, bDoubleSided );
	}
	if ( CObj<CMemObject> pMemObject = new CMemObject() )
	{
		vector<CVec3> points;
		points.reserve( nXStripCount * nYStripCount * 4 );
		//
		vector<STriangle> tris;
		tris.reserve( nXStripCount * nYStripCount * ( bDoubleSided ? 4 : 2 ) );
		//
		const float fStepX = vSize.x / nXStripCount;
		const float fStepY = vSize.y / nYStripCount;
		//
		for ( int nY = 0; nY < nYStripCount; ++nY )
		{
			for ( int nX = 0; nX < nXStripCount; ++nX )
			{
				points.push_back( CVec3( nX * fStepX + fDiff / 2.0f,
																 nY * fStepY + fDiff / 2.0f,
																 0.0f ) );
				points.push_back( CVec3( ( nX + 1 ) * fStepX - fDiff / 2.0f,
																 nY * fStepY + fDiff / 2.0f,
																 0.0f ) );
				points.push_back( CVec3( ( nX + 1 ) * fStepX - fDiff / 2.0f,
																 ( nY + 1 ) * fStepY - fDiff / 2.0f,
																 0.0f ) );
				points.push_back( CVec3( nX * fStepX + fDiff / 2.0f,
																 ( nY + 1 ) * fStepY - fDiff / 2.0f,
																 0.0f ) );
			}
		}
		//
		for ( int nY = 0; nY < nYStripCount; ++nY )
		{
			for ( int nX = 0; nX < nXStripCount; ++nX )
			{
				const int nStartPointIndex = ( nX + nY * nXStripCount ) * 4;
				tris.push_back( STriangle( nStartPointIndex, nStartPointIndex + 1, nStartPointIndex + 2 ) );
				tris.push_back( STriangle( nStartPointIndex, nStartPointIndex + 2, nStartPointIndex + 3 ) );
				if ( bDoubleSided )
				{
					tris.push_back( STriangle( nStartPointIndex, nStartPointIndex + 2, nStartPointIndex + 1 ) );
					tris.push_back( STriangle( nStartPointIndex, nStartPointIndex + 3, nStartPointIndex + 2 ) );
				}
			}
		}
		//
		pMemObject->Create( points, tris );
		NGScene::IGameView *pGameView = EditorScene()->GetGView();
		NGScene::IGameView::SMeshInfo meshInfo;
		meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( NGScene::CreateObjectInfo( pMemObject ), pGameView->CreateMaterial( color, true ) ) );
		return pGameView->CreateMesh( meshInfo, MakeTransform( vStart ), 0, 0 );
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
template<class TPolygon, class TPoint>
int GetIntersectionPointList( const TPolygon &rPolygon, const TPoint &rvBegin, const TPoint &rvEnd, bool bPolygon, vector<float> *pIntersectionPointList )
{
	//ноль точек
	if ( rPolygon.empty() )
	{
		return CI_UNKNOWN;
	}
	TPolygon::const_iterator itSourcePoint0 = rPolygon.begin();
	TPolygon::const_iterator itSourcePoint1 = rPolygon.begin();
	++itSourcePoint1;
	//одна точка
	if ( itSourcePoint1 == rPolygon.end() )
	{
		return CI_UNKNOWN;
	}
	int nIntersectionPointCount = 0;
	float fIntersectionPoint = 0.0f;
	while ( itSourcePoint0 != rPolygon.end() )
	{
		if ( ClassifyCross( *itSourcePoint0, *itSourcePoint1, rvBegin, rvEnd, &fIntersectionPoint ) == CI_SKEW_CROSS )
		{
			++nIntersectionPointCount;
			if ( pIntersectionPointList != 0 )
			{
				pIntersectionPointList->push_back( fIntersectionPoint );	
			}
		}
		++itSourcePoint0;
		++itSourcePoint1;
		if ( itSourcePoint1 == rPolygon.end() )
		{
			if ( bPolygon )
			{
				itSourcePoint1 = rPolygon.begin();
			}
			else
			{
				break;
			}
		}
	}
	if ( pIntersectionPointList != 0 )
	{
		sort( pIntersectionPointList->begin(), pIntersectionPointList->end() );
	}
	return nIntersectionPointCount;
}
/**/
	/**
	if ( Singleton<IFolderCallback>()->IsUniqueName( "EditorTest", "1\\1" ) )
	{
		DebugTrace( "CModelState::OnKeyDown(): 1\\1 unique" );
	}
	else
	{
		DebugTrace( "CModelState::OnKeyDown(): 1\\1 exists" );
	}
	if ( Singleton<IFolderCallback>()->IsUniqueName( "EditorTest", "1\\1\\" ) )
	{
		DebugTrace( "CModelState::OnKeyDown(): 1\\1\\ unique" );
	}
	else
	{
		DebugTrace( "CModelState::OnKeyDown(): 1\\1\\ exists" );
	}
	/**/
	/**
	vector<CVec3> vPointList;
	CVec3 vBegin( 7.0f, 0.0f, 0.0f );
	CVec3 vEnd( 7.0f, 16.0f, 0.0f );
	const int nPointCount = 256;
	const CVec3 vCenterPoint( 0.0f, 8.0f, 0.0f );
	const float fRadius = 7.0f;
	vPointList.reserve( nPointCount );
	for ( int nPointIndex = 0; nPointIndex < nPointCount; ++nPointIndex )
	{
		CVec3 vPoint = vCenterPoint + CVec3( 0.0f, fRadius, 0.0f );
		RotatePoint( &vPoint, nPointIndex * FP_2PI / nPointCount, vCenterPoint );
		vPointList.push_back( vPoint );
	}	
	vector<float> intersectionPointList;
	//
	const int nCaseCount = 64 * 64 * 128;
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	for ( int nCaseIndex = 0; nCaseIndex < nCaseCount; ++nCaseIndex )
	{
		const int nIntersectionPointCount = GetIntersectionPointList( vPointList, vBegin, vEnd, true, 0 );
	}
	DebugTrace( "CModelState::OnKeyDown(): %g", NHPTimer::GetTimePassed( &time ) );
/**/
/**
	{
		CFreeIDCollector freeIDCollector;	
		DebugTrace( "Empty:" );
		freeIDCollector.Trace();
		//
		int nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		freeIDCollector.Clear();	
		DebugTrace( "Clear:" );
		freeIDCollector.Trace();
		//
		for ( int nTestIndex = 0; nTestIndex < 10; ++nTestIndex )
		{
			freeIDCollector.LockID();
		}
		DebugTrace( "Filled:" );
		freeIDCollector.Trace();
		//
		int nFreeID = 8;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 7;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 6;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 9;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 10;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 11;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 1;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 2;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nFreeID = 4;
		freeIDCollector.FreeID( nFreeID );
		DebugTrace( "FreeID(): %d", nFreeID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
		//
		nLockID = freeIDCollector.LockID();
		DebugTrace( "LockID(): %d", nLockID );
		freeIDCollector.Trace();
	}
	{
		int nCount = 1;
		for ( int nTestIndex = 0; nTestIndex < 8; ++nTestIndex )
		{
			CFreeIDCollector freeIDCollector;	
			{
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				int nLockID = -1;
				for ( int nIndex = 0; nIndex < nCount; ++nIndex )
				{
					nLockID = freeIDCollector.LockID();
				}
				const float fTime = NHPTimer::GetTimePassed( &time );
				DebugTrace( "LockID(): test: %d, Lock %d times: %g", nTestIndex, nCount, fTime );
			}
			freeIDCollector.Trace();
			{
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				for ( int nIndex = 0; nIndex < nCount; nIndex += 2 )
				{
					freeIDCollector.FreeID( nIndex );
				}
				const float fTime = NHPTimer::GetTimePassed( &time );
				DebugTrace( "LockID(): test: %d, Free %d times: %g", nTestIndex, nCount / 2, fTime );
			}
			if ( nCount < 101 )
			{
				freeIDCollector.Trace();
			}
			{
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				for ( int nIndex = 0; nIndex < nCount; nIndex += 2 )
				{
					freeIDCollector.LockID();
				}
				const float fTime = NHPTimer::GetTimePassed( &time );
				DebugTrace( "LockID(): test: %d, Lock %d times: %g", nTestIndex, nCount / 2, fTime );
			}
			freeIDCollector.Trace();
			nCount *= 10;
		}
	}
/**/
