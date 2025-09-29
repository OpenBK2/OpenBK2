#include "StdAfx.h"

#include "MapInfoEditorData_ObjectInfo.h"
#include "MapInfoEditorData_ObjectInfoCollector.h"
#include "..\MapEditorLib\Interface_Logger.h"
#include "..\Misc\PlaneGeometry.h"
#include "..\Stats_B2_M1\AnimationType.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::MakeAbsolute()
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			itMapInfoElement->second.vPosition = itMapInfoElement->second.GetPosition( vPosition );
			itMapInfoElement->second.vAdditionalPosition = VNULL3;
			itMapInfoElement->second.fDirection = itMapInfoElement->second.GetDirection( fDirection );
			itMapInfoElement->second.fAdditionalDirection = 0.0f;
		}
		vPosition = VNULL3;
		fDirection = 0.0f;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::MakeRelative()
	{
		CVec3 vRelativePosition = VNULL3;
		float fRelativeDirection = 0.0f;
		int nElementCount = 0;
		for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			vRelativePosition += itMapInfoElement->second.GetPosition( vPosition );
			fRelativeDirection += itMapInfoElement->second.GetDirection( fDirection );
			++nElementCount;
		}
		if ( nElementCount > 0 )
		{
			vPosition = vRelativePosition / ( nElementCount * 1.0f );
			fDirection = fRelativeDirection / ( nElementCount * 1.0f );
		}
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			itMapInfoElement->second.vPosition -= vPosition;
			itMapInfoElement->second.vAdditionalPosition = VNULL3;
			itMapInfoElement->second.fDirection -= fDirection;
			itMapInfoElement->second.fAdditionalDirection = 0.0f;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::Pick( UINT nSceneID )
	{
		CSceneElementMap::const_iterator itSceneElement = sceneElementMap.find( nSceneID );
		return ( itSceneElement != sceneElementMap.end() );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::Draw( CSceneDrawTool *pEditorSceneDrawTool )
	{
		if ( !pEditorSceneDrawTool )
		{
			return false;
		}
		//
		DWORD dwSceneObject = 0;
		DWORD dwObject = 0;
		DWORD dwObjectLink = 0;
		DWORD dwMainObject = 0;
		GetDrawSelectionParameters( &dwSceneObject, &dwObject, &dwObjectLink, &dwMainObject );
		//
		if ( dwSceneObject > 0 )
		{
			for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
			{
				CVec3 vObjectScenePosition = itSceneElement->second.GetPosition( vPosition );
				vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
				if ( ( dwSceneObject & DRAW_SELECTION_CIRCLE0 ) > 0 )
				{
					pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, SCENE_OBJECT_SELECTION_RADIUS0, SCENE_OBJECT_SELECTION_PARTS, SCENE_OBJECT_SELECTION_COLOR, false );
				}
				if ( ( dwSceneObject & DRAW_SELECTION_CIRCLE1 ) > 0 )
				{
					pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, SCENE_OBJECT_SELECTION_RADIUS1, SCENE_OBJECT_SELECTION_PARTS, SCENE_OBJECT_SELECTION_COLOR, false );
				}
				if ( ( dwSceneObject & DRAW_DIRECTION ) > 0 )
				{
					float fObjectSceneDirection = itSceneElement->second.GetDirection( fDirection ) + FP_PI2;
					if ( fObjectSceneDirection > FP_2PI )
					{ 
						fObjectSceneDirection -= FP_2PI;
					}
					CVec2 vDirection = CreateFromPolarCoord( SCENE_OBJECT_DIRECTION_RADIUS, fObjectSceneDirection );
					const CVec3 vObjectSceneDirection( vDirection.x + vObjectScenePosition.x, vDirection.y + vObjectScenePosition.y, vObjectScenePosition.z );
					//
					pEditorSceneDrawTool->DrawLine( vObjectScenePosition, vObjectSceneDirection, SCENE_OBJECT_SELECTION_COLOR, false );
				}
			}
		}
		//
		if ( dwObject )
		{
			for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
			{
				CVec3 vObjectScenePosition = itMapInfoElement->second.GetPosition( vPosition );
				vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
				if ( ( dwObject & DRAW_SELECTION_CIRCLE0 ) > 0 )
				{
					pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, OBJECT_SELECTION_RADIUS0, OBJECT_SELECTION_PARTS, OBJECT_SELECTION_COLOR, false );
				}
				if ( ( dwObject & DRAW_SELECTION_CIRCLE1 ) > 0 )
				{
					pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, OBJECT_SELECTION_RADIUS1, OBJECT_SELECTION_PARTS, OBJECT_SELECTION_COLOR, false );
				}
				if ( ( dwObject & DRAW_DIRECTION ) > 0 )
				{
					float fObjectSceneDirection = itMapInfoElement->second.GetDirection( fDirection ) + FP_PI2;
					if ( fObjectSceneDirection > FP_2PI )
					{ 
						fObjectSceneDirection -= FP_2PI;
					}
					CVec2 vDirection = CreateFromPolarCoord( OBJECT_DIRECTION_RADIUS, fObjectSceneDirection );
					const CVec3 vObjectSceneDirection( vDirection.x + vObjectScenePosition.x, vDirection.y + vObjectScenePosition.y, vObjectScenePosition.z );
					//
					pEditorSceneDrawTool->DrawLine( vObjectScenePosition, vObjectSceneDirection, OBJECT_SELECTION_COLOR, false );
				}
			}
		}
		//
		if ( dwObjectLink )
		{
			for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
			{
				if ( const SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( itMapInfoElement->second.nLinkToLinkID ) )
				{
					for ( SObjectInfo::CMapInfoElementMap::const_iterator itLinkToMapInfoElement = pLinkToObjectInfo->mapInfoElementMap.begin(); itLinkToMapInfoElement != pLinkToObjectInfo->mapInfoElementMap.end(); ++itLinkToMapInfoElement )
					{
						for ( CLinkIDList::const_iterator itLinkedLinkID = itLinkToMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itLinkToMapInfoElement->second.linkedLinkIDIist.end(); ++itLinkedLinkID )
						{
							if ( ( *itLinkedLinkID ) == ( itMapInfoElement->first ) )
							{
								CVec3 vObjectScenePosition = itMapInfoElement->second.GetPosition( vPosition );
								vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
								//
								CVec3 vLinkToObjectScenePosition = itLinkToMapInfoElement->second.GetPosition( pLinkToObjectInfo->vPosition );
								vLinkToObjectScenePosition.z = GetTerrainHeight( vLinkToObjectScenePosition.x, vLinkToObjectScenePosition.y );

								if ( ( dwObjectLink & DRAW_SELECTION_CIRCLE0 ) > 0 )
								{
									pEditorSceneDrawTool->DrawCircle( vLinkToObjectScenePosition, OBJECT_LINK_RADIUS0, OBJECT_LINK_PARTS, OBJECT_LINK_COLOR, false );
								}
								if ( ( dwObjectLink & DRAW_SELECTION_CIRCLE1 ) > 0 )
								{
									pEditorSceneDrawTool->DrawCircle( vLinkToObjectScenePosition, OBJECT_LINK_RADIUS1, OBJECT_LINK_PARTS, OBJECT_LINK_COLOR, false );
								}
								if ( ( dwObjectLink & DRAW_DIRECTION ) > 0 )
								{
									pEditorSceneDrawTool->DrawLine( vObjectScenePosition, vLinkToObjectScenePosition, OBJECT_LINK_COLOR, false );
								}
							}
						}
					}
				}
			}
		}
		if ( dwMainObject )
		{
			CVec3 vObjectScenePosition = vPosition;
			vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
			if ( ( dwMainObject & DRAW_SELECTION_CIRCLE0 ) > 0 )
			{
				pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, MAIN_OBJECT_SELECTION_RADIUS0, MAIN_OBJECT_SELECTION_PARTS, MAIN_OBJECT_SELECTION_COLOR, false );
			}
			if ( ( dwMainObject & DRAW_SELECTION_CIRCLE1 ) > 0 )
			{
				pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, MAIN_OBJECT_SELECTION_RADIUS1, MAIN_OBJECT_SELECTION_PARTS, MAIN_OBJECT_SELECTION_COLOR, false );
			}
			if ( ( dwMainObject & DRAW_DIRECTION ) > 0 )
			{
				float fObjectSceneDirection = fDirection + FP_PI2;
				if ( fObjectSceneDirection > FP_2PI )
				{ 
					fObjectSceneDirection -= FP_2PI;
				}
				//
				CVec2 vDirection = CreateFromPolarCoord( MAIN_OBJECT_DIRECTION_RADIUS, fObjectSceneDirection );
				const CVec3 vObjectSceneDirection( vDirection.x + vObjectScenePosition.x, vDirection.y + vObjectScenePosition.y, vObjectScenePosition.z );
				//
				pEditorSceneDrawTool->DrawLine( vObjectScenePosition, vObjectSceneDirection, MAIN_OBJECT_SELECTION_COLOR, false );
			}
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::UpdateDBLinkedObjects( CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			for ( CLinkIDList::const_iterator itLinkedLinkID = itMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itMapInfoElement->second.linkedLinkIDIist.end(); ++itLinkedLinkID )
			{
				if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( *itLinkedLinkID ) )
				{
					if ( !pLinkToObjectInfo->UpdateDB( true, pObjectController, pManipulator ) )
					{
						bResult = false;
					}
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::MoveLinkedObjects( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition,
																			 bool bUpdateScene, IEditorScene *pEditorScene,
																			 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			for ( CLinkIDList::const_iterator itLinkedLinkID = itMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itMapInfoElement->second.linkedLinkIDIist.end(); ++itLinkedLinkID )
			{
				if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( *itLinkedLinkID ) )
				{
					const CVec3 vDifference = pLinkToObjectInfo->vPosition - vPosition;
					if ( !pLinkToObjectInfo->Move( pObjectEditInfo, rvNewPosition + vDifference,
																					true,
																					bUpdateScene, pEditorScene,
																					bUpdateDB, pObjectController, pManipulator ) )
					{
						bResult = false;
					}
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::RotateLinkedObjects( const SObjectEditInfo *pObjectEditInfo, float fNewDirection,
																				 bool bUpdateScene, IEditorScene *pEditorScene,
																				 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			for ( CLinkIDList::const_iterator itLinkedLinkID = itMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itMapInfoElement->second.linkedLinkIDIist.end(); ++itLinkedLinkID )
			{
				if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( *itLinkedLinkID ) )
				{
					const float fDifference = pLinkToObjectInfo->fDirection- fDirection;
					if ( !pLinkToObjectInfo->Rotate( pObjectEditInfo, fNewDirection + fDifference,
																						true,
																						bUpdateScene, pEditorScene,
																						bUpdateDB, pObjectController, pManipulator ) )
					{
						bResult = false;
					}
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::UpdateSceneElements( bool bModel, bool bPosition, bool bDirection, float fAdditionalDirection )
	{
		for ( SObjectInfo::CSceneElementMap::iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
		{
			SObjectInfo::CSceneIDToLinkIDMap::const_iterator posSceneIDToLinkID = sceneIDToLinkIDMap.find( itSceneElement->first );
			if ( posSceneIDToLinkID != sceneIDToLinkIDMap.end() )
			{
				SObjectInfo::CMapInfoElementMap::const_iterator posMapInfoElement = mapInfoElementMap.find( posSceneIDToLinkID->second );
				if ( posMapInfoElement != mapInfoElementMap.end() )
				{
					if ( bPosition )
					{
						itSceneElement->second.vPosition = posMapInfoElement->second.GetPosition( VNULL3 );
					}
					if ( bDirection )
					{
						RotatePoint( &( itSceneElement->second.vAdditionalPosition ), fAdditionalDirection + ( posMapInfoElement->second.GetDirection( 0.0f ) - itSceneElement->second.fDirection ), VNULL3 );
						itSceneElement->second.fDirection = posMapInfoElement->second.GetDirection( 0.0f );
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::ClearAdditionalPosition( bool bUpdateSceneElements )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			itMapInfoElement->second.vAdditionalPosition = VNULL3;
		}
		if ( bUpdateSceneElements )
		{
			UpdateSceneElements( false, true, false, 0.0f );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::ClearAdditionalDirection( bool bUpdateSceneElements )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			itMapInfoElement->second.fAdditionalDirection = 0.0f;
		}
		if ( bUpdateSceneElements )
		{
			UpdateSceneElements( false, false, true, 0.0f );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::FitToGrid( bool bUpdateSceneElements )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const NDb::SHPObjectRPGStats *pHPObjectRPGStats = dynamic_cast<const NDb::SHPObjectRPGStats*>( NDb::GetObject( itMapInfoElement->second.rpgStatsDBID ) );
			const NDb::SObjectBaseRPGStats *pObjectBaseRPGStats = dynamic_cast<const NDb::SObjectBaseRPGStats*>( pHPObjectRPGStats );
			if ( ( pObjectBaseRPGStats != 0 ) && ( !pObjectBaseRPGStats->passability.IsEmpty() ) )
			{
				CVec2 vOrigin = pObjectBaseRPGStats->vOrigin;
				RotatePoint( &vOrigin, itMapInfoElement->second.GetDirection( fDirection ) );
				// позиция без учета добавочных смещений
				CVec3 vNewPosition = vPosition + itMapInfoElement->second.vPosition;
				FitAIOrigin2AIGrid( &vNewPosition, vOrigin );
				// записываем смещение в добавочное смещение
				itMapInfoElement->second.vAdditionalPosition = vNewPosition - ( vPosition + itMapInfoElement->second.vPosition );
				itMapInfoElement->second.vAdditionalPosition.z = 0.0f;
			}
		}
		//
		if ( bUpdateSceneElements )
		{
			UpdateSceneElements( false, true, false, 0.0f );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::RotateTo90Degree( bool bUpdateSceneElements )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const NDb::SHPObjectRPGStats *pHPObjectRPGStats = dynamic_cast<const NDb::SHPObjectRPGStats*>( NDb::GetObject( itMapInfoElement->second.rpgStatsDBID ) );
			const NDb::SObjectBaseRPGStats *pObjectBaseRPGStats = dynamic_cast<const NDb::SObjectBaseRPGStats*>( pHPObjectRPGStats );
			if ( ( pObjectBaseRPGStats != 0 ) && ( !pObjectBaseRPGStats->passability.IsEmpty() ) )
			{
				// поворот без учета добавочных поворотов
				float fNewDirection = fDirection + itMapInfoElement->second.fDirection;
				fNewDirection = ToDegree( fNewDirection ) / 90.0f;
				fNewDirection = floor( fNewDirection );
				fNewDirection = ToRadian( fNewDirection * 90.0f );
				itMapInfoElement->second.fAdditionalDirection = fNewDirection - ( fDirection + itMapInfoElement->second.fDirection );
			}
		}
		//
		if ( bUpdateSceneElements )
		{
			UpdateSceneElements( false, false, true, 0.0f );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::SetCommonHeight( bool bUpdateSceneElements )
	{
		float fCommonHeight = 0;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const CVec3 vPos = itMapInfoElement->second.GetPosition( vPosition );
			fCommonHeight += ( vPos.z + GetTerrainHeight( vPos.x, vPos.y ) );
		}
		//
		fCommonHeight = fCommonHeight / ( mapInfoElementMap.size() * 1.0f );
		//
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const CVec3 vPos = itMapInfoElement->second.GetPosition( vPosition );
			const float fAdditionalHeight = fCommonHeight - GetTerrainHeight( vPos.x, vPos.y );
			//
			itMapInfoElement->second.vPosition.z = fAdditionalHeight - vPosition.z;
			itMapInfoElement->second.vAdditionalPosition.z = 0.0f;
		}
		//
		if ( bUpdateSceneElements )
		{
			UpdateSceneElements( false, true, false, 0.0f );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::FixInvalidPos( bool bUpdateSceneElements )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			if ( KeepZeroHeight() )
			{
				itMapInfoElement->second.vPosition.z = 0.0f;
			}
			itMapInfoElement->second.FixInvalidPos( pObjectInfoCollector->mapSize, vPosition );
		}
		//
		if ( bUpdateSceneElements )
		{
			UpdateSceneElements( false, true, false, 0.0f );
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::PostLoad( IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		// make reverse references
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( itMapInfoElement->second.nLinkToLinkID ) )
			{
				for ( SObjectInfo::CMapInfoElementMap::iterator itLinkedMapInfoElement = pLinkToObjectInfo->mapInfoElementMap.begin(); itLinkedMapInfoElement != pLinkToObjectInfo->mapInfoElementMap.end(); ++itLinkedMapInfoElement )
				{
					if ( itMapInfoElement->second.nLinkToLinkID == itLinkedMapInfoElement->first )
					{
						itLinkedMapInfoElement->second.linkedLinkIDIist.push_back( itMapInfoElement->first );
						return true;
					}
				}							
			}
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::CreateSceneObjects( IEditorScene *pEditorScene, IManipulator *pManipulator, bool bUpdateParentStructure )
	{
		// заполняем сцену и проставляем ссылки в mapInfo
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const NDb::SHPObjectRPGStats *pHPObjectRPGStats = dynamic_cast<const NDb::SHPObjectRPGStats*>( NDb::GetObject( itMapInfoElement->second.rpgStatsDBID ) );
			if ( !pHPObjectRPGStats )
			{
				continue;
			}
			if ( itMapInfoElement->second.szRPGStatsTypeName == "SquadRPGStats" )
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
								RotatePoint( &vAdditionalPosition, itMapInfoElement->second.GetDirection( fDirection ), VNULL3 );
								//
								CVec3 vObjectScenePosition = itMapInfoElement->second.GetPosition( vPosition ) + vAdditionalPosition;
								vObjectScenePosition.z += GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
								const float fObjectSceneDirection = itMapInfoElement->second.GetDirection( fDirection ) + fAdditionalDirection;
								DWORD dwNormal = Vec3ToDWORD( V3_AXIS_Z );
								CQuat qObjectSceneRotation = CQuat( fObjectSceneDirection, V3_AXIS_Z );
								if ( NeedMakeOrientation() )
								{
									dwNormal = EditorScene()->GetNormal( CVec2( vObjectScenePosition.x, vObjectScenePosition.y ) );
									MakeOrientation( &qObjectSceneRotation, DWORDToVec3( dwNormal ) );
								}
								const UINT nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
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
									pUpdate->info.fHitPoints = itMapInfoElement->second.fHP * pSquadRPGStats->members[nMemberIndex]->fMaxHP;
									pUpdate->info.fFuel = 1.0f;
									pUpdate->info.eDipl = EDI_FRIEND;
									pUpdate->info.nPlayer = itMapInfoElement->second.nPlayer;
									pUpdate->info.nFrameIndex = itMapInfoElement->second.nFrameIndex;
									pUpdate->info.nExpLevel = 0;
									//
									pObjectInfoCollector->pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
								}		
								
								if ( CPtr<SAIActionUpdate> pUpdate = new SAIActionUpdate( nSceneID, ACTION_NOTIFY_ANIMATION_CHANGED, NDb::ANIMATION_IDLE, Singleton<IGameTimer>()->GetGameTime() ) )
 								{
									pObjectInfoCollector->pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
								}

								SObjectInfo::SSceneElement sceneElement;
								sceneElement.vPosition = itMapInfoElement->second.GetPosition( VNULL3 );
								sceneElement.vAdditionalPosition = vAdditionalPosition;
								sceneElement.fDirection = itMapInfoElement->second.GetDirection( 0.0f );
								sceneElement.fAdditionalDirection = fAdditionalDirection;
								// заносим элемент в структуру данных объекта
								sceneElementMap[nSceneID] = sceneElement;
								sceneIDToLinkIDMap[nSceneID] = itMapInfoElement->first;
								//
								if ( bUpdateParentStructure )
								{
									pObjectInfoCollector->sceneIDMap[nSceneID] = nObjectInfoID;
								}
							}
							else
							{
								NLog::Log( LT_ERROR, "Member %d in squad \"%s\" are empty!\n", nMemberIndex, NDb::GetResName(pSquadRPGStats) );
							}
						}
					}
				}
			}
			else
			{
				CVec3 vObjectScenePosition = itMapInfoElement->second.GetPosition( vPosition );
				vObjectScenePosition.z += GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
				const float fObjectSceneDirection = itMapInfoElement->second.GetDirection( fDirection );
				DWORD dwNormal = Vec3ToDWORD( V3_AXIS_Z );
				CQuat qObjectSceneRotation = CQuat( fObjectSceneDirection, V3_AXIS_Z );
				if ( NeedMakeOrientation() )
				{
					dwNormal = EditorScene()->GetNormal( CVec2( vObjectScenePosition.x, vObjectScenePosition.y ) );
					MakeOrientation( &qObjectSceneRotation, DWORDToVec3( dwNormal ) );
				}
				const UINT nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
				if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
				{
					pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
					pUpdate->info.nObjUniqueID = nSceneID;
					pUpdate->info.pStats = NDb::Get<NDb::SHPObjectRPGStats>( itMapInfoElement->second.rpgStatsDBID );
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
					pUpdate->info.fHitPoints = itMapInfoElement->second.fHP * pHPObjectRPGStats->fMaxHP;
					pUpdate->info.fFuel = 0.0f;
					pUpdate->info.eDipl = EDI_FRIEND;
					pUpdate->info.nPlayer = itMapInfoElement->second.nPlayer;
					pUpdate->info.nFrameIndex = itMapInfoElement->second.nFrameIndex;
					pUpdate->info.nExpLevel = 0;
					//
					pObjectInfoCollector->pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
				}		
				SObjectInfo::SSceneElement sceneElement;
				sceneElement.vPosition = itMapInfoElement->second.GetPosition( VNULL3 );
				sceneElement.fDirection = itMapInfoElement->second.GetDirection( 0.0f );
				// заносим элемент в структуру данных объекта
				sceneElementMap[nSceneID] = sceneElement;
				sceneIDToLinkIDMap[nSceneID] = itMapInfoElement->first;
				//
				if ( bUpdateParentStructure )
				{
					pObjectInfoCollector->sceneIDMap[nSceneID] = nObjectInfoID;
				}
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::SetSceneObjectOpacity( IEditorScene *pEditorScene, const float fOpacity )
	{
		list<int> sceneIDlist;
		for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
		{
			sceneIDlist.push_back( itSceneElement->first );
		}
		if ( !sceneIDlist.empty() )
		{
			pEditorScene->SetFadedObjects( sceneIDlist, NMapInfoEditor::SCENE_FADE_COEFFICIENT );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::Move( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition,
													bool bMoveLinkedObjects,
													bool bUpdateScene, IEditorScene *pEditorScene,
													bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectEditInfo != 0, "SObjectEditInfo::Move(), pObjectEditInfo == 0" );
		//
		bool bResult = true;
		if ( bMoveLinkedObjects )
		{
			if ( !MoveLinkedObjects( pObjectEditInfo, rvNewPosition,
															 bUpdateScene, pEditorScene,
															 bUpdateDB, pObjectController, pManipulator ) )
			{
				bResult = false;
			}
		}
		vPosition = rvNewPosition;
		if ( KeepZeroHeight() )
		{
			vPosition.z = 0.0f;
		}
		//
		ClearAdditionalPosition( false );
		//
		if ( NeedProcessEditParameters() )
		{
			if ( pObjectEditInfo->bFitToGrid )
			{
				FitToGrid( false );
			}
			// Устанавливаем общую высоту
			if ( KeepCommonHeight() )
			{
				SetCommonHeight( false );
			}
		}
		//
		FixInvalidPos( false );
		//
		UpdateSceneElements( false, true, false, 0.0f );
		//
		if ( bUpdateScene )
		{
			if ( !UpdateScene( pEditorScene ) )
			{
				bResult = false;
			}
		}
		if ( bUpdateDB )
		{
			if ( !UpdateDB( false, pObjectController, pManipulator ) )
			{
				bResult = false;
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::Rotate( const SObjectEditInfo *pObjectEditInfo, float fNewDirection,
														bool bRotateLinkedObjects,
														bool bUpdateScene, IEditorScene *pEditorScene,
														bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectEditInfo != 0, "SObjectEditInfo::Move(), pObjectEditInfo == 0" );
		//
		bool bResult = true;
		if ( bRotateLinkedObjects )
		{
			if ( !RotateLinkedObjects( pObjectEditInfo, fNewDirection,
																 bUpdateScene, pEditorScene,
																 bUpdateDB, pObjectController, pManipulator ) )
			{
				bResult = false;
			}
		}
		const float fAdditionalDirection = fNewDirection -  fDirection;
		fDirection = fNewDirection;
		//
		ClearAdditionalPosition( false );
		ClearAdditionalDirection( false );
		// Перемешаем объекты
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			RotatePoint( &( itMapInfoElement->second.vPosition ), fAdditionalDirection, VNULL3 );
		}
		//
		if ( NeedProcessEditParameters() )
		{
			if ( pObjectEditInfo->bRotateTo90Degree )
			{
				RotateTo90Degree( false );
			}
			//
			if ( pObjectEditInfo->bFitToGrid )
			{
				FitToGrid( false );
			}
			// Устанавливаем общую высоту
			if ( KeepCommonHeight() )
			{
				SetCommonHeight( false );
			}
		}
		//
		FixInvalidPos( false );
		//
		UpdateSceneElements( false, true, true, fAdditionalDirection );
		//
		if ( bUpdateScene )
		{
			if ( !UpdateScene( pEditorScene ) )
			{
				bResult = false;
			}
		}
		if ( bUpdateDB )
		{
			if ( !UpdateDB( false, pObjectController, pManipulator ) )
			{
				bResult = false;
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::UpdateScene( IEditorScene *pEditorScene )
	{
		if ( pEditorScene == 0 )
		{
			return false;
		}
		for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
		{
			CVec3 vObjectScenePosition = itSceneElement->second.GetPosition( vPosition );
			vObjectScenePosition.z += GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
			const float fObjectSceneDirection = itSceneElement->second.GetDirection( fDirection );
			DWORD dwNormal = Vec3ToDWORD( V3_AXIS_Z );
			CQuat qObjectSceneRotation = CQuat( fObjectSceneDirection, V3_AXIS_Z );
			if ( NeedMakeOrientation() )
			{
				dwNormal = EditorScene()->GetNormal( CVec2( vObjectScenePosition.x, vObjectScenePosition.y ) );
				MakeOrientation( &qObjectSceneRotation, DWORDToVec3( dwNormal ) );
			}
			if ( CPtr<SAIPlacementUpdate> pUpdate = new SAIPlacementUpdate() )
			{
				pUpdate->eUpdateType = ACTION_NOTIFY_PLACEMENT;
				pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
				pUpdate->info.nObjUniqueID = itSceneElement->first;
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
				pObjectInfoCollector->pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
			}
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::UpdateDB( bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( ( pObjectController == 0 ) || ( pManipulator == 0 ) )
		{
			return false;
		}
		bool bResult = true;
		if ( bUpdateLinkedObjects )
		{
			bResult = UpdateDBLinkedObjects( pObjectController, pManipulator );
		}
		if ( bResult )
		{
			for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
			{
				const int nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( itMapInfoElement->first );
				if ( nObjectIndex != INVALID_NODE_ID )
				{
					const string szObjectProperty = StrFmt( "Objects.[%d].", nObjectIndex );
					//Change
					const CVec3 vObjectDBPosition = itMapInfoElement->second.GetPosition( vPosition );
					const WORD wObjectDBDirection = Vis2AIRad( itMapInfoElement->second.GetDirection( fDirection ) );
					//
					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szObjectProperty + "Pos", vObjectDBPosition, pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( szObjectProperty + "Dir", wObjectDBDirection, pManipulator );
				}
				if ( !bResult )
				{
					break;
				}
			}
		}
		//
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::RemoveFromScene( IEditorScene *pEditorScene, bool bUpdateParentStructure )
	{
		if ( pEditorScene != 0 )
		{
			for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
			{
				if ( CPtr<SAIDissapearObjUpdate> pUpdate = new SAIDissapearObjUpdate() )
				{
					pUpdate->eUpdateType = ACTION_NOTIFY_DISSAPEAR_OBJ;
					pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
					pUpdate->nDissapearObjID = itSceneElement->first;
					pUpdate->bShowEffects = false;
					//
					pObjectInfoCollector->pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
				}
				if ( bUpdateParentStructure )
				{
					pObjectInfoCollector->sceneIDMap.erase( itSceneElement->first );
				}
				pObjectInfoCollector->sceneIDCollector.FreeID( itSceneElement->first );
			}
			sceneElementMap.clear();
			sceneIDToLinkIDMap.clear();
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SObjectInfo::RemoveFromDB(), pObjectInfoCollector == 0" );
		//
		if ( ( pObjectController == 0 ) || ( pManipulator == 0 ) )
		{
			return false;
		}
		// удаляем стартовые команды
		for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			int nCommandCount = 0;
			CManipulatorManager::GetValue( &nCommandCount, pManipulator, "startCommandsList" );
			for ( int nCommandIndex = 0; nCommandIndex < nCommandCount; )
			{
				bool bCommandRemoved = false;
				const string szCommandPrefix = StrFmt( "startCommandsList.[%d]", nCommandIndex );
				int nCommandLinkIDCount = 0;
				CManipulatorManager::GetValue( &nCommandLinkIDCount, pManipulator, szCommandPrefix + ".unitLinkIDs" );
				int nCommandLinkIDIndex = 0;
				while ( nCommandLinkIDIndex < nCommandLinkIDCount )
				{
					const string szCommandLinkIDPrefix = szCommandPrefix + StrFmt( ".unitLinkIDs.[%d]", nCommandLinkIDIndex );
					int nCommandLinkID = INVALID_NODE_ID;
					CManipulatorManager::GetValue( &nCommandLinkID, pManipulator, szCommandLinkIDPrefix );
					if ( nCommandLinkID == itMapInfoElement->first )
					{
						if ( nCommandLinkIDCount > 1 )
						{
							pObjectController->AddRemoveOperation( szCommandPrefix + ".unitLinkIDs", nCommandLinkIDIndex, pManipulator ); 			
							--nCommandLinkIDCount;
						}
						else
						{
							pObjectController->AddRemoveOperation( "startCommandsList", nCommandIndex, pManipulator );
							bCommandRemoved = true;
							break;
						}
					}
					else
					{
						++nCommandLinkIDIndex;
					}
				}
				if ( bCommandRemoved )
				{
					--nCommandCount;
				}
				else
				{
					++nCommandIndex;
				}
			}
		}
		// коллекционируем индексы объектов для удаления:
		list<int> objectIndexList;
		for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const int nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( itMapInfoElement->first );
			if ( nObjectIndex != INVALID_NODE_ID )
			{
				objectIndexList.push_back( nObjectIndex );
			}
		}
		objectIndexList.sort();
		bool bResult = true;
		for ( list<int>::const_iterator itObjectIndex = objectIndexList.end(); itObjectIndex != objectIndexList.begin(); )
		{
			--itObjectIndex;
			bResult = bResult && pObjectController->AddRemoveOperation( "Objects", ( *itObjectIndex ), pManipulator );
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::Remove( bool bUpdateScene, IEditorScene *pEditorScene,
														bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SObjectInfo::RemoveFromMapInfo(), pObjectInfoCollector == 0" );
		if ( ( !bUpdateDB ) || ( pManipulator != 0 ) )
		{
			if ( bUpdateDB )
			{
				UpdateDBLinkedObjects( pObjectController, pManipulator );
			}
			//
			RemoveLinkTo( bUpdateDB, pObjectController, pManipulator );
			RemoveLinks( false, bUpdateDB, pObjectController, pManipulator );
			if ( bUpdateScene )
			{
				RemoveFromScene( pEditorScene, true );
			}
			if ( bUpdateDB )
			{
				RemoveFromDB( pObjectController, pManipulator );
			}
			//
			for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
			{
				pObjectInfoCollector->linkIDToIndexCollector.Remove( itMapInfoElement->first, true );
				pObjectInfoCollector->linkIDMap.erase( itMapInfoElement->first );
				pObjectInfoCollector->linkIDCollector.FreeID( itMapInfoElement->first );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::CheckLinkCapability( UINT nLinkToSceneID ) const
	{
		bool bLinkCapability = false;
		if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoBySceneID( nLinkToSceneID ) )
		{
			// Получаем RPGStatsID
			if ( const SMapInfoElement *pLinkToMapInfoElement = pLinkToObjectInfo->GetMapInfoElementBySceneID( nLinkToSceneID ) )
			{
				const string szLinkToObjectRPGStatsTypeName = pLinkToMapInfoElement->szRPGStatsTypeName;
				const CDBID linkToObjectRPGStatsDBID = pLinkToMapInfoElement->rpgStatsDBID;
				const UINT nLinkToFrameIndex = pLinkToMapInfoElement->nFrameIndex;
				if ( !szLinkToObjectRPGStatsTypeName.empty() && !linkToObjectRPGStatsDBID.IsEmpty() )
				{
					for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
					{
						const string szObjectRPGStatsTypeName = itMapInfoElement->second.szRPGStatsTypeName;
						const CDBID objectRPGStatsDBID = itMapInfoElement->second.rpgStatsDBID;
						const UINT nFrameIndex = itMapInfoElement->second.nFrameIndex;
						if ( !szObjectRPGStatsTypeName.empty() && !objectRPGStatsDBID.IsEmpty() )
						{
							if ( NMapInfoEditor::CheckLinkCapability( szObjectRPGStatsTypeName, objectRPGStatsDBID, nFrameIndex,
																												szLinkToObjectRPGStatsTypeName, linkToObjectRPGStatsDBID, nLinkToFrameIndex ) )
							{
								bLinkCapability = true;
								break;
							}
						}
					}
				}
			}
		}
		return bLinkCapability;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::InsertLink( bool bUpdateDB, UINT nLinkToSceneID, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		if ( CheckLinkCapability( nLinkToSceneID ) )
		{
			// удалим старый линк
			bResult = RemoveLinkTo( bUpdateDB, pObjectController, pManipulator );
			//
			if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoBySceneID( nLinkToSceneID ) )
			{
				UINT nLinkToLinkID = pLinkToObjectInfo->GetLinkIDBySceneID( nLinkToSceneID );
				if ( SMapInfoElement* pLinkToMapInfoElement = pLinkToObjectInfo->GetMapInfoElementByLinkID( nLinkToLinkID ) )
				{
					for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
					{
						DebugTrace( "SObjectInfo::InsertLink() insert link: %d -> %d", itMapInfoElement->first, nLinkToLinkID );
						bool bNotExists = true;
						for ( CLinkIDList::iterator itLinkedLinkID = pLinkToMapInfoElement->linkedLinkIDIist.begin(); itLinkedLinkID != pLinkToMapInfoElement->linkedLinkIDIist.end(); ++itLinkedLinkID )
						{
							if ( ( *itLinkedLinkID ) == itMapInfoElement->first )
							{
								bNotExists = false;
								break;
							}
						}
						if ( bNotExists )
						{
							pLinkToMapInfoElement->linkedLinkIDIist.push_back( itMapInfoElement->first );
						}
						//
						if ( bUpdateDB )
						{
							const int nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( itMapInfoElement->first );
							const string szObjectProperty = StrFmt( "Objects.[%d].", nObjectIndex );
							bResult = bResult && pObjectController->AddChangeValueOperation<UINT>( szObjectProperty + "Link.LinkWith", nLinkToLinkID, pManipulator );
						}
						itMapInfoElement->second.nLinkToLinkID = nLinkToLinkID;
					}
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::RemoveLinks( bool bUpdateLinkedObjects, bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( bUpdateLinkedObjects && bUpdateDB )
		{
			UpdateDBLinkedObjects( pObjectController, pManipulator );
		}
		//
		bool bResult = true;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			CLinkIDList tempLinkIDList = itMapInfoElement->second.linkedLinkIDIist;
			for ( CLinkIDList::const_iterator itLinkedLinkID = tempLinkIDList.begin(); itLinkedLinkID != tempLinkIDList.end(); ++itLinkedLinkID )
			{
				if ( SObjectInfo *pObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( *itLinkedLinkID ) )
				{
					pObjectInfo->RemoveLinkTo( bUpdateDB, pObjectController, pManipulator );
				}
			}
			itMapInfoElement->second.linkedLinkIDIist.clear();
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::RemoveLinkTo( bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( itMapInfoElement->second.nLinkToLinkID ) )
			{
				for ( SObjectInfo::CMapInfoElementMap::iterator itLinkToMapInfoElement = pLinkToObjectInfo->mapInfoElementMap.begin(); itLinkToMapInfoElement != pLinkToObjectInfo->mapInfoElementMap.end(); ++itLinkToMapInfoElement )
				{
					for ( CLinkIDList::iterator itLinkedLinkID = itLinkToMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itLinkToMapInfoElement->second.linkedLinkIDIist.end(); )
					{
						if ( ( *itLinkedLinkID ) == itMapInfoElement->first )
						{
							DebugTrace( "SObjectInfo::RemoveLinkTo() remove link: %d -> %d", itMapInfoElement->first, itLinkToMapInfoElement->first );
							itLinkedLinkID = itLinkToMapInfoElement->second.linkedLinkIDIist.erase( itLinkedLinkID );
						}
						else
						{
							++itLinkedLinkID;
						}
					}
				}
				// стираем из базы данных
				if ( bUpdateDB )
				{
					const int nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( itMapInfoElement->first );
					const string szObjectProperty = StrFmt( "Objects.[%d].", nObjectIndex );
					bResult = bResult && pObjectController->AddChangeValueOperation<int>( szObjectProperty + "Link.LinkWith", INVALID_NODE_ID, pManipulator );
					if ( !bResult )
					{
						break;
					}
				}
				// обнyляем ссылку
				itMapInfoElement->second.nLinkToLinkID = INVALID_NODE_ID;
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::UpdateByController( UINT nLinkID, UINT nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		if ( SMapInfoElement* pMapInfoElement = GetMapInfoElementByLinkID( nLinkID ) )
		{
			const int nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( nLinkID );
			const string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
			if ( ( nFlags & POSITION_CHANGED ) || ( nFlags & DIRECTION_CHANGED ) )
			{
				MakeAbsolute();
				CManipulatorManager::GetVec3<CVec3, float>( &( pMapInfoElement->vPosition ), pManipulator, szObjectPrefix + ".Pos" );
				WORD wDirection = 0;
				CManipulatorManager::GetValue( &wDirection, pManipulator, szObjectPrefix + ".Dir" );
				float fAdditionalDirection = AI2VisRad( wDirection ) - pMapInfoElement->fDirection;
				pMapInfoElement->fDirection = AI2VisRad( wDirection );
				MakeRelative();
				UpdateSceneElements( false, ( nFlags & POSITION_CHANGED ), ( nFlags & DIRECTION_CHANGED ), fAdditionalDirection );
			}
			if ( nFlags & PLAYER_CHANGED )
			{
				CManipulatorManager::GetValue( &( pMapInfoElement->nPlayer ), pManipulator, szObjectPrefix + ".Player" );
			}
			if ( nFlags & FRAME_INDEX_CHANGED )
			{
				CManipulatorManager::GetValue( &( pMapInfoElement->nFrameIndex ), pManipulator, szObjectPrefix + ".FrameIndex" );
			}
			if ( nFlags & LINK_CHANGED )
			{
				int nLinkToLinkID = INVALID_NODE_ID;
				CManipulatorManager::GetValue( &nLinkToLinkID, pManipulator, szObjectPrefix + ".Link.LinkWith" );
				if ( nLinkToLinkID == INVALID_NODE_ID )
				{
					// удаляем линк
					if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( pMapInfoElement->nLinkToLinkID ) )
					{
						for ( SObjectInfo::CMapInfoElementMap::iterator itLinkToMapInfoElement = pLinkToObjectInfo->mapInfoElementMap.begin(); itLinkToMapInfoElement != pLinkToObjectInfo->mapInfoElementMap.end(); ++itLinkToMapInfoElement )
						{
							for ( CLinkIDList::iterator itLinkedLinkID = itLinkToMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itLinkToMapInfoElement->second.linkedLinkIDIist.end(); )
							{
								if ( ( *itLinkedLinkID ) == nLinkID )
								{
									DebugTrace( "SObjectInfo::RemoveLinkTo() remove link: %d -> %d", nLinkID, itLinkToMapInfoElement->first );
									itLinkedLinkID = itLinkToMapInfoElement->second.linkedLinkIDIist.erase( itLinkedLinkID );
								}
								else
								{
									++itLinkedLinkID;
								}
							}
						}
					}
					pMapInfoElement->nLinkToLinkID = INVALID_NODE_ID;
				}
				else
				{
					// добавляем линк
					if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( nLinkToLinkID ) )
					{
						if ( SMapInfoElement* pLinkToMapInfoElement = pLinkToObjectInfo->GetMapInfoElementByLinkID( nLinkToLinkID ) )
						{
							DebugTrace( "SObjectInfo::InsertLink() insert link: %d -> %d", nLinkID, nLinkToLinkID );
							bool bNotExists = true;
							for ( CLinkIDList::iterator itLinkedLinkID = pLinkToMapInfoElement->linkedLinkIDIist.begin(); itLinkedLinkID != pLinkToMapInfoElement->linkedLinkIDIist.end(); ++itLinkedLinkID )
							{
								if ( ( *itLinkedLinkID ) == nLinkID )
								{
									bNotExists = false;
									break;
								}
							}
							if ( bNotExists )
							{
								pLinkToMapInfoElement->linkedLinkIDIist.push_back( nLinkID );
							}
							pMapInfoElement->nLinkToLinkID = nLinkToLinkID;
						}
					}
				}
			}
			if ( nFlags > 0 )
			{
				UpdateScene( pEditorScene );
			}
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::InsertMaskManipulators( CMultiManipulator *pPropertyManipulator, IManipulator *pManipulator )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			if ( CPtr<IManipulator> pObjectManipulator = new CMaskManipulator( "", pManipulator, CMaskManipulator::SMART_MODE ) )
			{
				CMaskManipulator* pMaskManipulator = dynamic_cast<CMaskManipulator*>( &( *pObjectManipulator ) );
				pMaskManipulator->AddName( "Dir",									false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Pos.x",								false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Pos.y",								false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Pos.z",								false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Player",							false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "ScriptID",						false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "HP",									false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "FrameIndex",					false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Link.LinkID",					false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Link.LinkWith",				false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Link.Intention",			false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "Object",							false, "", INVALID_NODE_ID, false );
				pMaskManipulator->AddName( "ConstructorProfile",	false, "", INVALID_NODE_ID, false );
				//
				string szMask;
				const int nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( itMapInfoElement->first );
				if ( nObjectIndex != INVALID_NODE_ID )
				{
					szMask = StrFmt( "Objects.[%d].", nObjectIndex );
				}
				//
				pMaskManipulator->SetMask( szMask );
				pPropertyManipulator->InsertManipulator( CDBID( szMask ), pMaskManipulator, false, false );  
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::CopySelf()
	{
		sceneElementMap.clear();
		sceneIDToLinkIDMap.clear();

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::PasteLinkIDList( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap )
	{
		if ( ( pNew2OldLinkIDMap == 0 ) || ( pOld2NewLinkIDMap == 0 ) )
		{
			return;
		}
		SObjectInfo::CMapInfoElementMap oldMapInfoElementMap = mapInfoElementMap;
		mapInfoElementMap.clear();
		for ( SObjectInfo::CMapInfoElementMap::const_iterator itOldMapInfoElement = oldMapInfoElementMap.begin(); itOldMapInfoElement != oldMapInfoElementMap.end(); ++itOldMapInfoElement )
		{
			UINT nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
			mapInfoElementMap[nLinkID] = itOldMapInfoElement->second;
			( *pNew2OldLinkIDMap )[nLinkID] = itOldMapInfoElement->first;
			( *pOld2NewLinkIDMap )[itOldMapInfoElement->first] = nLinkID;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfo::PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( ( pNew2OldLinkIDMap == 0 ) || ( pOld2NewLinkIDMap == 0 ) )
		{
			return false;
		}
		if ( mapInfoElementMap.empty() )
		{
			return false;
		}
		nObjectInfoID = pObjectInfoCollector->objectInfoIDCollector.LockID();
		pObjectInfoCollector->objectInfoMap[nObjectInfoID] = this;
		//
		bool bResult = true;
		// Добавляем объекты базы
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			int nObjectIndex = INVALID_NODE_ID;
			CManipulatorManager::GetValue( &nObjectIndex, pManipulator, "Objects" );
			if ( nObjectIndex == INVALID_NODE_ID )
			{
				return false;
			}
			SObjectInfo::SMapInfoElement &rMapInfoElement = itMapInfoElement->second;
			const nLinkID = itMapInfoElement->first;
			//
			for ( CLinkIDList::iterator itLinkedLinkID = rMapInfoElement.linkedLinkIDIist.begin(); itLinkedLinkID != rMapInfoElement.linkedLinkIDIist.end(); ++itLinkedLinkID )
			{
				if ( pOld2NewLinkIDMap->find( *itLinkedLinkID ) != pOld2NewLinkIDMap->end() )
				{
					( *itLinkedLinkID ) = ( *pOld2NewLinkIDMap )[*itLinkedLinkID];
				}
			}
			if ( rMapInfoElement.nLinkToLinkID != INVALID_NODE_ID )
			{
				if ( pOld2NewLinkIDMap->find( rMapInfoElement.nLinkToLinkID ) != pOld2NewLinkIDMap->end() )
				{
					rMapInfoElement.nLinkToLinkID = ( *pOld2NewLinkIDMap )[rMapInfoElement.nLinkToLinkID];
				}
				else
				{
					if ( SObjectInfo *pLinkToObjectInfo = pObjectInfoCollector->GetObjectInfoByLinkID( rMapInfoElement.nLinkToLinkID ) )
					{
						if ( SMapInfoElement* pLinkToMapInfoElement = pLinkToObjectInfo->GetMapInfoElementByLinkID( rMapInfoElement.nLinkToLinkID ) )
						{
							pLinkToMapInfoElement->linkedLinkIDIist.push_back( nLinkID );
						}
					}
				}
			}
			//
			const string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
			// Insert
			bResult = bResult && pObjectController->AddInsertOperation( "Objects", NODE_ADD_INDEX, pManipulator );
			//Change
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", rMapInfoElement.rpgStatsDBID.ToString(), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( rMapInfoElement.nFrameIndex ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( rMapInfoElement.nPlayer ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".HP", (int)( rMapInfoElement.fHP ), pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szObjectPrefix + ".Pos", rMapInfoElement.GetPosition( vPosition ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", Vis2AIRad( rMapInfoElement.GetDirection( fDirection ) ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nLinkID, pManipulator );
			if ( rMapInfoElement.nLinkToLinkID != INVALID_NODE_ID )
			{
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkWith", ( int )( rMapInfoElement.nLinkToLinkID ), pManipulator );
			}
			//
			if ( bResult )
			{
				pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
				pObjectInfoCollector->linkIDToIndexCollector.Insert( nLinkID, nObjectIndex, false );
			}
		}
		CreateSceneObjects( pEditorScene, pManipulator, true );
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfo::Trace() const
	{
		DebugTrace( "ID: %d, pos: ( %g, %g, %g ), dir: %g",
								nObjectInfoID, vPosition.x, vPosition.y, vPosition.z, fDirection );
		//
		DebugTrace( "mapInfoElementMap, begin" );
		for ( CMapInfoElementMap::const_iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			DebugTrace( "ID: %d", itMapInfoElement->first );
			itMapInfoElement->second.Trace();
		}
		DebugTrace( "mapInfoElementMap, end" );
		//
		DebugTrace( "sceneElementMap, begin" );
		for ( CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
		{
			DebugTrace( "ID: %d", itSceneElement->first );
			itSceneElement->second.Trace();
		}
		DebugTrace( "sceneElementMap, end" );
		//
		DebugTrace( "sceneIDToLinkIDMap: begin" );
		for ( CSceneIDToLinkIDMap::const_iterator itSceneIDLinkID = sceneIDToLinkIDMap.begin(); itSceneIDLinkID != sceneIDToLinkIDMap.end(); ++itSceneIDLinkID )
		{
			DebugTrace( "sceneID: %d, mapInfoID: %d", itSceneIDLinkID->first, itSceneIDLinkID->second );
		}
		DebugTrace( "sceneIDToLinkIDMap: end" );
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
