#include "stdafx.h"
#include <fmt/format.h>
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"
#include "libdb/ResourceManager.h"
#include "Main/GameTimer.h"
#include "EntrenchmentInfoData.h"
#include <float.h>

#include <cstdint>

#include <zconf.h>

namespace NMapInfoEditor 
{
	////////////////////////////////////////////////////////////////////t//////////////////////////////////////////////////////////
	const NDb::SModel* SEntrenchmentInfo::GetModel(	const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats, NDb::EEntrenchSegmType type, NDb::ESeason eSeason )
	{
		if ( !pEntrenchmentRPGStats )
		{
			return 0;
		}
		int nSegmentIndex = -1;
		switch ( type )
		{
			case NDb::EST_LINE: 
			{
				nSegmentIndex = pEntrenchmentRPGStats->GetLineIndex(); 
				break;
			}
			case NDb::EST_FIREPLACE: 
			{
				nSegmentIndex = pEntrenchmentRPGStats->GetFirePlaceIndex(); 
				break;
			}
			case NDb::EST_TERMINATOR: 
			{
				nSegmentIndex = pEntrenchmentRPGStats->GetTerminatorIndex(); 
				break;
			}
			case NDb::EST_ARC: 
			{
				nSegmentIndex = pEntrenchmentRPGStats->GetArcIndex(); 
				break;
			}
			default:
			{
				NI_ASSERT( 0, "WARNING: SEntrenchmentInfo::GetModel() - strange segment type" );
			}
		}
		if ( nSegmentIndex < 0 || nSegmentIndex >= pEntrenchmentRPGStats->segments.size() )
		{
			return 0;
		}
		const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats *pSeg = &pEntrenchmentRPGStats->segments[nSegmentIndex]; 
		const NDb::SVisObj *pVisObj = pSeg->pVisObj;
		if ( !pVisObj )
		{
			return 0;
		}
		else
		{
			return ::GetModel( pVisObj, eSeason );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats* SEntrenchmentInfo::GetSegmentInfo( const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats, int nFrameIndex )
	{
		if ( !pEntrenchmentRPGStats )
		{
			return 0;
		}
		NDb::EEntrenchSegmType eType = static_cast<NDb::EEntrenchSegmType>( nFrameIndex );
		for ( int nSegmentIndex = 0; nSegmentIndex < pEntrenchmentRPGStats->segments.size(); ++nSegmentIndex )
		{
			const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &rEntrenchSegmentRPGStats = pEntrenchmentRPGStats->segments[nSegmentIndex];
			if ( rEntrenchSegmentRPGStats.eType == eType )
			{
				return &rEntrenchSegmentRPGStats;
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SEntrenchmentInfo::GetEntrenchmentSegmentPositionOnTerrain( CVec3 *pPos,
																																	 CQuat *pQuat,
																																	 uint32_t *pdwNormal,
																																	 CVec3 *pScale,
																																	 const float fSegmentHalfLen,
																																	 const CVec3 &rvCenterPos,
																																	 const float fXYDirection )
	{
		CQuat qXYRot = CQuat( fXYDirection, V3_AXIS_Z );
		CVec3 vSegmentDir = VNULL3;
		qXYRot.Rotate( &vSegmentDir, V3_AXIS_X );
		//
		CVec3 vPos0 = rvCenterPos - vSegmentDir * fSegmentHalfLen;
		vPos0.z = GetTerrainHeight( vPos0.x, vPos0.y );
		CVec3 vPos1 = rvCenterPos + vSegmentDir * fSegmentHalfLen;
		vPos1.z = GetTerrainHeight( vPos1.x, vPos1.y );
		//
		*pPos = rvCenterPos;
		pPos->z = GetTerrainHeight( pPos->x, pPos->y );
		//
		CVec3 vTerraDir = vPos1 - vPos0;
		float fTerraLen = fabs( vTerraDir );
		//
		NI_ASSERT( fTerraLen > FLT_EPSILON, "GetEntrenchmentSegmentPositionOnTerrain(): fTerraLen <= FLT_EPSILON" ); 
		if ( !( fTerraLen > FLT_EPSILON ) )
		{
			return false;
		}
		//
		float fKScale = 0.5f * fTerraLen / fSegmentHalfLen;
		*pScale = CVec3( fKScale, 1.0f, 1.0f );
		//
		Normalize( &vTerraDir );
		float fVertAngle = atan2( vTerraDir.z, sqrt( sqr( vTerraDir.x ) + sqr( vTerraDir.y ) ) );
		//
		CVec3 vrAxis = VNULL3;
		qXYRot.Rotate( &vrAxis, -V3_AXIS_Y );
		CQuat qRotVert( fVertAngle, vrAxis ); 
		//
		*pQuat = qRotVert * qXYRot;
		//
	  ( *pdwNormal ) = EditorScene()->GetNormal( CVec2( pPos->x, pPos->y ) );
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SEntrenchmentInfo::Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SEntrenchmentInfo::Load(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectLoadInfo != 0, "SEntrenchmentInfo::Load(), pObjectLoadInfo == 0" );
		//
		segmentLinkIDListList.clear();
		// Устанавливаем общие параметры
		vPosition = VNULL3; // будет заполняться в CreateSceneObject()
		fDirection = 0.0f;
		const std::string szEntrenchmentPrefix = fmt::format( "Entrenchments.[{}]", pObjectLoadInfo->nObjectIndex );
		nEntrenchmentID = pObjectInfoCollector->trenchIDCollector.LockID();
		if ( nEntrenchmentID == INVALID_NODE_ID )
		{
			return false;
		}
		//
		int nSectionsNumber = 0;
		CManipulatorManager::GetValue( &nSectionsNumber, pManipulator, szEntrenchmentPrefix + ".sections" );
		bool bResult = true;
		for ( int nSectionIndex = 0; nSectionIndex < nSectionsNumber; ++nSectionIndex )
		{
			CSegmentLinkIDListList::iterator itSegmentLinkIDList = segmentLinkIDListList.insert( segmentLinkIDListList.end(), CSegmentLinkIDList() );
			CManipulatorManager::GetArray<CSegmentLinkIDList, int>( &( *itSegmentLinkIDList ), pManipulator, szEntrenchmentPrefix + fmt::format( ".sections.[{}].data", nSectionIndex ) );
			//
			// Добавляем объекты базы
			for ( CSegmentLinkIDList::const_iterator itSegmentLinkID = itSegmentLinkIDList->begin(); itSegmentLinkID != itSegmentLinkIDList->end(); ++itSegmentLinkID )
			{
				// получаем расположенние объекта в базе данных
				const unsigned nLinkID = *itSegmentLinkID;
				const unsigned nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( nLinkID );
				NI_ASSERT( nObjectIndex != INVALID_NODE_ID, fmt::format( "Invalid trench index for LinkID: {}", nLinkID ) );
				if ( nObjectIndex == INVALID_NODE_ID )
				{
					continue;
				}
				const std::string szObjectPrefix = fmt::format( "Objects.[{}]", nObjectIndex );
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
		}
		if ( !bResult )
		{
			return false;
		}
		else
		{
			CreateSceneObjects( pEditorScene, pManipulator, true );
		}
		pObjectInfoCollector->trenchIDToIndexCollector.Insert( nEntrenchmentID, pObjectLoadInfo->nObjectIndex, pObjectLoadInfo->bSearchIndices );
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SEntrenchmentInfo::Remove( bool bUpdateScene, IEditorScene *pEditorScene,
																	bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		CWaitCursor wcur;
		SObjectInfo::Remove( bUpdateScene, pEditorScene, bUpdateDB, pObjectController, pManipulator );
		pObjectInfoCollector->trenchIDToIndexCollector.Remove( nEntrenchmentID, true );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SEntrenchmentInfo::RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		bool bResult = SObjectInfo::RemoveFromDB( pObjectController, pManipulator );
		const unsigned nEntrenchmentIndex = pObjectInfoCollector->trenchIDToIndexCollector.Get( nEntrenchmentID );
		if ( nEntrenchmentIndex != INVALID_NODE_ID )
		{
			bResult = bResult && pObjectController->AddRemoveOperation( "Entrenchments", nEntrenchmentIndex, pManipulator );
		}
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SEntrenchmentInfo::CreateSceneObjects( IEditorScene *pEditorScene, IManipulator *pManipulator, bool bUpdateParentStructure )
	{
		MakeAbsolute();

		// вычисляем относительные координаты и угол элементов объекта
		int nEntrenchmentElementCount = 0;
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			vPosition += itMapInfoElement->second.vPosition;
			if ( itMapInfoElement == mapInfoElementMap.begin() )
			{
				fDirection = itMapInfoElement->second.fDirection;
			}
			++nEntrenchmentElementCount;
		}
		if ( nEntrenchmentElementCount > 0 )
		{
			vPosition /= ( 1.0f * nEntrenchmentElementCount );
			for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
			{
				itMapInfoElement->second.vPosition -= vPosition;
				itMapInfoElement->second.fDirection -= fDirection;
			}
		}
		// заполняем сцену и проставляем ссылки в mapInfo
		for ( SObjectInfo::CMapInfoElementMap::iterator itMapInfoElement = mapInfoElementMap.begin(); itMapInfoElement != mapInfoElementMap.end(); ++itMapInfoElement )
		{
			const unsigned nObjectIndex = pObjectInfoCollector->linkIDToIndexCollector.Get( itMapInfoElement->first );
			const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats = dynamic_cast<const NDb::SEntrenchmentRPGStats*>( NDb::GetObject( itMapInfoElement->second.rpgStatsDBID ) );
			const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats *pSeg = GetSegmentInfo( pEntrenchmentRPGStats, itMapInfoElement->second.nFrameIndex );
			CVec3 vObjectScenePosition = vPosition + itMapInfoElement->second.vPosition;
			vObjectScenePosition.z = GetTerrainHeight( vObjectScenePosition.x, vObjectScenePosition.y );
			const float fObjectSceneDirection = fDirection + itMapInfoElement->second.fDirection;
			//
			//	повернуть в вертикальной плоскости и промасштабировать
			//
			CVec3 vTerrainPos = VNULL3;
			CQuat qRot = QNULL;
			CVec3 vScale = CVec3( 1.0f, 1.0f, 1.0f );
			uint32_t dwNormal = Vec3ToDWORD( V3_AXIS_Z );
			GetEntrenchmentSegmentPositionOnTerrain(	&vTerrainPos, &qRot, &dwNormal, &vScale, pSeg->vAABBHalfSize.x, vObjectScenePosition, fObjectSceneDirection );
			//
			const unsigned nSceneID = pObjectInfoCollector->sceneIDCollector.LockID();
			if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
			{
				pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
				pUpdate->info.nObjUniqueID = nSceneID;
				pUpdate->info.pStats = NDb::Get<NDb::SHPObjectRPGStats>( itMapInfoElement->second.rpgStatsDBID );
				pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
				pUpdate->info.bNewFormat = true;
				pUpdate->info.vPlacement = vTerrainPos;
				pUpdate->info.rotation = qRot;
				//
				pUpdate->info.center = CVec2( vTerrainPos.x, vTerrainPos.y );
				pUpdate->info.z = vTerrainPos.z;
				pUpdate->info.dir = Vis2AIRad( fObjectSceneDirection );
				pUpdate->info.dwNormal = dwNormal;
				//
				pUpdate->info.fSpeed = 0.0f;
				pUpdate->info.cSoil = 0;
				pUpdate->info.fResize = 1.0f;
				pUpdate->info.fHitPoints = itMapInfoElement->second.fHP * pEntrenchmentRPGStats->fMaxHP;
				pUpdate->info.fFuel = 1.0f;
				pUpdate->info.eDipl = EDI_FRIEND;
				pUpdate->info.nPlayer = itMapInfoElement->second.nPlayer;
				pUpdate->info.nFrameIndex = itMapInfoElement->second.nFrameIndex;
				pUpdate->info.nExpLevel = 0;
				//
				pObjectInfoCollector->pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
			}		
			//
			SObjectInfo::SSceneElement sceneElement;
			sceneElement.vPosition = itMapInfoElement->second.vPosition;
			sceneElement.fDirection = itMapInfoElement->second.fDirection;
			// заносим элемент в структуру данных объекта
			sceneElementMap[nSceneID] = sceneElement;
			sceneIDToLinkIDMap[nSceneID] = itMapInfoElement->first;
			//
			pObjectInfoCollector->sceneIDMap[nSceneID] = nObjectInfoID;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SEntrenchmentInfo::Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		CPtr<IResourceManager> pResourceManager = Singleton<IResourceManager>();
		if ( !pObjectCreateInfo || !pEditorScene || !pObjectController || !pManipulator || !pResourceManager )
			return false;

		bool bResult = true;

		vPosition = VNULL3;
		fDirection = 0.0f;
		//
		const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats = dynamic_cast<const NDb::SEntrenchmentRPGStats*>( NDb::GetObject( pObjectCreateInfo->rpgStatsDBID ) );
		//
		const std::list<SEntrenchmentSegInfo> &trenchElementsInfo = static_cast<const SEntrenchmentCreateInfo *>( pObjectCreateInfo )->segmentsInfo;
		//
		const int nEntrenchmentElementCount = trenchElementsInfo.size();
		if ( nEntrenchmentElementCount > 0 )
		{
			segmentLinkIDListList.clear();
			CSegmentLinkIDListList::iterator itSegmentLinkIDList = segmentLinkIDListList.end();
			int nEntrenchmentElementIndex = 0;
			bool bLineFound = false;
			//
			for ( std::list<SEntrenchmentSegInfo>::const_iterator itTE = trenchElementsInfo.begin(); itTE!= trenchElementsInfo.end(); ++itTE )
			{
				const SEntrenchmentSegInfo *pSgInfo = &(*itTE);
				if ( ( pSgInfo->eSegType != NDb::EST_LINE ) && ( pSgInfo->eSegType != NDb::EST_FIREPLACE ) )
				{
					if ( bLineFound && ( itTE != ( --trenchElementsInfo.end() ) ) )
					{
						itSegmentLinkIDList = segmentLinkIDListList.end();
						bLineFound = false;
					}
				}
				else
				{
					bLineFound = true;
				}
				//
				CVec3 vPos0 = pSgInfo->vPos0;
				vPos0.z = GetTerrainHeight( vPos0.x, vPos0.y );

				CVec3 vPos1 = pSgInfo->vPos1;
				vPos1.z = GetTerrainHeight( vPos1.x, vPos1.y );

				CVec3 vCenterPoint = 0.5f * ( vPos0 + vPos1 );
				vCenterPoint.z = GetTerrainHeight( vCenterPoint.x, vCenterPoint.y );
				vCenterPoint -= CVec3( pSgInfo->vAABBCenter.x, pSgInfo->vAABBCenter.y, 0 );
				//
				//	повернуть в вертикальной плоскости и промасштабировать
				//
				CVec3 vTerrainPos = VNULL3;
				uint32_t dwNormal = Vec3ToDWORD( V3_AXIS_Z );
				CQuat qRot = QNULL;
				CVec3 vScale = CVec3( 1.0f, 1.0f, 1.0f );
				float fDirection = pSgInfo->fDirAngle;
				SEntrenchmentInfo::GetEntrenchmentSegmentPositionOnTerrain(	&vTerrainPos, &qRot, &dwNormal, &vScale, pSgInfo->vAABBSize.x, vCenterPoint, fDirection );
				vTerrainPos.z = 0;		 // в базе хранится высота сегмента относительно высоты террейна 
				//
				int nObjectIndex = INVALID_NODE_ID;
				CManipulatorManager::GetValue( &nObjectIndex, pManipulator, "Objects" );
				if ( nObjectIndex == INVALID_NODE_ID )
				{
					continue;
				}
				const std::string szObjectPrefix = fmt::format( "Objects.[{}]", nObjectIndex );
				// Insert
				bResult = bResult && pObjectController->AddInsertOperation( "Objects", NODE_ADD_INDEX, pManipulator );
				//Change
				bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szObjectPrefix + ".Pos", vTerrainPos, pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", Vis2AIRad( fDirection ), pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", pObjectCreateInfo->rpgStatsDBID.ToString(), pManipulator );
				int nSegType = static_cast<int>( pSgInfo->eSegType );
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( nSegType ), pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( pObjectCreateInfo->nPlayer ), pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".HP", (int)( pObjectCreateInfo->fHP ), pManipulator );
				DebugTrace( "Add segment, type: %d", pSgInfo->eSegType );
				//
				if ( bResult )
				{
					const int nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
					bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nLinkID, pManipulator );
					//
					pObjectInfoCollector->linkIDToIndexCollector.Insert( nLinkID, nObjectIndex, false );
					//
					// создаем SMapInfoElement и заполняем эго данными
					SObjectInfo::SMapInfoElement mapInfoElement;
					mapInfoElement.szRPGStatsTypeName = pObjectCreateInfo->szRPGStatsTypeName;
					mapInfoElement.rpgStatsDBID = pObjectCreateInfo->rpgStatsDBID;
					//
					mapInfoElement.nFrameIndex = static_cast<int>( pSgInfo->eSegType );
					//
					mapInfoElement.vPosition = pSgInfo->vPosCenter;
					mapInfoElement.vPosition.z = 0;
					mapInfoElement.fDirection = fDirection;
					mapInfoElement.nPlayer = pObjectCreateInfo->nPlayer;
					mapInfoElement.fHP = pObjectCreateInfo->fHP;
					mapInfoElement.linkedLinkIDIist.clear();
					mapInfoElement.nLinkToLinkID = INVALID_NODE_ID;
					mapInfoElementMap[nLinkID] = mapInfoElement;
					//
					if ( itSegmentLinkIDList == segmentLinkIDListList.end() )
					{
						itSegmentLinkIDList = segmentLinkIDListList.insert( segmentLinkIDListList.end(), CSegmentLinkIDList() );
					}
					if ( itSegmentLinkIDList != segmentLinkIDListList.end() )
					{
						itSegmentLinkIDList->push_back( nLinkID );
					}
					//
					pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
				}
				else
				{
					bResult = false;
					break;
				}
				//
				++nEntrenchmentElementIndex;
			}

			//Добавляем информацию в секцию окопов
			int nEntrenchmentIndex = INVALID_NODE_ID;
			bResult = bResult && CManipulatorManager::GetValue( &nEntrenchmentIndex, pManipulator, "Entrenchments" );
			const std::string szEntrenchmentPrefix = fmt::format( "Entrenchments.[{}]", nEntrenchmentIndex );
			bResult = bResult && pObjectController->AddInsertOperation( "Entrenchments", NODE_ADD_INDEX, pManipulator );
			int nLinkListCount = 0;
			for ( CSegmentLinkIDListList::const_iterator itLinkList = segmentLinkIDListList.begin(); itLinkList != segmentLinkIDListList.end(); ++itLinkList )
			{
				if ( !itLinkList->empty() )
				{
					bResult = bResult && pObjectController->AddInsertOperation( szEntrenchmentPrefix + ".sections", NODE_ADD_INDEX, pManipulator );
					bResult = bResult && pObjectController->AddChangeArrayOperation<CSegmentLinkIDList, int>( szEntrenchmentPrefix + fmt::format( ".sections.[{}].data", nLinkListCount ), ( *itLinkList ), pManipulator );
					if( bResult )
					{
						++nLinkListCount;
					}
					else
					{
						break;
					}
				}
			}
			//
			if ( bResult )
			{
				//
				nEntrenchmentID = pObjectInfoCollector->trenchIDCollector.LockID();
				pObjectInfoCollector->trenchIDToIndexCollector.Insert( nEntrenchmentID, nEntrenchmentIndex, false );
				//
				CreateSceneObjects( pEditorScene, pManipulator, true );
				//
			}
			//
		}
		//
		return bResult;
		//
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SEntrenchmentInfo::GetEntrenchmentSegInfoByMapObjElement( SEntrenchmentSegInfo *pSegInfo, const SObjectInfo *pMOI, const NDb::SEntrenchmentRPGStats *pEntrenchmentStats, const SMapInfoElement *pElement )
	{
		// функция восстанавливает данные которые нужны для постановки сегмента окопа в редакторе SEntrenchmentSegInfo
		// по SMapInfoElement данного сегмента
		// (центр сегмента + поворот + длина сегмента ) ---> ( начальная и конечная точки сегмента )
		//
		if ( !pSegInfo || !pMOI || !pEntrenchmentStats || !pElement )
			return false;
		//
		pSegInfo->eSegType = 	static_cast<NDb::EEntrenchSegmType>( pElement->nFrameIndex );
		//
		int i = 0;
		for ( ; i < pEntrenchmentStats->segments.size(); ++i )
		{
			const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &seg = pEntrenchmentStats->segments[i];
			if ( seg.eType == pSegInfo->eSegType )
			{
				pSegInfo->vAABBSize = seg.vAABBHalfSize * 2.0f;
				pSegInfo->vAABBCenter = seg.vAABBCenter;
				break;
			}
		}
		if ( i == pEntrenchmentStats->segments.size() )
			return false;
		//
		pSegInfo->vPosCenter = pElement->vPosition;
		pSegInfo->fDirAngle = ( pElement->fDirection + pMOI->fDirection );
		CQuat qRot( pSegInfo->fDirAngle, V3_AXIS_Z );
		CVec3 vSegDir = VNULL3;
		qRot.Rotate( &vSegDir, V3_AXIS_X );
		pSegInfo->vPos0 = ( pSegInfo->vPosCenter - (vSegDir *  pSegInfo->vAABBSize.x / 2.0f ) );
		pSegInfo->vPos1 = ( pSegInfo->vPosCenter + (vSegDir *  pSegInfo->vAABBSize.x / 2.0f ) );
		//
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SEntrenchmentInfo::PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( !SObjectInfo::PasteSelf( pNew2OldLinkIDMap, pOld2NewLinkIDMap, pEditorScene, pObjectController, pManipulator ) )
		{
			return false;
		}
		if ( ( pNew2OldLinkIDMap == 0 ) || ( pOld2NewLinkIDMap == 0 ) )
		{
			return false;
		}
		int nEntrenchmentIndex = INVALID_NODE_ID;
		bool bResult = CManipulatorManager::GetValue( &nEntrenchmentIndex, pManipulator, "Entrenchments" );
		const std::string szEntrenchmentPrefix = fmt::format( "Entrenchments.[{}]", nEntrenchmentIndex );
		bResult = bResult && pObjectController->AddInsertOperation( "Entrenchments", NODE_ADD_INDEX, pManipulator );
		int nLinkListCount = 0;
		for ( CSegmentLinkIDListList::iterator itLinkList = segmentLinkIDListList.begin(); itLinkList != segmentLinkIDListList.end(); ++itLinkList )
		{
			if ( !itLinkList->empty() )
			{
				for ( CSegmentLinkIDList::iterator itLinkID = itLinkList->begin(); itLinkID != itLinkList->end(); ++itLinkID )
				{
					( *itLinkID ) = ( *pOld2NewLinkIDMap )[*itLinkID];
				}
				bResult = bResult && pObjectController->AddInsertOperation( szEntrenchmentPrefix + ".sections", NODE_ADD_INDEX, pManipulator );
				bResult = bResult && pObjectController->AddChangeArrayOperation<CSegmentLinkIDList, int>( szEntrenchmentPrefix + fmt::format( ".sections.[{}].data", nLinkListCount ), ( *itLinkList ), pManipulator );
				if( bResult )
				{
					++nLinkListCount;
				}
				else
				{
					break;
				}
			}
		}
		if ( bResult )
		{
			//
			nEntrenchmentID = pObjectInfoCollector->trenchIDCollector.LockID();
			pObjectInfoCollector->trenchIDToIndexCollector.Insert( nEntrenchmentID, nEntrenchmentIndex, false );
			//
			CreateSceneObjects( pEditorScene, pManipulator, true );
			//
		}
		return bResult;
	}
}

// basement storage  


