#include "StdAfx.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\sceneb2\scene.h"
#include "..\mapeditorlib\multimanipulator.h"
#include "SimpleObjectInfoData.h"

#include "../Stats_B2_M1/M1UnitType.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NMapInfoEditor
{
	bool SSimpleObjectInfo::NeedMakeOrientation()
	{
		if ( !mapInfoElementMap.empty() )
		{
			const SMapInfoElement &rMapInfoElement = mapInfoElementMap.begin()->second;
			return NeedMakeOrientation( rMapInfoElement.szRPGStatsTypeName, rMapInfoElement.rpgStatsDBID );
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSimpleObjectInfo::KeepZeroHeight()
	{
		if ( !mapInfoElementMap.empty() )
		{
			const SMapInfoElement &rMapInfoElement = mapInfoElementMap.begin()->second;
			if ( rMapInfoElement.szRPGStatsTypeName == "MechUnitRPGStats" )
			{
				if ( const NDb::SMechUnitRPGStats *pMechUnitRPGStats = dynamic_cast<const NDb::SMechUnitRPGStats*>( NDb::GetObject( rMapInfoElement.rpgStatsDBID ) ) )
				{
					if ( ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_SCOUT ) ||
							 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_BOMBER ) ||
							 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_ATTACK ) ||
							 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_FIGHTER )||
							 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_SUPER ) ||
							 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_LANDER ) ||
							 ( ( pMechUnitRPGStats->pM1UnitTargetType ) && 
								 ( pMechUnitRPGStats->pM1UnitTargetType->eBaseType == NDb::M1_PLANE ) )
						 )
					{
						return false;
					}
					else
						return true;
				}
			}
			else if ( ( rMapInfoElement.szRPGStatsTypeName == "SquadRPGStats" ) ||
								( rMapInfoElement.szRPGStatsTypeName == "InfantryRPGStats" ) )
			{
				return true;
			}
		}
		return false;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSimpleObjectInfo::Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSimpleObjectInfo::Load(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectLoadInfo != 0, "SSimpleObjectInfo::Load(), pObjectLoadInfo == 0" );
		//
		// Устанавливаем общие параметры
		const string szObjectPrefix = StrFmt( "Objects.[%d]", pObjectLoadInfo->nObjectIndex );
		// создаем SMapInfoElement и заполняем его данными
		bool bResult = true;
		{
			UINT nLinkID = INVALID_NODE_ID;
			SObjectInfo::SMapInfoElement mapInfoElement;
			if ( pObjectLoadInfo->bAdditionalDataFilled )
			{
				mapInfoElement.szRPGStatsTypeName = pObjectLoadInfo->szRPGStatsTypeName;
				mapInfoElement.rpgStatsDBID = pObjectLoadInfo->rpgStatsDBID;
				mapInfoElement.nFrameIndex = pObjectLoadInfo->nFrameIndex;
				mapInfoElement.nPlayer = pObjectLoadInfo->nPlayer;
				mapInfoElement.fHP = pObjectLoadInfo->fHP;
				vPosition = pObjectLoadInfo->vPosition;
				fDirection = pObjectLoadInfo->fDirection;
				nLinkID = pObjectLoadInfo->nLinkID;
				mapInfoElement.nLinkToLinkID = pObjectLoadInfo->nLinkWith;
			}
			else
			{
				string szRPGStatsTypeName;
				string szRPGStatsName;
				bResult = bResult && CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0 );
				mapInfoElement.szRPGStatsTypeName = szRPGStatsTypeName;
				mapInfoElement.rpgStatsDBID = CDBID( szRPGStatsName );
				bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.nFrameIndex ), pManipulator, szObjectPrefix + ".FrameIndex" );
				bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.nPlayer ), pManipulator, szObjectPrefix + ".Player" );
				bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.fHP ), pManipulator, szObjectPrefix + ".HP" );
				bResult = bResult && CManipulatorManager::GetVec3<CVec3, float>( &vPosition, pManipulator, szObjectPrefix + ".Pos" );
				WORD wDirection = 0;
				bResult = bResult && CManipulatorManager::GetValue( &wDirection, pManipulator, szObjectPrefix + ".Dir" );
				fDirection = AI2VisRad( wDirection );
				bResult = bResult && CManipulatorManager::GetValue( &nLinkID, pManipulator, szObjectPrefix + ".Link.LinkID" );
				mapInfoElement.nLinkToLinkID = INVALID_NODE_ID;
				bResult = bResult && CManipulatorManager::GetValue( &( mapInfoElement.nLinkToLinkID ), pManipulator, szObjectPrefix + ".Link.LinkWith" );
			}
			//
			mapInfoElement.vPosition = VNULL3;
			mapInfoElement.fDirection = 0.0f;
			//
			mapInfoElement.linkedLinkIDIist.clear();
			// заносим элемент в структуру данных объекта
			mapInfoElementMap[nLinkID] = mapInfoElement;
			pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;
		}
		//
		// заполняем сцену и проставляем ссылки в mapInfo
		CreateSceneObjects( pEditorScene, pManipulator, true );
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSimpleObjectInfo::Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		NI_ASSERT( pObjectInfoCollector != 0, "SSimpleObjectInfo::Create(), pObjectInfoCollector == 0" );
		NI_ASSERT( pObjectCreateInfo != 0, "SSimpleObjectInfo::Create(), pObjectCreateInfo == 0" );
		//
		const NDb::SHPObjectRPGStats *pHPObjectRPGStats = dynamic_cast<const NDb::SHPObjectRPGStats*>( NDb::GetObject( pObjectCreateInfo->rpgStatsDBID ) );
		NI_ASSERT( pHPObjectRPGStats != 0, StrFmt( "%s not NDb::SHPObjectRPGStats type", pObjectCreateInfo->szRPGStatsTypeName.c_str() ) ); 
		if (  pHPObjectRPGStats == 0 )
		{
			return false;
		}
		// Устанавливаем общие параметры
		vPosition = pObjectCreateInfo->vPosition;
		fDirection = pObjectCreateInfo->fDirection;

		// Добавляем объекты базы
		int nObjectIndex = INVALID_NODE_ID;
		CManipulatorManager::GetValue( &nObjectIndex, pManipulator, "Objects" );
		if ( nObjectIndex == INVALID_NODE_ID )
		{
			return false;
		}
		// создаем SMapInfoElements и заполняем их данными
		bool bResult = true;
		{
			SObjectInfo::SMapInfoElement mapInfoElement;
			mapInfoElement.szRPGStatsTypeName = pObjectCreateInfo->szRPGStatsTypeName;
			mapInfoElement.rpgStatsDBID = pObjectCreateInfo->rpgStatsDBID;
			mapInfoElement.nFrameIndex = pObjectCreateInfo->nFrameIndex;
			mapInfoElement.nPlayer = pObjectCreateInfo->nPlayer;
			mapInfoElement.fHP = pObjectCreateInfo->fHP;
			//
			mapInfoElement.vPosition = VNULL3;
			mapInfoElement.fDirection = 0.0f;
			//
			mapInfoElement.linkedLinkIDIist.clear();
			mapInfoElement.nLinkToLinkID = INVALID_NODE_ID;

			const string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
			// Insert
			bResult = bResult && pObjectController->AddInsertOperation( "Objects", NODE_ADD_INDEX, pManipulator );
			//Change
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", mapInfoElement.rpgStatsDBID.ToString(), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( mapInfoElement.nFrameIndex ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( mapInfoElement.nPlayer ), pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".HP", (int)( mapInfoElement.fHP ), pManipulator );
			//
			if ( bResult )
			{
				const int nLinkID = pObjectInfoCollector->linkIDCollector.LockID();
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nLinkID, pManipulator );
				//
				pObjectInfoCollector->linkIDToIndexCollector.Insert( nLinkID, nObjectIndex, false );
				//
				mapInfoElementMap[nLinkID] = mapInfoElement;
				pObjectInfoCollector->linkIDMap[nLinkID] = nObjectInfoID;

				//обновим роложение объектв свсоответствии с настройками
				if ( pObjectCreateInfo->bRotateTo90Degree )
				{
					RotateTo90Degree( true );
				}
				if ( pObjectCreateInfo->bFitToGrid )
				{
					FitToGrid( true );
				}
				//
				FixInvalidPos( false );
				//
				bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szObjectPrefix + ".Pos", mapInfoElement.GetPosition( vPosition ), pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", Vis2AIRad( mapInfoElement.GetDirection( fDirection ) ), pManipulator );
			}
		}
		CreateSceneObjects( pEditorScene, pManipulator, true );
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const NDb::SModel* SSimpleObjectInfo::GetModel( const NDb::SHPObjectRPGStats *pHPObjectRPGStats, const NDb::ESeason eSeason )
	{
		if ( pHPObjectRPGStats )
		{
			if ( const NDb::SVisObj *pVisObj = pHPObjectRPGStats->pvisualObject )
			{
				return ::GetModel( pVisObj, eSeason ); 
			}
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const NDb::SModel* SSimpleObjectInfo::ChooseModelWithHP( const NDb::SHPObjectRPGStats *pHPObjectRPGStats, const float fHP, const NDb::ESeason eSeason )
	{
		if ( pHPObjectRPGStats )
		{
			if ( fHP != 1 && !pHPObjectRPGStats->damageLevels.empty() ) 
			{
				const NDb::SVisObj *pVisObj = pHPObjectRPGStats->damageLevels.back().pVisObj;
				for ( int nDamadeLeveIndex = pHPObjectRPGStats->damageLevels.size() - 1; nDamadeLeveIndex >= 0; --nDamadeLeveIndex ) 
				{
					if ( pHPObjectRPGStats->damageLevels[nDamadeLeveIndex].fDamageHP >= fHP ) 
					{
						return ::GetModel( pVisObj, eSeason );
					}
					else
					{
						pVisObj = pHPObjectRPGStats->damageLevels[nDamadeLeveIndex].pVisObj;
					}
				}
			}
			return ::GetModel( pHPObjectRPGStats->pvisualObject, eSeason );
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SSimpleObjectInfo::NeedMakeOrientation( const string &rszRPGStatsTypeName, const CDBID &rRPGStatsDBID )
	{
		if ( rszRPGStatsTypeName == "MechUnitRPGStats" )
		{
			if ( const NDb::SMechUnitRPGStats *pMechUnitRPGStats = dynamic_cast<const NDb::SMechUnitRPGStats*>( NDb::GetObject( rRPGStatsDBID ) ) )
			{
				if ( ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_SCOUT ) ||
						 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_BOMBER ) ||
						 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_ATTACK ) ||
						 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_FIGHTER )||
						 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_SUPER ) ||
						 ( pMechUnitRPGStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_LANDER ) )
				{
					return false;
				}
				return true;
			}
		}
		return false;
	}
};

// basement storage



