#include "StdAfx.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "../mapeditorlib/objectcontroller.h"

#include "SpotInfoData.h"
#include "MapInfoEditor.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::Pick( const CVec3 &rvPos )
	{
		const CVec2 vPos = CVec2( rvPos.x - vPosition.x - vAdditionalPosition.x, rvPos.y - vPosition.y - vAdditionalPosition.y );
		return ( ClassifyConvexPolygon( spotSquare, vPos ) != CP_OUTSIDE );
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::Pick( const CSelectionSquare &rSelectionSquare )
	{
		return ( ClassifyConvexPolygon( rSelectionSquare, vPosition + vAdditionalPosition ) != CP_OUTSIDE );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::Draw( CSceneDrawTool *pEditorSceneDrawTool )
	{
		if ( !pEditorSceneDrawTool )
		{
			return false;
		}
		//
		CVec3 vObjectScenePosition = vPosition + vAdditionalPosition;
		vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
		pEditorSceneDrawTool->DrawCircle( vObjectScenePosition, SCENE_OBJECT_SELECTION_RADIUS0, SCENE_OBJECT_SELECTION_PARTS, SCENE_OBJECT_SELECTION_COLOR, false );

		float fObjectSceneDirection = fDirection + FP_PI2;
		if ( fObjectSceneDirection > FP_2PI )
		{ 
			fObjectSceneDirection -= FP_2PI;
		}
		const CVec2 vDirection = CreateFromPolarCoord( SCENE_OBJECT_DIRECTION_RADIUS, fObjectSceneDirection );
		const CVec3 vObjectSceneDirection( vDirection.x + vObjectScenePosition.x, vDirection.y + vObjectScenePosition.y, vObjectScenePosition.z );
		pEditorSceneDrawTool->DrawLine( vObjectScenePosition, vObjectSceneDirection, SCENE_OBJECT_SELECTION_COLOR, false );
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::FixInvalidPos( bool bUpdateSceneElements )
	{
		if ( !sceneElementMap.empty() )
		{
			const CVec3 vPos = vPosition;
			CVec3 vNewPos = vPos;
			if ( pObjectInfoCollector->mapSize.maxx > 0.0f )
			{
				vNewPos.x = Clamp( vNewPos.x, pObjectInfoCollector->mapSize.minx, pObjectInfoCollector->mapSize.maxx );
			}
			if ( pObjectInfoCollector->mapSize.maxy > 0.0f )
			{
				vNewPos.y = Clamp( vNewPos.y, pObjectInfoCollector->mapSize.miny, pObjectInfoCollector->mapSize.maxy );
			}
			vAdditionalPosition = CVec3( vNewPos.x - vPos.x, vNewPos.y - vPos.y, 0.0f );
			sceneElementMap.begin()->second.vAdditionalPosition = vAdditionalPosition;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSpotInfo::Load(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectLoadInfo != 0, "SSpotInfo::Load(), pObjectLoadInfo == 0" );
		//
		const SSpotLoadInfo* pSpotLoadInfo = checked_cast<const SSpotLoadInfo*>( pObjectLoadInfo );
		if ( pSpotLoadInfo == 0 )
		{
			return false;
		}
		// Устанавливаем общие параметры
		const string szSpotPrefix = StrFmt( "Spots.[%d]", pSpotLoadInfo->nObjectIndex );
		// Добавляем объекты базы
		nLinkID = INVALID_NODE_ID;
		// создаем SMapInfoElement и заполняем его данными
		bool bResult = true;
		{
			if ( pSpotLoadInfo->bAdditionalDataFilled )
			{
				NI_ASSERT( ( pSpotLoadInfo->spotSquare.size() == 4 ), "SSpotInfo::Load(), pSpotLoadInfo->spotSquare.size() != 4" );
				szRPGStatsTypeName = pSpotLoadInfo->szRPGStatsTypeName;
				rpgStatsDBID = pSpotLoadInfo->rpgStatsDBID;
				nLinkID = pSpotLoadInfo->nLinkID;
				spotSquare = pSpotLoadInfo->spotSquare;
			}
			else
			{
				string szRPGStatsName;
				bResult = bResult && CManipulatorManager::GetParamsFromReference( szSpotPrefix + ".Descriptor", pManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0 );
				rpgStatsDBID = CDBID( szRPGStatsName );
				bResult = bResult && CManipulatorManager::GetValue( &nLinkID, pManipulator, szSpotPrefix + ".SpotID" );
				bResult = bResult && CManipulatorManager::GetArray<CSpotSquare, CVec2>( &spotSquare, pManipulator, szSpotPrefix + ".points" );
			}
			ClearAdditionalPosition( false );
			MakeRelativeSpotSquare( true );
			// Create dummy scene object and grab some sceneID
			UINT nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
			SObjectInfo::SSceneElement sceneElement;
			sceneElement.vPosition = VNULL3;
			sceneElement.vAdditionalPosition = vAdditionalPosition;
			sceneElement.fDirection = 0.0f;
			// заносим элемент в структуру данных объекта
			sceneElementMap[nSceneID] = sceneElement;
			pObjectInfoCollector->sceneIDMap[nSceneID] = nObjectInfoID;
			pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
			pObjectInfoCollector->spotIDToIndexCollector.Insert( nLinkID, pSpotLoadInfo->nObjectIndex, pSpotLoadInfo->bSearchIndices );
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSpotInfo::Create(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectCreateInfo != 0, "SSimpleObjectInfo::Create(), pObjectCreateInfo == 0" );
		//
		const SSpotCreateInfo *pSpotSreateInfo = checked_cast<const SSpotCreateInfo*>( pObjectCreateInfo );
		if ( pSpotSreateInfo == 0 )
		{
			return false;
		}
		//
		NI_ASSERT( ( pSpotSreateInfo->spotSquare.size() == 4 ), "SSpotInfo::Create(), pSpotSreateInfo->spotSquare.size() != 4" );
		// Устанавливаем общие параметры
		szRPGStatsTypeName = pSpotSreateInfo->szRPGStatsTypeName;
		rpgStatsDBID = pSpotSreateInfo->rpgStatsDBID;
		spotSquare = pSpotSreateInfo->spotSquare;
		nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
		//
		// Добавляем объекты базы
		int nSpotIndex = INVALID_NODE_ID;
		CManipulatorManager::GetValue( &nSpotIndex, pManipulator, "Spots" );
		if ( nSpotIndex == INVALID_NODE_ID )
		{
			return false;
		}
		// создаем SMapInfoElements и заполняем их данными
		bool bResult = true;
		{
			const string szSpotPrefix = StrFmt( "Spots.[%d]", nSpotIndex );
			// Insert
			bResult = bResult && pObjectController->AddInsertOperation( "Spots", NODE_ADD_INDEX, pManipulator );
			//Change
			bResult = bResult && pObjectController->AddChangeOperation( szSpotPrefix + ".Descriptor", pSpotSreateInfo->rpgStatsDBID, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szSpotPrefix + ".SpotID", (int)( nLinkID ), pManipulator );
			for ( int nSpotPoint = 0; nSpotPoint < 4; ++nSpotPoint )
			{
				//bResult = bResult && pObjectController->AddInsertOperation( szSpotPrefix + ".points", NODE_ADD_INDEX, pManipulator );
				bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( szSpotPrefix + StrFmt( ".points.[%d]", nSpotPoint ), spotSquare[nSpotPoint], pManipulator );
			}
			if ( bResult )
			{
				//NDb::SMapInfo *pMutablMapInfo = const_cast<NDb::SMapInfo*>( pObjectInfoCollector->pMapInfoEditor->pMapInfo );
				//NI_ASSERT( pMutablMapInfo != 0, "SSpotInfo::Create(), pMutablMapInfo == 0" );
				//
				NDb::STerrainSpotInstance terrainSpotInstance;
				terrainSpotInstance.pDescriptor = dynamic_cast<const NDb::STerrainSpotDesc*>( NDb::GetObject( rpgStatsDBID ) );
				terrainSpotInstance.nSpotID = nLinkID;
				terrainSpotInstance.points = spotSquare;
				pEditorScene->GetTerraManager()->AddTerraSpot( &terrainSpotInstance );
				//vector<NDb::STerrainSpotInstance>::iterator itSpot = pMutablMapInfo->spots.insert( pMutablMapInfo->spots.end(), terrainSpotInfoInstance );
				//if ( !pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.empty() )
				//{
					//NI_ASSERT(  nSpotIndex == ( pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.size() - 1 ), "Invalid spot index creation" );
					//const NDb::STerrainSpotInstance *pTerrainSpotInstance = &( pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots[nSpotIndex] );
					//pEditorScene->GetTerrain()->AddTerraSpot( &( *itSpot ) );
				//}
				const_cast<CMapInfoEditor*>( pObjectInfoCollector->pMapInfoEditor )->SetModified( true );
				//
				ClearAdditionalPosition( false );
				MakeRelativeSpotSquare( true );
				// Create dummy scene object and grab some sceneID
				UINT nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
				SObjectInfo::SSceneElement sceneElement;
				sceneElement.vPosition = VNULL3;
				sceneElement.vAdditionalPosition = vAdditionalPosition;
				sceneElement.fDirection = 0.0f;
				// заносим элемент в структуру данных объекта
				sceneElementMap[nSceneID] = sceneElement;
				pObjectInfoCollector->sceneIDMap[nSceneID] = nObjectInfoID;
				pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
				pObjectInfoCollector->spotIDToIndexCollector.Insert( nLinkID, nSpotIndex, false );
			}
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::Move( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition,
												bool bMoveLinkedObjects,
												bool bUpdateScene, IEditorScene *pEditorScene,
												bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectEditInfo != 0, "SSpotInfo::Move(), pObjectEditInfo == 0" );
		//
		bool bResult = true;
		vPosition = rvNewPosition;
		vPosition.z = 0.0f;
		ClearAdditionalPosition( false );
		FixInvalidPos( false );
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
	bool SSpotInfo::Rotate( const SObjectEditInfo *pObjectEditInfo, float fNewDirection,
													bool bRotateLinkedObjects,
													bool bUpdateScene, IEditorScene *pEditorScene,
													bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectEditInfo != 0, "SSpotInfo::Move(), pObjectEditInfo == 0" );
		//
		bool bResult = true;
		RotatePoints( &spotSquare, fNewDirection - fDirection, VNULL2 );
		fDirection = fNewDirection;
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
	bool SSpotInfo::UpdateScene( IEditorScene *pEditorScene )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSpotInfo::UpdateScene(), pObjectInfoCollector == 0" );
		//NDb::SMapInfo *pMutablMapInfo = const_cast<NDb::SMapInfo*>( pObjectInfoCollector->pMapInfoEditor->pMapInfo );
		//NI_ASSERT( pMutablMapInfo != 0, "SSpotInfo::UpdateScene(), pMutablMapInfo == 0" );
		//
		//const int nSpotIndex = pObjectInfoCollector->spotIDToIndexCollector.Get( nLinkID );
		//if ( ( nSpotIndex >= 0 ) && ( nSpotIndex < pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.size() ) )
		{
			MakeAbsoluteSpotSquare();
			//
			/**
			NDb::STerrainSpotInstance terrainSpotInstance = pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots[nSpotIndex];
			NI_ASSERT( terrainSpotInstance.nSpotID == nLinkID, "SSpotInfo::UpdateScene(), itSpot->nSpotID != nLinkID" );
			terrainSpotInstance.points = spotSquare;
			pEditorScene->GetTerraManager()->RemoveTerraSpot( terrainSpotInstance.nSpotID );
			pEditorScene->GetTerraManager()->AddTerraSpot( &terrainSpotInstance );
			/**/
			NDb::STerrainSpotInstance terrainSpotInstance;
			terrainSpotInstance.pDescriptor = dynamic_cast<const NDb::STerrainSpotDesc*>( NDb::GetObject( rpgStatsDBID ) );
			terrainSpotInstance.nSpotID = nLinkID;
			terrainSpotInstance.points = spotSquare;
			pEditorScene->GetTerraManager()->RemoveTerraSpot( terrainSpotInstance.nSpotID );
			pEditorScene->GetTerraManager()->AddTerraSpot( &terrainSpotInstance );
			//
			MakeRelativeSpotSquare( false );
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::UpdateDB( bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSpotInfo::UpdateDB(), pObjectInfoCollector == 0" );
		//NDb::SMapInfo *pMutablMapInfo = const_cast<NDb::SMapInfo*>( pObjectInfoCollector->pMapInfoEditor->pMapInfo );
		//NI_ASSERT( pMutablMapInfo != 0, "SSpotInfo::UpdateScene(), pMutablMapInfo == 0" );
		//
		const int nSpotIndex = pObjectInfoCollector->spotIDToIndexCollector.Get( nLinkID );
		if ( ( nSpotIndex >= 0 ) && ( nSpotIndex < pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.size() ) )
		{
			MakeAbsoluteSpotSquare();
			//
			const string szSpotPrefix = StrFmt( "Spots.[%d]", nSpotIndex );
			bool bResult = true;
			for ( int nSpotPoint = 0; nSpotPoint < 4; ++nSpotPoint )
			{
				bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( szSpotPrefix + StrFmt( ".points.[%d]", nSpotPoint ), spotSquare[nSpotPoint], pManipulator );
			}
			//
			MakeRelativeSpotSquare( false );
			return bResult;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::Remove(	bool bUpdateScene, IEditorScene *pEditorScene,
													bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSpotInfo::Remove(), pObjectInfoCollector == 0" );
		//NDb::SMapInfo *pMutablMapInfo = const_cast<NDb::SMapInfo*>( pObjectInfoCollector->pMapInfoEditor->pMapInfo );
		//NI_ASSERT( pMutablMapInfo != 0, "SSpotInfo::UpdateScene(), pMutablMapInfo == 0" );
		//
		const int nSpotIndex = pObjectInfoCollector->spotIDToIndexCollector.Get( nLinkID );
		//if ( ( nSpotIndex >= 0 ) && ( nSpotIndex < pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.size() ) )
		if ( bUpdateScene )
		{
			//vector<NDb::STerrainSpotInstance>::iterator itSpot = pMutablMapInfo->spots.begin() + nSpotIndex;
			//const NDb::STerrainSpotInstance *pTerrainSpotInstance = &( pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots[nSpotIndex] );
			//NI_ASSERT( pTerrainSpotInstance->nSpotID == nLinkID, "SSpotInfo::UpdateScene(), pTerrainSpotInstance->nSpotID != nLinkID" );
			pEditorScene->GetTerraManager()->RemoveTerraSpot( nLinkID );
			//pMutablMapInfo->spots.erase( itSpot );
		}
		if ( bUpdateDB )
		{
			if ( ( nSpotIndex >= 0 ) && ( nSpotIndex < pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.size() ) )
			{
				pObjectController->AddRemoveOperation( "Spots", nSpotIndex, pManipulator );
			}
		}
		pObjectInfoCollector->linkIDMap.erase( nLinkID );
		pObjectInfoCollector->spotIDToIndexCollector.Remove( nLinkID, true );
		pObjectInfoCollector->linkIDCollector.FreeID( nLinkID );
		// remove dummy scene ID
		for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
		{
			pObjectInfoCollector->sceneIDMap.erase( itSceneElement->first );
			pObjectInfoCollector->sceneIDCollector.FreeID( itSceneElement->first );
		}
		const_cast<CMapInfoEditor*>( pObjectInfoCollector->pMapInfoEditor )->SetModified( true );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::UpdateByController( UINT nSpotID, UINT nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		const int nSpotIndex = pObjectInfoCollector->spotIDToIndexCollector.Get( nLinkID );
		if ( ( nSpotIndex != INVALID_NODE_ID ) && ( nFlags > 0 ) )
		{
			const string szSpotPrefix = StrFmt( "Spots.[%d]", nSpotIndex );
			CManipulatorManager::GetArray<CSpotSquare, CVec2>( &spotSquare, pManipulator, szSpotPrefix + ".points" );
			ClearAdditionalPosition( false );
			MakeRelativeSpotSquare( true );
			FixInvalidPos( false );
			UpdateScene( pEditorScene );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::CreateSceneObjects( IEditorScene *pEditorScene, IManipulator *pManipulator, bool bUpdateParentStructure )
	{
		/**
		MakeAbsoluteSpotSquare();
		//
		int nSpotIndex = pObjectInfoCollector->pMapInfoEditor->pMapInfo->spots.size();
		nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
		NDb::STerrainSpotInstance terrainSpotInstance;
		terrainSpotInstance.pDescriptor = dynamic_cast<const NDb::STerrainSpotDesc*>( NDb::GetObject( rpgStatsDBID ) );
		terrainSpotInstance.nSpotID = nLinkID;
		terrainSpotInstance.points = spotSquare;
		pEditorScene->GetTerraManager()->AddTerraSpot( &terrainSpotInstance );
		//
		ClearAdditionalPosition( false );
		MakeRelativeSpotSquare( true );

		UINT nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
		SObjectInfo::SSceneElement sceneElement;
		sceneElement.vPosition = VNULL3;
		sceneElement.vAdditionalPosition = vAdditionalPosition;
		sceneElement.fDirection = 0.0f;
		sceneElementMap[nSceneID] = sceneElement;
		/**/
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::RemoveFromScene( IEditorScene *pEditorScene, bool bUpdateParentStructure )
	{
		/**
		if ( nLinkID != INVALID_NODE_ID )
		{
			pEditorScene->GetTerraManager()->RemoveTerraSpot( nLinkID );
			pObjectInfoCollector->linkIDCollector.FreeID( nLinkID );
			// remove dummy scene ID
			for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = sceneElementMap.begin(); itSceneElement != sceneElementMap.end(); ++itSceneElement )
			{
				pObjectInfoCollector->sceneIDCollector.FreeID( itSceneElement->first );
			}
		}
		sceneElementMap.clear();
		/**/
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::CopySelf()
	{
		sceneElementMap.clear();
		sceneIDToLinkIDMap.clear();
		nLinkID = INVALID_NODE_ID;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SSpotInfo::PasteLinkIDList( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap )
	{
		if ( ( pNew2OldLinkIDMap == 0 ) || ( pOld2NewLinkIDMap == 0 ) )
		{
			return;
		}
		const UINT nOldLinkID = nLinkID;
		nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
		( *pNew2OldLinkIDMap )[nLinkID] = nOldLinkID;
		( *pOld2NewLinkIDMap )[nOldLinkID] = nLinkID;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSpotInfo::PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( ( pNew2OldLinkIDMap == 0 ) || ( pOld2NewLinkIDMap == 0 ) )
		{
			return false;
		}
		NI_ASSERT( pObjectInfoCollector != 0, "SSpotInfo::Create(), pObjectInfoCollector == 0" );
		//
		MakeAbsoluteSpotSquare();
		//
		// Добавляем объекты базы
		int nSpotIndex = INVALID_NODE_ID;
		CManipulatorManager::GetValue( &nSpotIndex, pManipulator, "Spots" );
		if ( nSpotIndex == INVALID_NODE_ID )
		{
			return false;
		}
		// создаем SMapInfoElements и заполняем их данными
		bool bResult = true;
		{
			const string szSpotPrefix = StrFmt( "Spots.[%d]", nSpotIndex );
			// Insert
			bResult = bResult && pObjectController->AddInsertOperation( "Spots", NODE_ADD_INDEX, pManipulator );
			//Change
			bResult = bResult && pObjectController->AddChangeOperation( szSpotPrefix + ".Descriptor", rpgStatsDBID, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szSpotPrefix + ".SpotID", (int)( nLinkID ), pManipulator );
			for ( int nSpotPoint = 0; nSpotPoint < 4; ++nSpotPoint )
			{
				bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( szSpotPrefix + StrFmt( ".points.[%d]", nSpotPoint ), spotSquare[nSpotPoint], pManipulator );
			}
			if ( bResult )
			{
				NDb::STerrainSpotInstance terrainSpotInstance;
				terrainSpotInstance.pDescriptor = dynamic_cast<const NDb::STerrainSpotDesc*>( NDb::GetObject( rpgStatsDBID ) );
				terrainSpotInstance.nSpotID = nLinkID;
				terrainSpotInstance.points = spotSquare;
				pEditorScene->GetTerraManager()->AddTerraSpot( &terrainSpotInstance );
				//
				const_cast<CMapInfoEditor*>( pObjectInfoCollector->pMapInfoEditor )->SetModified( true );
				//
				ClearAdditionalPosition( false );
				MakeRelativeSpotSquare( true );

				// Create dummy scene object and grab some sceneID
				UINT nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
				SObjectInfo::SSceneElement sceneElement;
				sceneElement.vPosition = VNULL3;
				sceneElement.vAdditionalPosition = vAdditionalPosition;
				sceneElement.fDirection = 0.0f;
				// заносим элемент в структуру данных объекта
				//
				nObjectInfoID = pObjectInfoCollector->objectInfoIDCollector.LockID();
				pObjectInfoCollector->objectInfoMap[nObjectInfoID] = this;
				//
				sceneElementMap[nSceneID] = sceneElement;
				pObjectInfoCollector->sceneIDMap[nSceneID] = nObjectInfoID;
				pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
				pObjectInfoCollector->spotIDToIndexCollector.Insert( nLinkID, nSpotIndex, false );
			}
		}
		return bResult;
	}
};


// basement storage  


