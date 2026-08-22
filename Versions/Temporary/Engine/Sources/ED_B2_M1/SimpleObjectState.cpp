#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "SceneB2/VisObjDesc.h"
#include "Stats_B2_M1/AnimationType.h"
#include "libdb/ResourceManager.h"
#include "Main/GameTimer.h"
#include "SimpleObjectInfoData.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "SimpleObjectState.h"
#include "ResourceDefines.h"


#include "MapEditorLib/Interface_Logger.h"

#include "MapObjectMultiState.h"
#include "SimpleObjectState.h"
#include "Misc/Win32Random.h"
#include "EditorMethods.h"
#include "SeasonMnemonics.h"
#include "MapInfoEditor.h"

#include <cstdint>

#include <zconf.h>

void CSimpleObjectState::ClearData()
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( !sceneObjectlist.empty() )
		{
			for ( CSceneObjectList::const_iterator itSceneObject = sceneObjectlist.begin(); itSceneObject != sceneObjectlist.end(); ++itSceneObject )
			{
				if ( itSceneObject->nID != INVALID_NODE_ID )
				{
					if ( CPtr<SAIDissapearObjUpdate> pUpdate = new SAIDissapearObjUpdate() )
					{
						pUpdate->eUpdateType = ACTION_NOTIFY_DISSAPEAR_OBJ;
						pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
						pUpdate->nDissapearObjID = itSceneObject->nID;
						pUpdate->bShowEffects = false;
						//
						GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
					}
				}
			}
		}
	}
	sceneObjectlist.clear();
	szRPGStatsTypeName.clear();
	rpgStatsDBID.Clear();
}


bool CSimpleObjectState::CanAddSimpleObject()
{
	bool bResult = false;
	SObjectSet objectSet;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
	{
		bResult = ( ( objectSet.szObjectTypeName == "MineRPGStats" ) ||
								( objectSet.szObjectTypeName == "BuildingRPGStats" ) ||
								( objectSet.szObjectTypeName == "MechUnitRPGStats" ) ||
								( objectSet.szObjectTypeName == "ObjectRPGStats" ) ||
								( objectSet.szObjectTypeName == "SquadRPGStats" ) ) &&
							( !objectSet.objectNameSet.empty() );
		if ( bResult )
		{
			bResult = false;
			if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( GetMapInfoEditor()->GetViewManipulator()->GetDesc( "HiddenObjectType" ) ) )
			{
				for ( SPropertyDesc::CTypesMap::const_iterator itObjectTypeName = pDesc->refTypes.begin(); itObjectTypeName != pDesc->refTypes.end(); ++itObjectTypeName )
				{
					if ( itObjectTypeName->first == objectSet.szObjectTypeName )
					{
						bResult = true;
					}
				}
			}
			if ( !bResult )
			{
				NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Object have no visual part: %s%c%s\n", objectSet.szObjectTypeName.c_str(), TYPE_SEPARATOR_CHAR, objectSet.objectNameSet.begin()->first.ToString().c_str() ) );
			}
		}
		if ( bResult )
		{
			bResult = false;
			if ( objectSet.szObjectTypeName != "SquadRPGStats" )
			{
				if ( CPtr<IManipulator> pRPGStatsManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
				{
					bResult = true;
					CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pRPGStatsManipulator, 0, 0, 0 );
					if ( pVisObjectManipulator == 0 )
					{
						bResult = false;
						NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Object have no visual part: %s%c%s\n", objectSet.szObjectTypeName.c_str(), TYPE_SEPARATOR_CHAR, objectSet.objectNameSet.begin()->first.ToString().c_str() ) );
					}
				}
			}
			else
			{
				if ( CPtr<IManipulator> pRPGStatsManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
				{
					bResult = true;
					int nMemberCount = 0;
					CManipulatorManager::GetValue( &nMemberCount, pRPGStatsManipulator, "members" );
					if ( nMemberCount > 0 )
					{
						for ( int nMemberIndex = 0; nMemberIndex < nMemberCount; ++nMemberIndex )
						{
							CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( StrFmt( "members.[%d]", nMemberIndex ), pRPGStatsManipulator, 0, 0, 0 );
							if ( pVisObjectManipulator == 0 )
							{
								bResult = false;
								NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Object have no visual part: %s%c%s.member[%d]\n", objectSet.szObjectTypeName.c_str(), TYPE_SEPARATOR_CHAR, objectSet.objectNameSet.begin()->first.ToString().c_str(), nMemberIndex ) );
								break;
							}
						}
					}
					else
					{
						bResult = false;
						NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Object have no visual part: %s%c%s\n", objectSet.szObjectTypeName.c_str(), TYPE_SEPARATOR_CHAR, objectSet.objectNameSet.begin()->first.ToString().c_str() ) );
					}
				}
			}
		}
	}
	return bResult;
}


bool CSimpleObjectState::InsertObjectMouseMove( unsigned nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		const float fDirection = ( ( pEditParameters->fDirection * FP_PI ) / 180.0f );
		const CVec3 vPosition = pStoreInputState->lastEventInfo.vTerrainPos;
		for ( CSceneObjectList::const_iterator itSceneObject = sceneObjectlist.begin(); itSceneObject != sceneObjectlist.end(); ++itSceneObject )
		{
			CVec3 vAdditionalPosition = itSceneObject->vPosition;
			RotatePoint( &vAdditionalPosition, fDirection, VNULL3 );
			CVec3 vObjectScenePosition = vPosition + vAdditionalPosition;
			vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
			//
			const float fObjectSceneDirection = fDirection + itSceneObject->fDirection;
			uint32_t dwNormal = Vec3ToDWORD( V3_AXIS_Z );
			CQuat qObjectSceneRotation = CQuat( fObjectSceneDirection, V3_AXIS_Z );
			if ( SSimpleObjectInfo::NeedMakeOrientation( szRPGStatsTypeName, rpgStatsDBID ) )
			{
				dwNormal = EditorScene()->GetNormal( CVec2( vObjectScenePosition.x, vObjectScenePosition.y ) );
				MakeOrientation( &qObjectSceneRotation, DWORDToVec3( dwNormal ) );
			}
			if ( CPtr<SAIPlacementUpdate> pUpdate = new SAIPlacementUpdate() )
			{
				pUpdate->eUpdateType = ACTION_NOTIFY_PLACEMENT;
				pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
				pUpdate->info.nObjUniqueID = itSceneObject->nID;
				pUpdate->info.bNewFormat = true;
				pUpdate->info.vPlacement = vObjectScenePosition;
				pUpdate->info.rotation = qObjectSceneRotation;
				//
				pUpdate->info.center = CVec2( vObjectScenePosition.x, vObjectScenePosition.y );
				pUpdate->info.z = vObjectScenePosition.z;
				pUpdate->info.dir = Vis2AIRad( fObjectSceneDirection );
				pUpdate->info.dwNormal = dwNormal;
				//
				pUpdate->info.fSpeed = 1.0f;
				//
				GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
			}
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	return false;
}


bool CSimpleObjectState::InsertObjectLButtonUp( unsigned nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		IEditorScene *pScene = EditorScene();
		ICamera *pCamera = Camera();
		IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
		if ( ( pScene == 0 ) || ( pCamera == 0 ) || ( pManipulator == 0 ) )
		{
			return false;
		}
		//
		if ( !CanAddSimpleObject() )
		{
			return false;
		}
		//
		CWaitCursor waitCursor;
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			szRPGStatsTypeName = objectSet.szObjectTypeName;
			rpgStatsDBID = objectSet.objectNameSet.begin()->first;
		}
		//
		const float fDirection = ( pEditParameters->fDirection * FP_PI ) / 180.0f;
		if ( !rpgStatsDBID.IsEmpty() )
		{
			pEditParameters->nFlags = MIMOSEP_PLAYER_INDEX;
			GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
			//
			NMapInfoEditor::SObjectCreateInfo objectCreateInfo;
			objectCreateInfo.vPosition = rTerrainPos;
			objectCreateInfo.vPosition.z = 0.0f;
			objectCreateInfo.fDirection = fDirection;
			objectCreateInfo.szRPGStatsTypeName = szRPGStatsTypeName;
			objectCreateInfo.rpgStatsDBID = rpgStatsDBID;
			objectCreateInfo.nFrameIndex = INVALID_NODE_ID;
			objectCreateInfo.nPlayer = pEditParameters->nPlayerIndex;
			objectCreateInfo.fHP = 1.0f;
			objectCreateInfo.bRotateTo90Degree = GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
			objectCreateInfo.bFitToGrid = GetMapInfoEditor()->editorSettings.bFitToGrid;
			//
			unsigned nSimpleObjectInfoID = INVALID_NODE_ID;
			if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
			{
				if ( NMapInfoEditor::SSimpleObjectInfo *pSimpleObjectInfo = GetMapInfoEditor()->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSimpleObjectInfo*>( 0 ), &nSimpleObjectInfoID ) )
				{
					if ( pSimpleObjectInfo->Create( &objectCreateInfo, pScene, pObjectController, pManipulator ) )
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
		//
		if ( pEditParameters->eDirectionType == CMapObjectMultiState::SEditParameters::DT_RANDOM )
		{
			pEditParameters->nFlags = MIMOSEP_DIRECTION;
			pEditParameters->fDirection = NWin32Random::Random( 360 ) * 1.0f;
			SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	return false;
}


bool CSimpleObjectState::InsertObjectRButtonUp( unsigned nFlags, const CVec3 &rTerrainPos )
{
	return true;
}


bool CSimpleObjectState::InsertObjectKeyDown( unsigned nChar, unsigned nFlags, const CVec3 &rTerrainPos )
{
	if ( nChar == VK_ESCAPE )
	{
		return true;
	}
	/**
	else
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	/**/
	return false;
}


void CSimpleObjectState::InsertObjectEnter()
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		IEditorScene *pScene = EditorScene();
		ICamera *pCamera = Camera();
		IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
		if ( ( pScene == 0 ) || ( pCamera == 0 ) || ( pManipulator == 0 ) )
		{
			return;
		}
		//
		if ( !CanAddSimpleObject() )
		{
			return;
		}
		//
		CVec3 vPosition = pCamera->GetAnchor();
		Vis2AI( &vPosition );
		vPosition.z = GetTerrainHeight( vPosition.x, vPosition.y );
		//
		if ( pEditParameters->eDirectionType == CMapObjectMultiState::SEditParameters::DT_RANDOM )
		{
			pEditParameters->nFlags = MIMOSEP_DIRECTION;
			pEditParameters->fDirection = NWin32Random::Random( 360 ) * 1.0f;
			SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
		}
		const float fDirection = ( pEditParameters->fDirection * FP_PI ) / 180.0f;
		//
		ClearData();
		SObjectSet objectSet;
		//
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			szRPGStatsTypeName = objectSet.szObjectTypeName;
			rpgStatsDBID = objectSet.objectNameSet.begin()->first;
		}
		// вставляем объект
		if ( !rpgStatsDBID.IsEmpty() )
		{
			string szSeason;
			CManipulatorManager::GetValue( &szSeason, pManipulator, "Season" );
			const NDb::ESeason eSeason = static_cast<NDb::ESeason>( typeSeasonMnemonics.GetValue( szSeason ) );
			// заполняем сцену и проставляем ссылки в mapInfo
			CWaitCursor waitCursor;
			const NDb::SHPObjectRPGStats *pHPObjectRPGStats = dynamic_cast<const NDb::SHPObjectRPGStats*>( NDb::GetObject( rpgStatsDBID ) );
			if ( !pHPObjectRPGStats )
			{
				return;
			}
			if ( szRPGStatsTypeName == "SquadRPGStats" )
			{
				if ( const NDb::SSquadRPGStats *pSquadRPGStats = dynamic_cast<const NDb::SSquadRPGStats*>( pHPObjectRPGStats ) )
				{
					const NDb::SSquadRPGStats::SFormation *pFormation = 0;
					for ( int nFormationIndex = 0; nFormationIndex < pSquadRPGStats->formations.size(); ++nFormationIndex )
					{
						if ( pSquadRPGStats->formations[nFormationIndex].etype == NDb::SSquadRPGStats::SFormation::DEFAULT )
						{
							pFormation = &( pSquadRPGStats->formations[nFormationIndex] );
							break;
						}
					}
					if ( pFormation != 0 )
 					{
						for ( int nMemberIndex = 0; ( nMemberIndex < pFormation->order.size() ) && ( nMemberIndex < pSquadRPGStats->members.size() ); ++nMemberIndex )
						{
							if ( pSquadRPGStats->members[nMemberIndex] != 0 )
							{
								CVec3 vAdditionalPosition = CVec3( pFormation->order[nMemberIndex].vPos.x, pFormation->order[nMemberIndex].vPos.y, 0.0f );
								float fAdditionalDirection = AI2VisRad( pFormation->order[nMemberIndex].fDir );
								//
								CVec3 vObjectScenePosition = vPosition + vAdditionalPosition;
								vObjectScenePosition.z += GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
								const float fObjectSceneDirection = fDirection + fAdditionalDirection;
								uint32_t dwNormal = Vec3ToDWORD( V3_AXIS_Z );
								CQuat qObjectSceneRotation = CQuat( fObjectSceneDirection, V3_AXIS_Z );
								if ( SSimpleObjectInfo::NeedMakeOrientation( szRPGStatsTypeName, rpgStatsDBID ) )
								{
									dwNormal = EditorScene()->GetNormal( CVec2( vObjectScenePosition.x, vObjectScenePosition.y ) );
									MakeOrientation( &qObjectSceneRotation, DWORDToVec3( dwNormal ) );
								}
								//
								const unsigned nSceneID = GetMapInfoEditor()->objectInfoCollector.sceneIDCollector.LockID();
								if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
								{
									pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
									pUpdate->info.nObjUniqueID = nSceneID;
									pUpdate->info.pStats = pSquadRPGStats->members[nMemberIndex];
									pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
									pUpdate->info.bNewFormat = true;
									pUpdate->info.vPlacement = vObjectScenePosition;
									pUpdate->info.rotation = qObjectSceneRotation;
									//
									pUpdate->info.center = CVec2( vObjectScenePosition.x, vObjectScenePosition.y );
									pUpdate->info.z = vObjectScenePosition.z;
									pUpdate->info.dir = Vis2AIRad( fObjectSceneDirection );
									pUpdate->info.dwNormal = dwNormal;
									//
									pUpdate->info.fSpeed = 0.0f;
									pUpdate->info.cSoil = 0;
									pUpdate->info.fResize = 1.0f;
									pUpdate->info.fHitPoints = pSquadRPGStats->members[nMemberIndex]->fMaxHP;
									pUpdate->info.fFuel = 0.0f;
									pUpdate->info.eDipl = EDI_FRIEND;
									pUpdate->info.nPlayer = pEditParameters->nPlayerIndex;
									pUpdate->info.nFrameIndex = -1;
									pUpdate->info.nExpLevel = 0;
									//
									GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
								}		
								
								if ( CPtr<SAIActionUpdate> pUpdate = new SAIActionUpdate( nSceneID, ACTION_NOTIFY_ANIMATION_CHANGED, NDb::ANIMATION_IDLE, Singleton<IGameTimer>()->GetGameTime() ) )
 								{
									GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
								}
								CSceneObjectList::iterator itSceneObject = sceneObjectlist.insert( sceneObjectlist.end(), SSceneObject() );
								itSceneObject->nID = nSceneID;
								itSceneObject->vPosition = vAdditionalPosition;
								itSceneObject->fDirection = fAdditionalDirection;
							}
						}
					}
				}
			}
			else
			{
				uint32_t dwNormal = Vec3ToDWORD( V3_AXIS_Z );
				CQuat qRotation = CQuat( fDirection, V3_AXIS_Z );
				if ( SSimpleObjectInfo::NeedMakeOrientation( szRPGStatsTypeName, rpgStatsDBID ) )
				{
					dwNormal = EditorScene()->GetNormal( CVec2( vPosition.x, vPosition.y ) );
					MakeOrientation( &qRotation, DWORDToVec3( dwNormal ) );
				}
				//
				const unsigned nSceneID = GetMapInfoEditor()->objectInfoCollector.sceneIDCollector.LockID();
				if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
				{
					pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
					pUpdate->info.nObjUniqueID = nSceneID;
					pUpdate->info.pStats = NDb::Get<NDb::SHPObjectRPGStats>( rpgStatsDBID );
					pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
					pUpdate->info.bNewFormat = true;
					pUpdate->info.vPlacement = vPosition;
					pUpdate->info.rotation = qRotation;
					//
					pUpdate->info.center = CVec2( vPosition.x, vPosition.y );
					pUpdate->info.z = vPosition.z;
					pUpdate->info.dir = Vis2AIRad( fDirection );
					pUpdate->info.dwNormal = dwNormal;
					//
					pUpdate->info.fSpeed = 0.0f;
					pUpdate->info.cSoil = 0;
					pUpdate->info.fResize = 1.0f;
					pUpdate->info.fHitPoints = pHPObjectRPGStats->fMaxHP;
					pUpdate->info.fFuel = 0.0f;
					pUpdate->info.eDipl = EDI_FRIEND;
					pUpdate->info.nPlayer = pEditParameters->nPlayerIndex;
					pUpdate->info.nFrameIndex = -1;
					pUpdate->info.nExpLevel = 0;
					//
					GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
				}		
				CSceneObjectList::iterator itSceneObject = sceneObjectlist.insert( sceneObjectlist.end(), SSceneObject() );
				itSceneObject->nID = nSceneID;
				itSceneObject->vPosition = VNULL3;
				itSceneObject->fDirection = 0.0f;
			}
			//
			list<int> sceneIDlist;
			for ( CSceneObjectList::const_iterator itSceneObject = sceneObjectlist.begin(); itSceneObject != sceneObjectlist.end(); ++itSceneObject )
			{
				sceneIDlist.push_back( itSceneObject->nID );
			}
			pScene->SetFadedObjects( sceneIDlist, NMapInfoEditor::SCENE_FADE_COEFFICIENT );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CSimpleObjectState::InsertObjectLeave()
{
	ClearData();
}


void CSimpleObjectState::InsertObjectDraw( class CPaintDC *pPaintDC )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		if ( IEditorScene *pScene = EditorScene() )
		{
			CVec3 vPlacementPosition = pStoreInputState->lastEventInfo.vTerrainPos;
			vPlacementPosition.z = GetTerrainHeight( vPlacementPosition.x, vPlacementPosition.y );
			CVec3 vPlacementSelectorPosition = vPlacementPosition;
			sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS0, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
			sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS1, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
			float fPlacementDirection = ( ( pEditParameters->fDirection * FP_PI ) / 180.0f );
			const float fDirection = fPlacementDirection;
			fPlacementDirection += FP_PI2;
			if ( fPlacementDirection > FP_2PI )
			{ 
				fPlacementDirection -= FP_2PI;
			}
			CVec2 vDirection = CreateFromPolarCoord( NMapInfoEditor::PLACEMENT_DIRECTION_RADIUS, fPlacementDirection );
			const CVec3 vPlacementSelectorDirection( vDirection.x + vPlacementSelectorPosition.x, vDirection.y + vPlacementSelectorPosition.y, vPlacementSelectorPosition.z );
			sceneDrawTool.DrawLine( vPlacementSelectorPosition, vPlacementSelectorDirection, NMapInfoEditor::PLACEMENT_COLOR, false );
		}
	}
}

// basement storage  


/**

void CMapObjectAddState::InsertMapObject()
{
	unsigned nNewMapObjectID = INVALID_NODE_ID;
	if ( pParentState->PrepareControlPoints( &( pParentState->controlPointList ) ) )
	{
		bool bMapObjectIsValid = true;
		int nMinCount = INVALID_NODE_ID;
		int nMaxCount = INVALID_NODE_ID;
		pParentState->GetBounds( &nMinCount, &nMaxCount );
		if ( ( nMinCount != INVALID_NODE_ID ) && ( pParentState->controlPointList.size() < nMinCount ) )
		{
			bMapObjectIsValid = false;
		}
		else if ( ( nMaxCount != INVALID_NODE_ID ) && ( pParentState->controlPointList.size() > nMaxCount ) )
		{
			bMapObjectIsValid = false;
		}
		if ( bMapObjectIsValid )
		{
			nNewMapObjectID = pParentState->InsertMapObject( pParentState->controlPointList );
		}
	}
	pParentState->nSelectedControlPoint = INVALID_NODE_ID;
	pParentState->controlPointList.clear();
	pParentState->pickMapObjectIDList.clear();
	pParentState->nSelectedIndex = INVALID_NODE_ID;
	//
	if ( nNewMapObjectID != INVALID_NODE_ID )
	{
		pParentState->pickMapObjectIDList.push_back( nNewMapObjectID );
		pParentState->nSelectedIndex = 0;
		//
		// Запоманаем первоначальное расположение контрольных точек
		if ( CMapObjectState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedMapObjectID() ) )
		{
			pParentState->controlPointList = ( *pControlPointList );
		}
		else
		{
			pParentState->controlPointList.clear();
		}
		pParentState->UpdateMapObject( pParentState->GetSelectedMapObjectID(), CMapObjectState::UT_START );
		pParentState->SetActiveInputState( CMapObjectState::IS_EDIT, true, false );
	}
	else
	{
		pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}
/**/

