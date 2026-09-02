#include "stdafx.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"
#include "MapEditorLib/Interface_View.h"
#include "BridgeInfoData.h"
#include "MapEditorLib/Interface_Logger.h"

#include <cstdint>

#include <zconf.h>

namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SBridgeInfo::Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SBridgeInfo::Load(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectLoadInfo != 0, "SBridgeInfo::Load(), pObjectLoadInfo == 0" );
		//
		// Устанавливаем общие параметры
		vPosition = VNULL3;
		fDirection = 0.0f;
		const std::string szBridgePrefix = StrFmt( "Bridges.[%d]", pObjectLoadInfo->nObjectIndex );
		nBridgeID = pObjectInfoCollector->bridgeIDCollector.LockID();
		if ( nBridgeID == INVALID_NODE_ID )
		{
			return false;
		}
		//
		std::vector<int> bridgeElementList;
		CManipulatorManager::GetArray<std::vector<int>, int>( &bridgeElementList,  pManipulator, szBridgePrefix + ".data" );
		if ( bridgeElementList.size() < 2 )
		{
			return false;
		}
		// Добавляем объекты базы
		bool bResult = true;
		for ( int nBridgeElementIndex = 0; nBridgeElementIndex < bridgeElementList.size(); ++nBridgeElementIndex )
		{
			// получаем расположенние объекта в базе данных
			const unsigned nLinkID = bridgeElementList[nBridgeElementIndex];
			const unsigned nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( nLinkID );
			if ( nObjectIndex == INVALID_NODE_ID )
			{
				NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Invalid index for LinkID in bridge: %d\n", nLinkID ) );
				continue;
			}
			const std::string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
			// создаем SMapInfoElement и заполняем его данными
			SObjectInfo::SMapInfoElement mapInfoElement;
			std::string szRPGStatsTypeName;
			std::string szRPGStatsName;
			bResult = bResult && CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0 );
			mapInfoElement.szRPGStatsTypeName = szRPGStatsTypeName;
			mapInfoElement.rpgStatsDBID = CDBID( szRPGStatsName );
			bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.nFrameIndex ), pManipulator, szObjectPrefix + ".FrameIndex" );
			bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.nPlayer ), pManipulator, szObjectPrefix + ".Player" );
			bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.fHP ), pManipulator, szObjectPrefix + ".HP" );
			bResult = bResult && CManipulatorManager::GetVec3<CVec3, float>( &( mapInfoElement.vPosition ), pManipulator, szObjectPrefix + ".Pos" );
			uint16_t wDirection = 0;
			bResult = bResult && CManipulatorManager::GetValue( &wDirection, pManipulator, szObjectPrefix + ".Dir" );
			mapInfoElement.fDirection = AI2VisRad( wDirection );
			//
			mapInfoElement.nLinkToLinkID = INVALID_NODE_ID;
			bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.nLinkToLinkID ), pManipulator, szObjectPrefix + ".Link.LinkWith" );
			if ( !bResult )
			{
				continue;
			}
			// заносим элемент в структуру данных объекта
			mapInfoElementMap[nLinkID] = mapInfoElement;
			pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
		}
		if ( !bResult )
		{
			return false;
		}
		// вычисляем относительные координаты и угол элементов объекта
		int nBridgeElementCount = 0;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			vPosition += itMapInfoElement->second.vPosition;
			if ( itMapInfoElement == mapInfoElementMap.begin() )
			{
				fDirection = itMapInfoElement->second.fDirection;
			}
			++nBridgeElementCount;
		}
		if ( nBridgeElementCount > 0 )
		{
			vPosition /= ( 1.0f * nBridgeElementCount );
			for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
			{
				itMapInfoElement->second.vPosition -= vPosition;
				itMapInfoElement->second.fDirection -= fDirection;
			}
		}
		// заполняем сцену и проставляем ссылки в mapInfo
		CreateSceneObjects( pEditorScene, pManipulator, true );
		pObjectInfoCollector->bridgeIDToIndexCollector.Insert( nBridgeID, pObjectLoadInfo->nObjectIndex, pObjectLoadInfo->bSearchIndices );
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SBridgeInfo::Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SBridgeInfo::Create(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectCreateInfo != 0, "SBridgeInfo::Create(), pObjectCreateInfo == 0" );
		//
		const SBridgeCreateInfo* pBridgeCreateInfo = checked_cast<const SBridgeCreateInfo*>( pObjectCreateInfo );
		if ( pBridgeCreateInfo == 0 )
		{
			return false;
		}
		const int nBridgeElementCount = pBridgeCreateInfo->centerPointList.size();
		if ( nBridgeElementCount < 2 )
		{
			return false;
		}
		const NDb::SBridgeRPGStats *pBridgeRPGStats = dynamic_cast<const NDb::SBridgeRPGStats*>( NDb::GetObject( pBridgeCreateInfo->rpgStatsDBID ) );
		NI_ASSERT( pBridgeRPGStats != 0, StrFmt( "%s not NDb::SBridgeRPGStats type", pBridgeCreateInfo->szRPGStatsTypeName.c_str() ) ); 
		if (  pBridgeRPGStats == 0 )
		{
			return false;
		}
		// Устанавливаем общие параметры
		vPosition = VNULL3;
		fDirection = pBridgeCreateInfo->fDirection;
		for ( std::list<CVec3>::const_iterator itBridgeElementCenterPoint = pBridgeCreateInfo->centerPointList.begin(); itBridgeElementCenterPoint != pBridgeCreateInfo->centerPointList.end(); ++itBridgeElementCenterPoint )
		{
			vPosition += *itBridgeElementCenterPoint;
		}
		vPosition /= ( 1.0f * nBridgeElementCount );
		// Добавляем объекты базы
		int nBridgeElementIndex = 0;
		std::list<int> bridgeLinkIDList; // для быстрой записи вектора в базу (секция мостов)
		for ( std::list<CVec3>::const_iterator itBridgeElementCenterPoint = pBridgeCreateInfo->centerPointList.begin(); itBridgeElementCenterPoint != pBridgeCreateInfo->centerPointList.end(); ++itBridgeElementCenterPoint )
		{
			int nObjectIndex = INVALID_NODE_ID;
			CManipulatorManager::GetValue( &nObjectIndex, pManipulator, "Objects" );
			if ( nObjectIndex == INVALID_NODE_ID )
			{
				continue;
			}
			// создаем SMapInfoElement и заполняем его данными
			SObjectInfo::SMapInfoElement mapInfoElement;
			mapInfoElement.szRPGStatsTypeName = pBridgeCreateInfo->szRPGStatsTypeName;
			mapInfoElement.rpgStatsDBID = pBridgeCreateInfo->rpgStatsDBID;
			mapInfoElement.nFrameIndex = 1;
			mapInfoElement.nPlayer = pBridgeCreateInfo->nPlayer;
			mapInfoElement.fHP = pBridgeCreateInfo->fHP;
			//
			mapInfoElement.vPosition = *itBridgeElementCenterPoint - vPosition;
			mapInfoElement.fDirection = 0.0f;
			mapInfoElement.linkedLinkIDIist.clear();
			mapInfoElement.nLinkToLinkID = INVALID_NODE_ID;
			//
			if ( nBridgeElementIndex == 0 )
			{
				mapInfoElement.nFrameIndex = 0;
			}
			else if ( nBridgeElementIndex == ( nBridgeElementCount - 1 ) )
			{
				mapInfoElement.nFrameIndex = 0;
				mapInfoElement.fDirection = ( pBridgeCreateInfo->fDirection >= FP_PI ) ? ( -FP_PI ) : FP_PI;
			}
			//
			mapInfoElement.FixInvalidPos( pObjectInfoCollector->mapSize, vPosition );
			//
			const std::string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
			// Insert
			bool bResult = pObjectController->AddInsertOperation( "Objects", NODE_ADD_INDEX, pManipulator );
			//Change
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szObjectPrefix + ".Pos", mapInfoElement.GetPosition( vPosition ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", Vis2AIRad( mapInfoElement.GetDirection( fDirection ) ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", mapInfoElement.rpgStatsDBID.ToString(), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( mapInfoElement.nFrameIndex ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( mapInfoElement.nPlayer ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".HP", (int)( mapInfoElement.fHP ), pManipulator );
			//
			if ( bResult )
			{
				const int nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nLinkID, pManipulator );
				bridgeLinkIDList.push_back( nLinkID );
				//
				pObjectInfoCollector->linkIDToIndexCollector.Insert( nLinkID, nObjectIndex, false );
				//
				mapInfoElementMap[nLinkID] = mapInfoElement;
				pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
			}
			++nBridgeElementIndex;
		}
		// заполняем сцену и проставляем ссылки в mapInfo
		CreateSceneObjects( pEditorScene, pManipulator, true );
		//Добавляем информацию в секцию мостов
		int nBridgeIndex = INVALID_NODE_ID;
		CManipulatorManager::GetValue( &nBridgeIndex, pManipulator, "Bridges" );
		const std::string szBridgePrefix = StrFmt( "Bridges.[%d]", nBridgeIndex );
		bool bResult = pObjectController->AddInsertOperation( "Bridges", NODE_ADD_INDEX, pManipulator );
		bResult = bResult && pObjectController->AddChangeArrayOperation<std::list<int>, int>( szBridgePrefix + ".data", bridgeLinkIDList, pManipulator );
		if ( bResult )
		{
			nBridgeID = pObjectInfoCollector->bridgeIDCollector.LockID();
			pObjectInfoCollector->bridgeIDToIndexCollector.Insert( nBridgeID, nBridgeIndex, false );
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SBridgeInfo::RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = SObjectInfo::RemoveFromDB( pObjectController, pManipulator );
		const unsigned nBridgeIndex = pObjectInfoCollector->bridgeIDToIndexCollector.Get( nBridgeID );
		if ( nBridgeIndex != INVALID_NODE_ID )
		{
			bResult = bResult && pObjectController->AddRemoveOperation( "Bridges", nBridgeIndex, pManipulator );
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SBridgeInfo::Remove(	bool bUpdateScene, IEditorScene *pEditorScene,
														bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		SObjectInfo::Remove( bUpdateScene, pEditorScene, bUpdateDB, pObjectController, pManipulator );
		pObjectInfoCollector->bridgeIDToIndexCollector.Remove( nBridgeID, true );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int SBridgeInfo::CreateCenterPoints( CBridgeCenterPointList *pResultList,
																			 CVec3 *pvDirection,
																			 const CVec3 &rvStart,
																			 const CVec3 &rvEnd,
																			 EDirection direction,
																			 float fEndSize,
																			 float fCenterSize,
																			 bool bFixStartPoint )
	{
		CVec3 vStart = VNULL3;
		CVec3 vEnd = VNULL3;
		switch ( direction )
		{
			case DIRECTION_X:
			{
				const fY = float ( rvStart.y + rvEnd.y ) / 2.0f;
				vStart = CVec3( rvStart.x, fY, rvStart.z );
				vEnd = CVec3( rvEnd.x, fY, rvEnd.z );
				break;
			}				
			case DIRECTION_Y:
			{
				const fX = float ( rvStart.x + rvEnd.x ) / 2.0f;
				vStart = CVec3( fX, rvStart.y, rvStart.z );
				vEnd = CVec3( fX, rvEnd.y, rvEnd.z );
				break;
			}				
			case DIRECTION_UNKNOWN:
			{
				const float fSizeX = abs( rvEnd.x - rvStart.x );
				const float fSizeY = abs( rvEnd.y - rvStart.y );
				if ( fSizeY > fSizeX )
				{
					const fX = float ( rvStart.x + rvEnd.x ) / 2.0f;
					vStart = CVec3( fX, rvStart.y, rvStart.z );
					vEnd = CVec3( fX, rvEnd.y, rvEnd.z );
				}
				else
				{
					const fY = float ( rvStart.y + rvEnd.y ) / 2.0f;
					vStart = CVec3( rvStart.x, fY, rvStart.z );
					vEnd = CVec3( rvEnd.x, fY, rvEnd.z );
				}
				break;
			}
			case DIRECTION_FREE:
			default:
				vStart = rvStart;
				vEnd = rvEnd;
				break;
		}
		//
		CVec3 vDirection = vEnd - vStart;
		const float fSize = fabs( vDirection.x, vDirection.y );
		if ( fSize < FP_EPSILON )
		{
			return 0;
		}
		vDirection = vDirection / fSize;
		//
		const float fMiddleSize = fSize - fEndSize;
		const unsigned nCenterCount = fMiddleSize > 0.0f ? unsigned( 0.5f + ( fMiddleSize / fCenterSize ) ) : 0;
		const float fBridgeSize = ( fEndSize * 2 ) + ( fCenterSize * nCenterCount );
		CVec3 vbridgeStart = VNULL3;
		if ( bFixStartPoint )
		{
			vbridgeStart = vStart - ( vDirection * ( fEndSize / 2.0f ) );
		}
		else
		{
			vbridgeStart = vStart + vDirection * ( ( fSize - fBridgeSize ) / 2.0f );
		}
		if ( pResultList )
		{
			pResultList->clear();
			pResultList->push_back( vbridgeStart + vDirection * ( fEndSize / 2.0f ) );
			for ( int nCenterIndex = 0; nCenterIndex < nCenterCount; ++nCenterIndex )
			{
				pResultList->push_back( vbridgeStart + vDirection * ( fEndSize + ( nCenterIndex * fCenterSize ) + ( fCenterSize / 2.0f ) ) );
			}
			pResultList->push_back( vbridgeStart + vDirection * ( fEndSize + ( nCenterCount * fCenterSize ) + ( fEndSize / 2.0f ) ) );
		}
		if ( pvDirection )
		{
			( *pvDirection ) = vDirection;
		}
		return 2 + nCenterCount;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const NDb::SModel* SBridgeInfo::GetModel( const NDb::SBridgeRPGStats *pBridgeRPGStats, EType type, NDb::ESeason eSeason )
	{
		if ( pBridgeRPGStats )
		{
			const NDb::SVisObj *pVisObj = 0;
			if ( type == TYPE_TERMINATOR )
			{
				if ( !pBridgeRPGStats->end.visualObjects.empty() )
				{
					pVisObj = pBridgeRPGStats->end.visualObjects[0];
				}
			}
			else
			{
				if ( !pBridgeRPGStats->center.visualObjects.empty() )
				{
					pVisObj = pBridgeRPGStats->center.visualObjects[0];
				}
			}
			if ( pVisObj != 0 )
			{
				return ::GetModel( pVisObj, eSeason ); 
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SBridgeInfo::PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( !SObjectInfo::PasteSelf( pNew2OldLinkIDMap, pOld2NewLinkIDMap, pEditorScene, pObjectController, pManipulator ) )
		{
			return false;
		}
		if ( ( pNew2OldLinkIDMap == 0 ) || ( pOld2NewLinkIDMap == 0 ) )
		{
			return false;
		}
		std::list<int> bridgeLinkIDList; // для быстрой записи вектора в базу (секция мостов)
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			bridgeLinkIDList.push_back( itMapInfoElement->first );
		}
		int nBridgeIndex = INVALID_NODE_ID;
		CManipulatorManager::GetValue( &nBridgeIndex, pManipulator, "Bridges" );
		const std::string szBridgePrefix = StrFmt( "Bridges.[%d]", nBridgeIndex );
		bool bResult = pObjectController->AddInsertOperation( "Bridges", NODE_ADD_INDEX, pManipulator );
		bResult = bResult && pObjectController->AddChangeArrayOperation<std::list<int>, int>( szBridgePrefix + ".data", bridgeLinkIDList, pManipulator );
		if ( bResult )
		{
			nBridgeID = pObjectInfoCollector->bridgeIDCollector.LockID();
			pObjectInfoCollector->bridgeIDToIndexCollector.Insert( nBridgeID, nBridgeIndex, false );
		}
		return false;
	}
};

// basement storage  


