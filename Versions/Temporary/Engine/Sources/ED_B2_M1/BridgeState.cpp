#include "StdAfx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "sceneb2/scene.h"
#include "main/gametimer.h"
#include "mapeditorlib/commoneditormethods.h"
#include "ResourceDefines.h"

#include "MapObjectMultiState.h"
#include "BridgeState.h"
#include "MapInfoEditor.h"

#include "libdb/ResourceManager.h"
#include "EditorMethods.h"
#include "SeasonMnemonics.h"

#include <zconf.h>

const int CBridgeState::START_TERMINATOR_INDEX = 0;
const int CBridgeState::FINISH_TERMINATOR_INDEX = 1;
const int CBridgeState::TERMINATOR_COUNT = 2;


bool CBridgeState::CanAddBridge()
{
	SObjectSet objectSet;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
	{
		return ( objectSet.szObjectTypeName == "BridgeRPGStats" ) &&
					 ( !objectSet.objectNameSet.empty() );
	}
	return false;
}


void CBridgeState::InsertObjectEnter()
{
	ClearScene();
	ClearData();
	//
	CWaitCursor waitCursor;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) );
}


void CBridgeState::InsertObjectLeave()
{
	ClearScene();
	ClearData();
}


void CBridgeState::InsertObjectDraw( CPaintDC *pPaintDC )
{
	if ( bPlaceBridge )
	{
		const int nBridgeElementCount = bridgeElementCenterPointList.size();
		int nBridgeElementIndex = 0;
		float fDirection = GetPolarAngle( vDirection ); 
		for ( NMapInfoEditor::CBridgeCenterPointList::const_iterator itBridgeElementCenterPoint = bridgeElementCenterPointList.begin(); itBridgeElementCenterPoint != bridgeElementCenterPointList.end(); ++itBridgeElementCenterPoint )
		{
			CVec2 vHalfSize = VNULL2;
			NMapInfoEditor::CBridgeCenterPointList bridgeElementPointList;
			if ( ( nBridgeElementIndex == 0 ) || ( nBridgeElementIndex == ( nBridgeElementCount - 1 ) ) )
			{
				vHalfSize = vEndSize / 2.0f;
			}
			else
			{
				vHalfSize = vCenterSize / 2.0f;
			}
			bridgeElementPointList.push_back( CVec3( -vHalfSize.x, -vHalfSize.y, 0.0f ) );
			bridgeElementPointList.push_back( CVec3(  vHalfSize.x, -vHalfSize.y, 0.0f ) );
			bridgeElementPointList.push_back( CVec3(  vHalfSize.x,  vHalfSize.y, 0.0f ) );
			bridgeElementPointList.push_back( CVec3( -vHalfSize.x,  vHalfSize.y, 0.0f ) );
			//
			RotatePoints<NMapInfoEditor::CBridgeCenterPointList, CVec3>( &bridgeElementPointList, fDirection );
			CVec3 vCenterPoint = *itBridgeElementCenterPoint;
			vCenterPoint.z += GetTerrainHeight( itBridgeElementCenterPoint->x, itBridgeElementCenterPoint->y );
			MovePoints( &bridgeElementPointList, vCenterPoint );
			sceneDrawTool.DrawPolyline( bridgeElementPointList, PLACEMENT_COLOR, true, false );
			sceneDrawTool.DrawCircle( vCenterPoint, NMapInfoEditor::PLACEMENT_RADIUS0, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
			sceneDrawTool.DrawCircle( vCenterPoint, NMapInfoEditor::PLACEMENT_RADIUS1, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
			//
			++nBridgeElementIndex;
		}
	}
	else
	{
		CVec3 vPlacementPosition = pStoreInputState->lastEventInfo.vTerrainPos;
		vPlacementPosition.z = GetTerrainHeight( vPlacementPosition.x, vPlacementPosition.y );
		CVec3 vPlacementSelectorPosition = vPlacementPosition;
		sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS0, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
		sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS1, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
	}
}


bool CBridgeState::InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( bPlaceBridge )
	{
		vEnd = rTerrainPos;
		if ( fabs( vEnd - vStart ) < FP_EPSILON )
		{
			vEnd = vStart + CVec3( AI_TILE_SIZE * 1.0f, 0.0f, 0.0f );
		}
		InsertBridge( SBridgeInfo::DIRECTION_FREE, true, false );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}


bool CBridgeState::InsertObjectLButtonDown( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() && CanAddBridge() )
	{
		if ( bPlaceBridge )
		{
			vEnd = rTerrainPos;
			InsertBridge( SBridgeInfo::DIRECTION_FREE, true, true );
			ClearScene();
			ClearData();
			bPlaceBridge = false;
		}
		else
		{
			ClearScene();
			ClearData();
			bPlaceBridge = true;
			vStart = rTerrainPos;
			vEnd = vStart + CVec3( AI_TILE_SIZE * 1.0f, 0.0f, 0.0f );
			InsertBridge( SBridgeInfo::DIRECTION_FREE, true, false );
		}
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}



bool CBridgeState::InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( bPlaceBridge )
	{
		ClearScene();
		ClearData();
		bPlaceBridge = false;
	}
	else
	{
		return true;
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}


bool CBridgeState::InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() && CanAddBridge() )
	{
		if ( nChar == VK_ESCAPE )
		{
			if ( bPlaceBridge )
			{
				ClearScene();
				ClearData();
				bPlaceBridge = false;
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
			else
			{
				return true;
			}
		}
		else if ( nChar == VK_RETURN )
		{
			if ( bPlaceBridge )
			{
				vEnd = rTerrainPos;
				InsertBridge( SBridgeInfo::DIRECTION_FREE, true, true );
				ClearScene();
				ClearData();
				bPlaceBridge = false;
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
		}
	}
	return false;
}


void CBridgeState::ClearScene()
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( !sceneIDlist.empty() )
		{
			for ( NMapInfoEditor::CSceneIDList::const_iterator itSceneID = sceneIDlist.begin(); itSceneID != sceneIDlist.end(); ++itSceneID )
			{
				if ( CPtr<SAIDissapearObjUpdate> pUpdate = new SAIDissapearObjUpdate() )
				{
					pUpdate->eUpdateType = ACTION_NOTIFY_DISSAPEAR_OBJ;
					pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
					pUpdate->nDissapearObjID = ( *itSceneID );
					pUpdate->bShowEffects = false;
					//
					GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
				}
			}
		}
	}
}


void CBridgeState::ClearData()
{
	objectSet.Clear();
	sceneIDlist.clear();
	vStart = VNULL3;
	vEnd = VNULL3;
	bridgeElementCenterPointList.clear();
	vDirection = VNULL3;
	vEndSize = VNULL2;
	vCenterSize = VNULL2;
	bPlaceBridge = false;
}


void CBridgeState::InsertBridge( SBridgeInfo::EDirection direction, bool bFixStartPoint, bool bPlace )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		IEditorScene *pScene = EditorScene();
		ICamera *pCamera = Camera();
		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
		if ( ( pScene == 0 ) || ( pCamera == 0 ) || ( pResourceManager == 0 ) || ( pManipulator == 0 ) )
		{
			return;
		}
		//
		pEditParameters->nFlags = MIMOSEP_PLAYER_INDEX;
		GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
		//
		// получаем размер 
		if ( objectSet.szObjectTypeName.empty() )
		{
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
			{
				CPtr<IManipulator> pBridgeRPGStatsManipulator = pResourceManager->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first );
				if ( pBridgeRPGStatsManipulator )
				{
					vEndSize = VNULL2;
					vCenterSize = VNULL2;
					CManipulatorManager::GetVec2<CVec2, float>( &vEndSize, pBridgeRPGStatsManipulator, "End.Size" );
					CManipulatorManager::GetVec2<CVec2, float>( &vCenterSize, pBridgeRPGStatsManipulator, "Center.Size" );
				}
			}
		}
		// находим опорные точки моста
		bridgeElementCenterPointList.clear();
		vDirection = VNULL3;
		if ( ( vEndSize.x > FP_EPSILON ) && ( vCenterSize.x > FP_EPSILON ) )
		{
			SBridgeInfo::CreateCenterPoints( &bridgeElementCenterPointList, &vDirection, vStart, vEnd, direction, vEndSize.x, vCenterSize.x, bFixStartPoint );
			const int nBridgeElementCount = bridgeElementCenterPointList.size();
			if ( nBridgeElementCount > 0 )
			{
				float fAverageZ = 0.0f;
				for ( NMapInfoEditor::CBridgeCenterPointList::const_iterator itBridgeElementCenterPoint = bridgeElementCenterPointList.begin(); itBridgeElementCenterPoint != bridgeElementCenterPointList.end(); ++itBridgeElementCenterPoint )
				{
					fAverageZ += GetTerrainHeight( itBridgeElementCenterPoint->x, itBridgeElementCenterPoint->y );
				}
				fAverageZ = fAverageZ / ( 1.0f * nBridgeElementCount );
				for ( NMapInfoEditor::CBridgeCenterPointList::iterator itBridgeElementCenterPoint = bridgeElementCenterPointList.begin(); itBridgeElementCenterPoint != bridgeElementCenterPointList.end(); ++itBridgeElementCenterPoint )
				{
					itBridgeElementCenterPoint->z = fAverageZ - GetTerrainHeight( itBridgeElementCenterPoint->x, itBridgeElementCenterPoint->y );
				}
			}
		}
		//
		const NDb::SBridgeRPGStats *pBridgeRPGStats = 0;
		pBridgeRPGStats = dynamic_cast<const NDb::SBridgeRPGStats*>( NDb::GetObject( objectSet.objectNameSet.begin()->first ) );
		NI_ASSERT( pBridgeRPGStats != 0, StrFmt( "%s not NDb::SBridgeRPGStats type", objectSet.szObjectTypeName.c_str() ) ); 
		//
		if ( bPlace )
		{
			CWaitCursor waitCursor;
			// Записываем мост в базу
			ClearScene();
			const int nBridgeElementCount = bridgeElementCenterPointList.size();
			if ( nBridgeElementCount > 1 )
			{
				NMapInfoEditor::SBridgeCreateInfo bridgeCreateInfo;
				bridgeCreateInfo.centerPointList = bridgeElementCenterPointList;
				bridgeCreateInfo.fDirection = GetPolarAngle( vDirection );
				bridgeCreateInfo.nPlayer = pEditParameters->nPlayerIndex;
				bridgeCreateInfo.fHP = 1.0f;
				bridgeCreateInfo.szRPGStatsTypeName = objectSet.szObjectTypeName;
				bridgeCreateInfo.rpgStatsDBID = objectSet.objectNameSet.begin()->first;
				UINT nBridgeInfoID = INVALID_NODE_ID;
				if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
				{
					if ( NMapInfoEditor::SBridgeInfo *pBridgeInfo = GetMapInfoEditor()->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SBridgeInfo*>( 0 ), &nBridgeInfoID ) )
					{
						if ( pBridgeInfo->Create( &bridgeCreateInfo, pScene, pObjectController, pManipulator ) )
						{
							pObjectController->Redo( false, true, GetMapInfoEditor() );
							Singleton<IControllerContainer>()->Add( pObjectController );
						}
						else
						{
							pObjectController->Undo( true, false, GetMapInfoEditor() );
						}
					}
				}
			}
		}
		else
		{
			if ( bridgeElementCenterPointList.empty() )
			{
				ClearScene();
			}
			else
			{
				if ( sceneIDlist.empty() )
				{
					string szSeason;
					CManipulatorManager::GetValue( &szSeason, pManipulator, "Season" );
					const NDb::ESeason eSeason = static_cast<NDb::ESeason>( typeSeasonMnemonics.GetValue( szSeason ) );
					//
					for ( int nTerminatorIndex = 0; nTerminatorIndex < TERMINATOR_COUNT; ++nTerminatorIndex )
					{
						const UINT nSceneID = GetMapInfoEditor()->objectInfoCollector.sceneIDCollector.LockID();
						if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
						{
							pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
							pUpdate->info.nObjUniqueID = nSceneID;
							pUpdate->info.pStats = NDb::Get<NDb::SHPObjectRPGStats>( objectSet.objectNameSet.begin()->first );
							pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
							pUpdate->info.bNewFormat = true;
							pUpdate->info.vPlacement = VNULL3;
							pUpdate->info.rotation = QNULL;
							//
							pUpdate->info.center = CVec2( 0.0f, 0.0f );
							pUpdate->info.z = 0.0f;
							pUpdate->info.dir = Vis2AIRad( 0.0f );
							pUpdate->info.dwNormal = Vec3ToDWORD( V3_AXIS_Z );
							//
							pUpdate->info.fSpeed = 0.0f;
							pUpdate->info.cSoil = 0;
							pUpdate->info.fResize = 1.0f;
							pUpdate->info.fHitPoints = pBridgeRPGStats->fMaxHP;
							pUpdate->info.fFuel = 1.0f;
							pUpdate->info.eDipl = EDI_FRIEND;
							pUpdate->info.nPlayer = pEditParameters->nPlayerIndex;
							pUpdate->info.nFrameIndex = 0;
							pUpdate->info.nExpLevel = 0;
							//
							GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
						}		
						sceneIDlist.push_back( nSceneID );
					}
					pScene->SetFadedObjects( sceneIDlist, NMapInfoEditor::SCENE_FADE_COEFFICIENT );
				}
				//
				const int nAdditionalSpanCount = bridgeElementCenterPointList.size() - sceneIDlist.size();
				if ( nAdditionalSpanCount > 0 )
				{
					string szSeason;
					CManipulatorManager::GetValue( &szSeason, pManipulator, "Season" );
					const NDb::ESeason eSeason = static_cast<NDb::ESeason>( typeSeasonMnemonics.GetValue( szSeason ) );
					//
					NMapInfoEditor::CSceneIDList newSceneIDList;
					for ( int nTerminatorIndex = 0; nTerminatorIndex < nAdditionalSpanCount; ++nTerminatorIndex )
					{
						const UINT nSceneID = GetMapInfoEditor()->objectInfoCollector.sceneIDCollector.LockID();
						if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
						{
							pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
							pUpdate->info.nObjUniqueID = nSceneID;
							pUpdate->info.pStats = NDb::Get<NDb::SHPObjectRPGStats>( objectSet.objectNameSet.begin()->first );
							pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
							pUpdate->info.bNewFormat = true;
							pUpdate->info.vPlacement = VNULL3;
							pUpdate->info.rotation = QNULL;
							//
							pUpdate->info.center = CVec2( 0.0f, 0.0f );
							pUpdate->info.z = 0.0f;
							pUpdate->info.dir = Vis2AIRad( 0.0f );
							pUpdate->info.dwNormal = Vec3ToDWORD( V3_AXIS_Z );
							//
							pUpdate->info.fSpeed = 0.0f;
							pUpdate->info.cSoil = 0;
							pUpdate->info.fResize = 1.0f;
							pUpdate->info.fHitPoints = pBridgeRPGStats->fMaxHP;
							pUpdate->info.fFuel = 1.0f;
							pUpdate->info.eDipl = EDI_FRIEND;
							pUpdate->info.nPlayer = pEditParameters->nPlayerIndex;
							pUpdate->info.nFrameIndex = 1;
							pUpdate->info.nExpLevel = 0;
							//
							GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
						}		
						sceneIDlist.push_back( nSceneID );
						newSceneIDList.push_back( nSceneID );
					}
					pScene->SetFadedObjects( newSceneIDList, NMapInfoEditor::SCENE_FADE_COEFFICIENT );
				}
				else
				{
					for ( int nTerminatorIndex = 0; nTerminatorIndex < abs( nAdditionalSpanCount ); ++nTerminatorIndex )
					{
						const UINT nSceneID = sceneIDlist.back();
						if ( CPtr<SAIDissapearObjUpdate> pUpdate = new SAIDissapearObjUpdate() )
						{
							pUpdate->eUpdateType = ACTION_NOTIFY_DISSAPEAR_OBJ;
							pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
							pUpdate->nDissapearObjID = nSceneID;
							pUpdate->bShowEffects = false;
							//
							GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
						}
						sceneIDlist.pop_back();
					}
				}
				// ставим повое изображение моста
				const float fInDirectionAngle = GetPolarAngle( vDirection ); 
				int nBridgeElementSceneID = INVALID_NODE_ID;
				//
				NMapInfoEditor::CSceneIDList::const_iterator itStartTerminator = sceneIDlist.begin();
				NMapInfoEditor::CSceneIDList::const_iterator itFinishTerminator = itStartTerminator;
				++itFinishTerminator;
				NMapInfoEditor::CSceneIDList::const_iterator itSpanTerminator = itFinishTerminator;
				++itSpanTerminator;
				//
				for ( NMapInfoEditor::CBridgeCenterPointList::iterator itBridgeElementCenterPoint = bridgeElementCenterPointList.begin(); itBridgeElementCenterPoint != bridgeElementCenterPointList.end(); ++itBridgeElementCenterPoint )
				{
					CVec3 vObjectScenePosition = *itBridgeElementCenterPoint;
					vObjectScenePosition.z += GetTerrainHeight( itBridgeElementCenterPoint->x, itBridgeElementCenterPoint->y );
					//
					float fDirection = fInDirectionAngle;
					if ( itBridgeElementCenterPoint ==  bridgeElementCenterPointList.begin() )
					{
						nBridgeElementSceneID = *itStartTerminator;
					}
					else if ( itBridgeElementCenterPoint == ( --( bridgeElementCenterPointList.end() ) ) )
					{
						nBridgeElementSceneID = *itFinishTerminator;
						fDirection = ( fInDirectionAngle >= FP_PI ) ? ( fInDirectionAngle - FP_PI ) : ( fInDirectionAngle + FP_PI );
					}
					else
					{
						nBridgeElementSceneID = *itSpanTerminator;
						++itSpanTerminator;
					}
					//
					const CQuat qObjectSceneRotation( fDirection, V3_AXIS_Z );
					const DWORD dwNormal = Vec3ToDWORD( V3_AXIS_Z );
					//
					if ( CPtr<SAIPlacementUpdate> pUpdate = new SAIPlacementUpdate() )
					{
						pUpdate->eUpdateType = ACTION_NOTIFY_PLACEMENT;
						pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
						pUpdate->info.nObjUniqueID = nBridgeElementSceneID;
						pUpdate->info.bNewFormat = true;
						pUpdate->info.vPlacement = vObjectScenePosition;
						pUpdate->info.rotation = qObjectSceneRotation;
						//
						pUpdate->info.center = CVec2( vObjectScenePosition.x, vObjectScenePosition.y );
						pUpdate->info.z = vObjectScenePosition.z;
						pUpdate->info.dir = Vis2AIRad( fDirection );
						pUpdate->info.dwNormal = dwNormal;
						//
						pUpdate->info.fSpeed = 1.0f;
						//
						GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
					}
				}
			}
		}
	}
}


// basement storage  

/**
			// Поднимаем землю под мостом
			if ( nSelectedMapObjectInfoID != INVALID_NODE_ID )
			{
				if ( CanEdit() )
				{
					SMapInfo::CMapObjectInfoMap::iterator posMapObjectInfo = GetMapInfoEditor()->objectInfoCollector.objectInfoMap.find( nSelectedMapObjectInfoID );
					if ( posMapObjectInfo != GetMapInfoEditor()->objectInfoCollector.objectInfoMap.end() )
					{
						SBridgeInfo *pMapObjectInfo = checked_cast<SBridgeInfo*>( &( *posMapObjectInfo->second ) );
						if ( pMapObjectInfo )
						{
							//Вычисляем параметры поднятия земли под объектом
							bool bStartEmpty = true;
							bool bEndEmpty = true;
							CVec3 vStart = VNULL3;
							CVec3 vEnd = VNULL3;
							float fCommonHeight = 0.0f;
							float fLenght = 0.0f;
							float fWidth = 0.0f;
							float fHeight = 0.0f;
							for ( SMapObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = pMapObjectInfo->mapInfoElementMap.begin(); itMapInfoElement != pMapObjectInfo->mapInfoElementMap.end(); ++itMapInfoElement )
							{
								const CVec3 vPos = itMapInfoElement->second.GetPosition( pMapObjectInfo->vPosition );
								fCommonHeight += ( vPos.z + GetTerrainHeight( vPos.x, vPos.y ) );
								if ( itMapInfoElement->second.nFrameIndex == 0 )
								{
									if ( bStartEmpty )
									{
										bStartEmpty = false;
										vStart = vPos;
									}
									else if ( bEndEmpty )
									{
										bEndEmpty = false;
										vEnd = vPos;
									}
									CPtr<IManipulator> pBridgeRPGStatsManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( itMapInfoElement->second.nRPGStatsTypeID, itMapInfoElement->second.nRPGStatsID );
									if ( pBridgeRPGStatsManipulator != 0 )
									{
										CVec2 vSize = VNULL2;
										CManipulatorManager::GetVec2<CVec2, float>( &vSize, pBridgeRPGStatsManipulator, "End.Size" );
										CManipulatorManager::GetValue( &fHeight, pBridgeRPGStatsManipulator, "Height" );
										fLenght = vSize.x;
										fWidth = vSize.y;
									}
								}
							}
							const CVec3 vNormDirection = ( vEnd - vStart ) / fabs( vEnd - vStart );
							vStart -= vNormDirection * ( fWidth / 2.0f );
							vEnd += vNormDirection * ( fWidth / 2.0f );
							fCommonHeight = fCommonHeight / ( pMapObjectInfo->mapInfoElementMap.size() * 1.0f );
							// Поднимаем землю
							if ( IEditorScene *pScene = EditorScene() )
							{
								if ( ITerraManager *pTerrain = pScene->GetTerrain() )
								{
									pTerrain->ApplyBridgeTerraForm( CVec2( vStart.x, vStart.y ), CVec2( vEnd.x, vEnd.y ), fWidth, fCommonHeight + fHeight );
									GetMapInfoEditor()->bNeedSave = true;
								}
							}
							// Вычисляем новую высоту
							float fNewCommonHeight = 0.0f;
							for ( SMapObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = pMapObjectInfo->mapInfoElementMap.begin(); itMapInfoElement != pMapObjectInfo->mapInfoElementMap.end(); ++itMapInfoElement )
							{
								const CVec3 vPos = itMapInfoElement->second.GetPosition( pMapObjectInfo->vPosition );
								fNewCommonHeight += ( vPos.z + GetTerrainHeight( vPos.x, vPos.y ) );
							}
							fNewCommonHeight = fNewCommonHeight / ( pMapObjectInfo->mapInfoElementMap.size() * 1.0f );
							// Подгоняем мост под новую высоту
							CVec3 vPos = VNULL3;
							Get3DPosOnMapHeights( &vPos, CVec2( previousMousePoint.x, previousMousePoint.y ), ( *GetStaticMapHeights() ) );
							vPos.z = 0.0f;
							vPos.z = fCommonHeight - fNewCommonHeight;
							if ( vPos.z != 0.0f )
							{
								MoveBridge( vPos, false, true, false, false );
							}
						}
					}
				}
			}
		}
	}
/**/

