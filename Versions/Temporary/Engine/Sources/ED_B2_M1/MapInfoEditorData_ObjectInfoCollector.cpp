#include "stdafx.h"
#include <fmt/format.h>
#include "ResourceDefines.h"
#include "CommandHandlerDefines.h"

#include "MapInfoEditorData_ObjectInfoCollector.h"
#include "MapInfoEditor.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include <cstdint>

namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SObjectInfoCollector::SObjectInfoCollector()
		:	linkIDToIndexCollector( INVALID_NODE_ID ),
			bridgeIDToIndexCollector( INVALID_NODE_ID ),
			trenchIDToIndexCollector( INVALID_NODE_ID ),
			spotIDToIndexCollector( INVALID_NODE_ID ),
			pPropertyManipulator( 0 ),
			mapSize( 0, 0, 0, 0 ),
			eSeason( NDb::SEASON_SUMMER ),
			eDayNight( NDb::DAY_DAY ),
			pMapInfoEditor( 0 ),
			pEditorUpdatableWorld( 0 ) {}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::Clear()
	{
		ClearSelection();
		ClearClipboard();
		//
		objectInfoMap.clear();
		linkIDMap.clear();
		sceneIDMap.clear();
		//
		linkIDToIndexCollector.Clear();
		bridgeIDToIndexCollector.Clear();
		trenchIDToIndexCollector.Clear();
		spotIDToIndexCollector.Clear();
		//
		objectInfoIDCollector.Clear();
		linkIDCollector.Clear();
		bridgeIDCollector.Clear();
		trenchIDCollector.Clear();
		sceneIDCollector.Clear();
		//
		pPropertyManipulator = 0;
		mapSize.minx = 0.0f;
		mapSize.miny = 0.0f;
		mapSize.maxx = 0.0f;
		mapSize.maxy = 0.0f;
		eSeason = NDb::SEASON_SUMMER;
		eDayNight = NDb::DAY_DAY;
		pMapInfoEditor = 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const NDb::SMapObjectInfo* SObjectInfoCollector::GetObjectStatusBarParams( int nSceneID )
	{
		CSceneIDMap::const_iterator itSceneID = sceneIDMap.find( nSceneID );
		if ( itSceneID != sceneIDMap.end() )
		{
			CObjectInfoMap::const_iterator posObject = objectInfoMap.find( itSceneID->second );
			if ( posObject != objectInfoMap.end() )
			{
				if ( SObjectInfo *pObjectInfo = posObject->second )
				{
					SObjectInfo::CSceneIDToLinkIDMap::const_iterator posLinkID = pObjectInfo->sceneIDToLinkIDMap.find( nSceneID );
					if ( posLinkID != pObjectInfo->sceneIDToLinkIDMap.end() )
					{
						SObjectInfo::CMapInfoElementMap::const_iterator posMapInfoElement = pObjectInfo->mapInfoElementMap.find( posLinkID->second );
						if ( posMapInfoElement != pObjectInfo->mapInfoElementMap.end() )
						{
							int nObjectIndex = linkIDToIndexCollector.Get( posMapInfoElement->first );
							if ( pMapInfoEditor &&
									 pMapInfoEditor->pMapInfo &&
									 ( nObjectIndex >= 0 ) &&
									 ( nObjectIndex < pMapInfoEditor->pMapInfo->objects.size() ) )
							{
								return &( pMapInfoEditor->pMapInfo->objects[nObjectIndex] );
							}
						}
					}
				}
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::SetMapInfoEditor( const CMapInfoEditor *_pMapInfoEditor )
	{
		pMapInfoEditor = _pMapInfoEditor;
		//
		mapSize.minx = 1.0f;
		mapSize.miny = 1.0f;
		mapSize.maxx = pMapInfoEditor->pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE - 1;
		mapSize.maxy = pMapInfoEditor->pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE - 1;
		//
		eSeason = pMapInfoEditor->pMapInfo->eSeason;
		eDayNight = pMapInfoEditor->pMapInfo->eDayTime;
		if ( pEditorUpdatableWorld = new CEditorUpdatableWorld() )
		{
			pEditorUpdatableWorld->SetSeason( eSeason, eDayNight );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	unsigned SObjectInfoCollector::GetLinkIDByObjectIndex( int nObjectIndex, IManipulator *pManipulator, bool bObject )
	{
		unsigned nLinkID = INVALID_NODE_ID;
		if ( pManipulator != 0 )
		{
			if ( bObject )
			{
				CManipulatorManager::GetValue( &nLinkID, pManipulator, fmt::format( "Objects.[{}].Link.LinkID", nObjectIndex ) );
			}
			else
			{
				CManipulatorManager::GetValue( &nLinkID, pManipulator, fmt::format( "Spots.[{}].SpotID", nObjectIndex ) );
			}
		}
		else
		{
			if ( bObject )
			{
				nLinkID = linkIDToIndexCollector.GetID( nObjectIndex );
			}
			else
			{
				nLinkID = spotIDToIndexCollector.GetID( nObjectIndex );
			}
		}
		return nLinkID;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::PostLoad( IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		bool bResult = true;
		for( CObjectInfoMap::iterator itObject = objectInfoMap.begin(); itObject != objectInfoMap.end(); ++itObject )
		{
			if  ( !itObject->second->PostLoad( pEditorScene, pManipulator ) )
			{
				bResult = false;
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::Pick( std::list<int> *pEditorSceneIDList, const CTPoint<int> &rMousePoint ) const
	{
		CVec3 vTerrainPos = VNULL3;
		Get2DPosOnMapHeights( &vTerrainPos, CVec2( rMousePoint.x, rMousePoint.y ) );
		//
		for( CObjectInfoMap::const_iterator itObject = objectInfoMap.begin(); itObject != objectInfoMap.end(); ++itObject )
		{
			if ( itObject->second->Pick( vTerrainPos ) )
			{
				NI_ASSERT( !itObject->second->sceneElementMap.empty(), "SObjectInfoCollector::Pick, sceneElementMap.empty()" );
				pEditorSceneIDList->push_back( itObject->second->sceneElementMap.begin()->first );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::Pick( std::list<int> *pEditorSceneIDList, const CTRect<int> &rFrame ) const
	{
		CSelectionSquare selectionSquare;
		CVec3 vTerrainPos = VNULL3;
		//
		Get2DPosOnMapHeights( &vTerrainPos, CVec2( rFrame.minx, rFrame.miny ) );
		selectionSquare.insert( selectionSquare.end(), vTerrainPos );
		Get2DPosOnMapHeights( &vTerrainPos, CVec2( rFrame.maxx, rFrame.miny ) );
		selectionSquare.insert( selectionSquare.end(), vTerrainPos );
		Get2DPosOnMapHeights( &vTerrainPos, CVec2( rFrame.maxx, rFrame.maxy ) );
		selectionSquare.insert( selectionSquare.end(), vTerrainPos );
		Get2DPosOnMapHeights( &vTerrainPos, CVec2( rFrame.minx, rFrame.maxy ) );
		selectionSquare.insert( selectionSquare.end(), vTerrainPos );
		//
		for( CObjectInfoMap::const_iterator itObject = objectInfoMap.begin(); itObject != objectInfoMap.end(); ++itObject )
		{
			if ( itObject->second->Pick( selectionSquare ) )
			{
				NI_ASSERT( !itObject->second->sceneElementMap.empty(), "SObjectInfoCollector::Pick, sceneElementMap.empty()" );
				pEditorSceneIDList->push_back( itObject->second->sceneElementMap.begin()->first );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	unsigned SObjectInfoCollector::Pick( unsigned nSceneID ) const
	{
		CSceneIDMap::const_iterator itSceneID = sceneIDMap.find( nSceneID );
		if ( itSceneID != sceneIDMap.end() )
		{
			CObjectInfoMap::const_iterator posObject = objectInfoMap.find( itSceneID->second );
			if ( posObject != objectInfoMap.end() )
			{
				return posObject->first;	
			}
		}
		return INVALID_NODE_ID;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SObjectInfo* SObjectInfoCollector::GetObjectInfo( const unsigned nObjectInfoID )
	{
		if ( nObjectInfoID != INVALID_NODE_ID )
		{
			CObjectInfoMap::iterator posObject = objectInfoMap.find( nObjectInfoID );
			if ( posObject != objectInfoMap.end() )
			{
				return posObject->second;
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SObjectInfo* SObjectInfoCollector::GetObjectInfoByLinkID( const unsigned nLinkID )
	{
		if ( nLinkID != INVALID_NODE_ID )
		{
			SObjectInfoCollector::CLinkIDMap::const_iterator posLinkID = linkIDMap.find( nLinkID );
			if ( posLinkID != linkIDMap.end() )
			{
				return GetObjectInfo( posLinkID->second );
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SObjectInfo* SObjectInfoCollector::GetObjectInfoBySceneID( const unsigned nSceneID )
	{
		if ( nSceneID != INVALID_NODE_ID )
		{
			SObjectInfoCollector::CSceneIDMap::const_iterator posSceneID = sceneIDMap.find( nSceneID );
			if ( posSceneID != sceneIDMap.end() )
			{
				return GetObjectInfo( posSceneID->second );
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::UpdateDB( unsigned nObjectInfoID, bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			return pObjectInfo->UpdateDB( bUpdateLinkedObjects, pObjectController, pManipulator );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::Draw( unsigned nObjectInfoID, CSceneDrawTool *pEditorSceneDrawTool )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			pObjectInfo->Draw( pEditorSceneDrawTool );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::Move( unsigned nObjectInfoID,
																	 const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition,
																	 bool bMoveLinkedObjects,
																	 bool bUpdateScene, IEditorScene *pEditorScene,
																	 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			pObjectInfo->Move( pObjectEditInfo, rvNewPosition,
												 bMoveLinkedObjects,
												 bUpdateScene, pEditorScene,
												 bUpdateDB, pObjectController, pManipulator );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::Rotate( unsigned nObjectInfoID,
																		 const SObjectEditInfo *pObjectEditInfo, float fNewDirection,
																		 bool bRotateLinkedObjects,
																		 bool bUpdateScene, IEditorScene *pEditorScene,
																		 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			pObjectInfo->Rotate( pObjectEditInfo, fNewDirection,
													 bRotateLinkedObjects,
													 bUpdateScene, pEditorScene,
													 bUpdateDB, pObjectController, pManipulator );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::CheckLinkCapability( unsigned nObjectInfoID, unsigned nLinkToSceneID )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			return pObjectInfo->CheckLinkCapability( nLinkToSceneID );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::InsertLink( unsigned nObjectInfoID, unsigned nLinkToSceneID, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			return pObjectInfo->InsertLink( true, nLinkToSceneID, pObjectController, pManipulator );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::RemoveLinks( unsigned nObjectInfoID, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			return pObjectInfo->RemoveLinks( true, true, pObjectController, pManipulator );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::RemoveLinkTo( unsigned nObjectInfoID, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
		{
			return pObjectInfo->RemoveLinkTo( true, pObjectController, pManipulator );
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const SObjectSelection::ESelectionType SObjectInfoCollector::PickSelection( const CVec3 &rvTerrainPos )
	{
		objectSelection.eSelectionType = SObjectSelection::ST_UNKNOWN;
		if ( !IsSelectionEmpty() )
		{
			const CVec3 vTerrainPosition = CVec3( rvTerrainPos.x, rvTerrainPos.y, 0.0f );
			const CVec3 vSelectionPosition = CVec3( objectSelection.vPosition.x, objectSelection.vPosition.y, 0.0f );
			float fSelectionDirection = objectSelection.fDirection + FP_PI2;
			if ( fSelectionDirection > FP_2PI )
			{ 
				fSelectionDirection -= FP_2PI;
			}
			//
			const CVec2 vDirection = CreateFromPolarCoord( ( SELECTION_RADIUS1 + SELECTION_RADIUS0 ) / 2.0f, fSelectionDirection );
			const CVec3 vSelectionDirection = CVec3( vDirection.x + vSelectionPosition.x, vDirection.y + vSelectionPosition.y, 0.0f );
			//
			objectSelection.eSelectionType = SObjectSelection::ST_OUT;
			objectSelection.vPositionDifference = objectSelection.vPosition - rvTerrainPos;
			if ( fabs( vSelectionPosition - vTerrainPosition ) <= SELECTION_POINT_RADIUS )
			{
				objectSelection.eSelectionType = SObjectSelection::ST_CENTER;
			}
			objectSelection.fDirectionDifference = GetPolarAngle( vSelectionDirection - vSelectionPosition ) - GetPolarAngle( vTerrainPosition - vSelectionPosition );
			if ( ( objectSelection.eSelectionType == SObjectSelection::ST_OUT ) &&
					 ( fabs( vSelectionDirection - vTerrainPosition ) <= SELECTION_POINT_RADIUS ) )
			{
				objectSelection.eSelectionType = SObjectSelection::ST_DIRECTION;
			}
		}
		return objectSelection.eSelectionType;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::GetLinkedObjectInfoIDList( CObjectInfoIDSet *pLinkedObjectInfoIDSet, const CObjectInfoIDSet &rObjectInfoIDSet, CObjectInfoIDSet *pAlreadyInUseObjectInfoIDSet )
	{
		CObjectInfoIDSet foundObjectInfoIDset;
		//
		for ( CObjectInfoIDSet::const_iterator itObjectInfoID = rObjectInfoIDSet.begin(); itObjectInfoID != rObjectInfoIDSet.end(); ++itObjectInfoID )
		{
			if ( const SObjectInfo *pObjectInfo = GetObjectInfo( itObjectInfoID->first ) )
			{
				for ( SObjectInfo::CMapInfoElementMap::const_iterator itMapInfoElement = pObjectInfo->mapInfoElementMap.begin(); itMapInfoElement != pObjectInfo->mapInfoElementMap.end(); ++itMapInfoElement )
				{
					for ( CLinkIDList::const_iterator itLinkedLinkID = itMapInfoElement->second.linkedLinkIDIist.begin(); itLinkedLinkID != itMapInfoElement->second.linkedLinkIDIist.end(); ++itLinkedLinkID )
					{
						SObjectInfoCollector::CLinkIDMap::const_iterator posLinkID = linkIDMap.find( *itLinkedLinkID );
						if ( posLinkID != linkIDMap.end() )
						{
							if ( pAlreadyInUseObjectInfoIDSet->find( posLinkID->second ) == pAlreadyInUseObjectInfoIDSet->end() )
							{
								InsertHashSetElement( &foundObjectInfoIDset, posLinkID->second );
								InsertHashSetElement( pAlreadyInUseObjectInfoIDSet, posLinkID->second );
							}
						}
					}
				}
			}
		}
		( *pLinkedObjectInfoIDSet ) = foundObjectInfoIDset;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::AddLinkedObjectsToSelection()
	{
		CObjectInfoIDSet selectionObjectInfoIDset;
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			InsertHashSetElement( &selectionObjectInfoIDset, itObjectSelectionPart->first );
		}
		CObjectInfoIDSet newObjectInfoIDSet = selectionObjectInfoIDset;
		while ( !newObjectInfoIDSet.empty() )
		{
			GetLinkedObjectInfoIDList( &newObjectInfoIDSet, newObjectInfoIDSet, &selectionObjectInfoIDset );
		}
		//
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			CObjectInfoIDSet::iterator posSelectionObjectInfoID = selectionObjectInfoIDset.find( itObjectSelectionPart->first );
			if ( posSelectionObjectInfoID != selectionObjectInfoIDset.end() )
			{
				selectionObjectInfoIDset.erase( posSelectionObjectInfoID );
			}
		}
		//
		if ( !selectionObjectInfoIDset.empty() )
		{
			// установим абсолютные координаты
			objectSelection.MakeAbsolute();
			//
			// добавим элементы
			for ( CObjectInfoIDSet::const_iterator itSelectionObjectInfoID = selectionObjectInfoIDset.begin(); itSelectionObjectInfoID != selectionObjectInfoIDset.end(); ++itSelectionObjectInfoID )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfo( itSelectionObjectInfoID->first ) )
				{
					SObjectSelectionPart objectSelectionPart;
					objectSelectionPart.vPosition = pObjectInfo->vPosition;
					objectSelectionPart.fDirection = pObjectInfo->fDirection;
					// заносим элемент в структуру данных объекта
					objectSelection.objectSelectionPartMap[itSelectionObjectInfoID->first] = objectSelectionPart;
				}
			}
			// установим относительные координаты
			objectSelection.MakeRelative();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::RollbackSelectionPosition( const SObjectEditInfo *pObjectEditInfo, IEditorScene *pEditorScene )
	{
		if ( objectSelection.RollbackPosition() )
		{
			for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
				{
					pObjectInfo->Move( pObjectEditInfo, objectSelection.vPosition + itObjectSelectionPart->second.vPosition,
														 false,
														 true, pEditorScene,
														 false, 0, 0 );
					pObjectInfo->Rotate( pObjectEditInfo, objectSelection.fDirection + itObjectSelectionPart->second.fDirection,
															 false,
															 true, pEditorScene,
															 false, 0, 0 );
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::RemoveSelection( IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		CWaitCursor waitCursor;
		HideSelectionPropertyManipulator();
		//
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			CObjectInfoMap::iterator posObject = objectInfoMap.find( itObjectSelectionPart->first );
			if ( posObject != objectInfoMap.end() )
			{
				posObject->second->Remove( true, pEditorScene, true, pObjectController, pManipulator );
				objectInfoMap.erase( posObject );
			}
		}
		ClearSelection();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::UpdateDBSelection( bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		if ( !IsSelectionEmpty() )
		{
			for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
				{
					if ( !pObjectInfo->UpdateDB( bUpdateLinkedObjects, pObjectController, pManipulator ) )
					{
						bResult = false;
					}
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::DrawSelection( CSceneDrawTool *pEditorSceneDrawTool )
	{
		if ( !IsSelectionEmpty() )
		{
			for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
				{
					pObjectInfo->Draw( pEditorSceneDrawTool );
				}
			}
			//
			CVec3 vSelectionPosition = objectSelection.vPosition;
			vSelectionPosition.z = GetTerrainHeight( vSelectionPosition.x, vSelectionPosition.y );
			float fSelectionDirection = objectSelection.fDirection + FP_PI2;
			if ( fSelectionDirection > FP_2PI )
			{ 
				fSelectionDirection -= FP_2PI;
			}
			//
			CVec2 vDirection = CreateFromPolarCoord( DIRECTION_RADIUS, fSelectionDirection );
			CVec3 vSelectionDirection = CVec3( vDirection.x + vSelectionPosition.x, vDirection.y + vSelectionPosition.y, vSelectionPosition.z );
			//
			pEditorSceneDrawTool->DrawCircle( vSelectionPosition, SELECTION_RADIUS0, SELECTION_PARTS, SELECTION_COLOR, false );
			pEditorSceneDrawTool->DrawCircle( vSelectionPosition, SELECTION_RADIUS1, SELECTION_PARTS, SELECTION_COLOR, false );
			pEditorSceneDrawTool->DrawLine( vSelectionPosition, vSelectionDirection, SELECTION_COLOR, false );
			//
			vDirection = CreateFromPolarCoord( ( SELECTION_RADIUS1 + SELECTION_RADIUS0 ) / 2.0f, fSelectionDirection );
			vSelectionDirection = CVec3( vDirection.x + vSelectionPosition.x, vDirection.y + vSelectionPosition.y, vSelectionPosition.z );
			pEditorSceneDrawTool->DrawCircle( vSelectionPosition, SELECTION_POINT_RADIUS, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
			pEditorSceneDrawTool->DrawCircle( vSelectionDirection, SELECTION_POINT_RADIUS, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
			if ( objectSelection.eSelectionType == SObjectSelection::ST_CENTER )
			{
				float fRadius = SELECTION_POINT_RADIUS;
				while( fRadius > 0.0f )
				{
					pEditorSceneDrawTool->DrawCircle( vSelectionPosition, fRadius, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
					fRadius -= 2.0f;
				}
			}
			else if ( objectSelection.eSelectionType == SObjectSelection::ST_DIRECTION )
			{
				float fRadius = SELECTION_POINT_RADIUS;
				while( fRadius > 0.0f )
				{
					pEditorSceneDrawTool->DrawCircle( vSelectionDirection, fRadius, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
					fRadius -= 2.0f;
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::ClearShootAreas()
	{
		EditorScene()->ClearMarkers( ESMT_SHOOT_AREA, -1 ); // clear ALL markers
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::DrawShootAreas( CSceneDrawTool *pEditorSceneDrawTool )
	{
		if ( !IsSelectionEmpty() )
		{
			for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfo(itObjectSelectionPart->first) )
				{
					CVec3 vPos = pObjectInfo->vPosition;
					for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = pObjectInfo->mapInfoElementMap.begin(); itMapInfoElement != pObjectInfo->mapInfoElementMap.end(); ++itMapInfoElement )
					{
						const uint32_t colors[] = { 0x0000ff00, 0x000000ff, 0x00ff0000, 0x0000ff00 };
						const float fObjectDirection = pObjectInfo->fDirection;
						//
						if ( itMapInfoElement->second.szRPGStatsTypeName == "MechUnitRPGStats" )
						{
							const uint32_t dwColor = colors[0];
							CVec3 vColor( (dwColor & 0x00ff0000) >> 16, (dwColor & 0x0000ff00) >> 8, dwColor & 0x000000ff );
							vColor /= 256.0f;
							//
							const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( NDb::GetObject( itMapInfoElement->second.rpgStatsDBID) );
							const int nID = -1;
							for ( int nPlatform = 0; nPlatform < pStats->GetPlatformsSize(nID); ++nPlatform )
							{
								const NDb::SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( nID, nPlatform );
								//
								float fStartAngle = fObjectDirection + platform.constraint.fMin + FP_PI2;
								float fEndAngle = fObjectDirection + platform.constraint.fMax + FP_PI2;
								if ( fEndAngle - fStartAngle >= FP_2PI )
									fStartAngle = fEndAngle = 0;
								//
								for ( int nGun = 0; nGun < pStats->GetGunsSize(nID, nPlatform); ++nGun )
								{
									const NDb::SMechUnitRPGStats::SMechUnitGun &gun = pStats->GetGun( nID, nPlatform, nGun );
									if ( !gun.pWeapon || !gun.bIsPrimary )
										continue;

									const float fRangeMin = gun.pWeapon->fRangeMin;
									const float fRangeMax = gun.pWeapon->fRangeMax;
									const CVec2 vPos2 = pStats->vAABBCenter;///
									//
									EditorScene()->AddShootArea( pObjectInfo->nObjectInfoID, fStartAngle, fEndAngle,
																							 fRangeMin * 32.0f, fRangeMax * 32.0f, vColor, CVec2(vPos.x, vPos.y) );
								}
							}
						}
						else if ( itMapInfoElement->second.szRPGStatsTypeName == "SquadRPGStats" )
						{
							const uint32_t dwColor = colors[2];
							CVec3 vColor( (dwColor & 0x00ff0000) >> 16, (dwColor & 0x0000ff00) >> 8, dwColor & 0x000000ff );
							vColor /= 256.0f;
							//
							const NDb::SSquadRPGStats *pStats = checked_cast<const NDb::SSquadRPGStats*>( NDb::GetObject( itMapInfoElement->second.rpgStatsDBID ) );
							const int nID = -1;
							{
								for ( int nMember = 0; nMember < pStats->members.size(); ++nMember )
								{
									const NDb::SInfantryRPGStats *pMember = pStats->members[nMember];
									for ( int nGun = 0; nGun < pMember->GetGunsSize(nID, nMember); ++nGun )
									{
										const NDb::SInfantryRPGStats::SInfantryGun &gun = pMember->GetGun( nID, nMember, nGun );
										if ( !gun.pWeapon )
											continue;

										const float fDirection = gun.fDirection + FP_PI * 0.5;
										const float fRangeMin = gun.pWeapon->fRangeMin;
										const float fRangeMax = gun.pWeapon->fRangeMax;
										//
										EditorScene()->AddShootArea( pObjectInfo->nObjectInfoID, fDirection, fDirection,
																								 fRangeMin * 32.0f, fRangeMax * 32.0f, vColor, CVec2(vPos.x, vPos.y) );
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::MoveSelection( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
																						bool bUpdateScene, IEditorScene *pEditorScene,
																						bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		CWaitCursor *pWaitCursor = 0;
		if ( bUpdateDB )
		{
			pWaitCursor = new CWaitCursor();
		}
		bool bResult = true;
		if ( bSetToZero )
		{
			objectSelection.vPosition.z = 0.0f;
		}
		else
		{
			if ( bIgnoreDifference )
			{
				objectSelection.vPositionDifference.x = 0.0f;
				objectSelection.vPositionDifference.y = 0.0f;
			}
			objectSelection.vPosition = rvNewPosition + objectSelection.vPositionDifference;
		}
		for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				if ( bSetToZero )
				{
					itObjectSelectionPart->second.vPosition.z = 0.0f;
				}
				else
				{
					if ( bExactPosition )
					{
						itObjectSelectionPart->second.vPosition.x = 0.0f;
						itObjectSelectionPart->second.vPosition.y = 0.0f;
					}
				}
				if( pObjectInfo->Move( pObjectEditInfo, objectSelection.vPosition + itObjectSelectionPart->second.vPosition,
															 false,
															 bUpdateScene, pEditorScene,
															 bUpdateDB, pObjectController, pManipulator ) )
				{
					bResult = false;
				}
			}
		}
		if ( pWaitCursor )
		{
			delete pWaitCursor;
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::RotateSelection(	const SObjectEditInfo *pObjectEditInfo, const float fNewDirection, bool bExactDirection, bool bIgnoreDifference,
																							bool bUpdateScene, IEditorScene *pEditorScene,
																							bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		CWaitCursor *pWaitCursor = 0;
		if ( bUpdateDB )
		{
			pWaitCursor = new CWaitCursor();
		}
		bool bResult = true;
		const float fOldDirection = objectSelection.fDirection;
		if ( bIgnoreDifference )
		{
			objectSelection.fDirectionDifference = 0.0f;
		}
		objectSelection.fDirection = fNewDirection + objectSelection.fDirectionDifference;
		for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				if ( bExactDirection )
				{
					itObjectSelectionPart->second.fDirection = 0.0f;
				}
				CVec3 vNewPosition = itObjectSelectionPart->second.vPosition;
				RotatePoint( &vNewPosition, objectSelection.fDirection - fOldDirection, VNULL3 );
				if( pObjectInfo->Move( pObjectEditInfo, pObjectInfo->vPosition + ( vNewPosition - itObjectSelectionPart->second.vPosition ),
															 false,
															 bUpdateScene, pEditorScene,
															 bUpdateDB, pObjectController, pManipulator ) )
				{
					bResult = false;
				}
				itObjectSelectionPart->second.vPosition = vNewPosition;
				if( pObjectInfo->Rotate( pObjectEditInfo, objectSelection.fDirection + itObjectSelectionPart->second.fDirection,
																 false,
																 bUpdateScene, pEditorScene,
																 bUpdateDB, pObjectController, pManipulator ) )
				{
					bResult = false;
				}
			}
		}
		if ( pWaitCursor )
		{
			delete pWaitCursor;
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::CheckSelectionLinkCapability( unsigned nLinkToSceneID )
	{
		bool bResult = false;
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				if ( pObjectInfo->CheckLinkCapability( nLinkToSceneID ) )
				{
					bResult = true;
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::InsertSelectionLink( unsigned nLinkToSceneID, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				if ( !pObjectInfo->InsertLink( true, nLinkToSceneID, pObjectController, pManipulator ) )
				{
					bResult = false;
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::RemoveSelectionLinks( CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				if ( !pObjectInfo->RemoveLinks( true, true, pObjectController, pManipulator ) )
				{
					bResult = false;
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::RemoveSelectionLinkTo( CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = true;
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				if ( !pObjectInfo->RemoveLinkTo( true, pObjectController, pManipulator ) )
				{
					bResult = false;
				}
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::ShowSelectionPropertyManipulator( IManipulator *pManipulator, const SObjectSet &rObjectSet )
	{
		NHPTimer::STime totalTime = 0;
		NHPTimer::GetTime( &totalTime );

		bool bCreateTree = true;
		
		CPtr<IManipulator> packupManipulator = pPropertyManipulator;
		if ( pPropertyManipulator = new CMultiManipulator() )
		{
			//
			for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
				{
					pObjectInfo->InsertMaskManipulators( dynamic_cast<CMultiManipulator*>( &( *pPropertyManipulator ) ), pManipulator );
				}
			}
			if ( dynamic_cast<CMultiManipulator*>( &( *pPropertyManipulator ) )->IsEmpty() )
			{
				pPropertyManipulator = packupManipulator;
				if ( pPropertyManipulator == 0 )
				{
					return;
				}
			}
			IView *pView = 0;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uintptr_t>( &pView ) );
			if ( pView != 0 )
			{
				pView->SetViewManipulator( pPropertyManipulator, rObjectSet, std::string() );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_VIEW, ID_VIEW_SHOW_PROPERTY_BROWSER, 1 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, bCreateTree ? ID_PC_DIALOG_CREATE_TREE : ID_PC_DIALOG_UPDATE_VALUES, 0 );
			if ( bCreateTree )
			{
				ICommandHandler *pCommandHandler = 0;
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_COMMAND_HANDLER, reinterpret_cast<uintptr_t>( &pCommandHandler ) );
				if ( pCommandHandler != 0 )
				{
					pCommandHandler->HandleCommand( ID_PC_EXPAND_ALL, 0 );
				}
			}
		}
		DebugTrace( "ShowSelectionPropertyManipulator(): total: %g", NHPTimer::GetTimePassed( &totalTime ) );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::HideSelectionPropertyManipulator()
	{
		if ( pPropertyManipulator != 0 )
		{
			IView *pView = 0;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uintptr_t>( &pView ) );
			if ( pView != 0 )
			{
				pView->RemoveViewManipulator();
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
			pPropertyManipulator = 0;	
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::UpdateObjectByController( int nObjectIndex, unsigned nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		unsigned nLinkID = GetLinkIDByObjectIndex( nObjectIndex, pManipulator, true );
		if ( SObjectInfo *pObjectInfo = GetObjectInfoByLinkID( nLinkID ) )
		{
			return pObjectInfo->UpdateByController( nLinkID, nFlags, pEditorScene, pManipulator );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::UpdateSpotByController( int nSpotIndex, unsigned nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		unsigned nLinkID = GetLinkIDByObjectIndex( nSpotIndex, pManipulator, false );
		if ( SObjectInfo *pObjectInfo = GetObjectInfoByLinkID( nLinkID ) )
		{
			return pObjectInfo->UpdateByController( nLinkID, nFlags, pEditorScene, pManipulator );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::PostLoadByController( const CIndicesList &rIndices, IEditorScene *pEditorScene, IManipulator *pManipulator, bool bObject )
	{
		for( CIndicesList::const_iterator itIndex = rIndices.begin(); itIndex != rIndices.end(); ++itIndex )
		{
			unsigned nLinkID = GetLinkIDByObjectIndex( *itIndex, pManipulator, bObject );
			if ( nLinkID != INVALID_NODE_ID )
			{
				if ( SObjectInfo *pObjectInfo = GetObjectInfoByLinkID( nLinkID ) )
				{
					pObjectInfo->PostLoad( pEditorScene, pManipulator );
				}
			}
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::UpdateSelectionByController()
	{
		objectSelection.MakeAbsolute();
		for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				itObjectSelectionPart->second.vPosition = pObjectInfo->vPosition;
				itObjectSelectionPart->second.fDirection = pObjectInfo->fDirection;
				++itObjectSelectionPart;
			}
			else
			{
				objectSelection.objectSelectionPartMap.erase( itObjectSelectionPart++ );
			}
		}
		objectSelection.MakeRelative();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::ClearClipboard()
	{ 
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboard.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboard.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				objectInfoIDCollector.FreeID( ( *itObjectClipboardPart )->nObjectInfoID );
			}
		}
		objectClipboard.Clear();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int SObjectInfoCollector::CopyClipboard()
	{
		ClearClipboard();
		for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelection.objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelection.objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			if ( SObjectInfo *pObjectInfo = GetObjectInfo( itObjectSelectionPart->first ) )
			{
				objectClipboard.Insert( pObjectInfo );
			}
		}
		objectClipboard.MakeRelative();
		//
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboard.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboard.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->nObjectInfoID = objectInfoIDCollector.LockID();
			}
		}
		//
		return objectClipboard.Size();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectInfoCollector::PasteClipboard( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvPastePosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
																						 bool bUpdateScene, IEditorScene *pEditorScene,
																						 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		//убрать полупрозрачное изображение
		HideClipboard( pEditorScene, pManipulator );

		//поправить координаты объектов (сделать абсолютными)
		objectClipboard.vPosition = rvPastePosition;
		objectClipboard.MakeAbsolute();

		// копируем
		SObjectClipboard objectClipboardToPaste;
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboard.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboard.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				objectClipboardToPaste.Insert( ( *itObjectClipboardPart ) );
			}
		}

		//поправить координаты объектов (сделать относительными)
		objectClipboard.MakeRelative();

		//собрать старые LinkID, выдаем новые
		CLinkIDMap new2OldLinkIDMap;
		CLinkIDMap old2NewLinkIDMap;
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboardToPaste.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboardToPaste.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->PasteLinkIDList( &new2OldLinkIDMap, &old2NewLinkIDMap );
			}
		}

		//добавить объекты в структуры базового класса
		bool bResult = true;
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboardToPaste.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboardToPaste.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				bResult = bResult && ( *itObjectClipboardPart )->PasteSelf( &new2OldLinkIDMap, &old2NewLinkIDMap, pEditorScene, pObjectController, pManipulator );
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::ShowClipboard( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvPastePosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
																						IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		HideClipboard( pEditorScene, pManipulator );
		//
		objectClipboard.vPosition = rvPastePosition;
		objectClipboard.MakeAbsolute();
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboard.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboard.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->CreateSceneObjects( pEditorScene, pManipulator, false );
				( *itObjectClipboardPart )->Move( pObjectEditInfo, ( *itObjectClipboardPart )->vPosition,
																					false,
																					true, pEditorScene,
																					false, 0, pManipulator );
				( *itObjectClipboardPart )->SetSceneObjectOpacity( pEditorScene, SCENE_PASTE_OPACITY );
			}
		}
		objectClipboard.MakeRelative();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::MoveClipboard( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvPastePosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
																						IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		objectClipboard.vPosition = rvPastePosition;
		objectClipboard.MakeAbsolute();
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboard.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboard.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->Move( pObjectEditInfo, ( *itObjectClipboardPart )->vPosition,
																					false,
																					true, pEditorScene,
																					false, 0, pManipulator );
			}
		}
		objectClipboard.MakeRelative();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::HideClipboard( IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboard.objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboard.objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->RemoveFromScene( pEditorScene, false );
				( *itObjectClipboardPart )->sceneElementMap.clear();
				( *itObjectClipboardPart )->sceneIDToLinkIDMap.clear();
			}
		}
		objectClipboard.MakeRelative();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectInfoCollector::Trace()
	{
		DebugTrace( "" );
		DebugTrace( "SObjectInfoCollector::Trace(): begin" );
		//
		DebugTrace( "objectInfoMap, begin" );
		for ( CObjectInfoMap::const_iterator itObject = objectInfoMap.begin(); itObject != objectInfoMap.end(); ++itObject )
		{
			itObject->second->Trace();
		}
		DebugTrace( "objectInfoMap, end" );
		//
		DebugTrace( "linkIDMap, begin" );
		for ( CLinkIDMap::const_iterator itMapInfoLink = linkIDMap.begin(); itMapInfoLink != linkIDMap.end(); ++itMapInfoLink )
		{
			DebugTrace( "mapInfoLinkID: %d, objectInfoID: %d", itMapInfoLink->first, itMapInfoLink->second );
		}
		DebugTrace( "linkIDMap, end" );
		//
		DebugTrace( "sceneIDMap, begin" );
		for ( CSceneIDMap::const_iterator itScene = sceneIDMap.begin(); itScene != sceneIDMap.end(); ++itScene )
		{
			DebugTrace( "SceneID: %d, objectInfoID: %d", itScene->first, itScene->second );
		}
		DebugTrace( "sceneIDMap, end" );
		//
		DebugTrace( "linkIDToIndexCollector" );
		linkIDToIndexCollector.Trace();
		//
		DebugTrace( "bridgeIDToIndexCollector" );
		bridgeIDToIndexCollector.Trace();
		//
		DebugTrace( "trenchIDToIndexCollector" );
		trenchIDToIndexCollector.Trace();
		//
		DebugTrace( "spotIDToIndexCollector" );
		spotIDToIndexCollector.Trace();
		//
		DebugTrace( "objectSelection" );
		objectSelection.Trace();
		//
		DebugTrace( "objectClipboard" );
		objectClipboard.Trace();
		//
		DebugTrace( "ObjectInfoIDCollector" );
		CFreeIDCollector objectInfoIDCollector;
		DebugTrace( "LinkIDCollector" );
		CFreeIDCollector linkIDCollector;
		DebugTrace( "BridgeIDCollector" );
		CFreeIDCollector bridgeIDCollector;
		DebugTrace( "TrenchIDCollector" );
		CFreeIDCollector trenchIDCollector;
		DebugTrace( "SceneIDCollector" );
		CFreeIDCollector sceneIDCollector;
		//
		DebugTrace( "pPropertyManipulator: %p", static_cast<const void*>( pPropertyManipulator.GetPtr() ) );
		DebugTrace( "SObjectInfoCollector::Trace(): end" );
		DebugTrace( "" );
	}
};

// basement storage


