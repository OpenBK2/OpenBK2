#include "stdafx.h"

#include "SingleReinforcement.h"
#include "NewUpdater.h"
#include "General.h"
#include "AILogicInternal.h"
#include "UnitCreation.h"
#include "AIGeometry.h"
#include "Diplomacy.h"
#include "ExecutorContainer.h"
#include "ExecutorSeaReinf.h"
#include "Formation.h"
#include "Soldier.h"
#include "Units.h"
#include "UnitsIterators2.h"
#include "Reinforcement.h"

#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/Terrain.h"
#include "FeedbackSystem.h"

extern CFeedBackSystem theFeedBackSystem;
extern CSupremeBeing theSupremeBeing;
extern CEventUpdater updater;
extern CUnitCreation theUnitCreation;
extern CScripts* pScripts;
extern CDiplomacy theDipl;
extern CExecutorContainer theExecutorContainer;
extern CUnits units;

static const int FIND_POSITION_STEP = 5;
static const int DIRECTION_STEP = 8;

namespace NReinforcement
{

bool PositionLocked( const CVec2 &vPos, const int nBoundTileRadius, const EAIClasses aiClass )
{
	return GetTerrain()->CanUnitGo( nBoundTileRadius, GetAIMap()->GetTile( vPos.x, vPos.y ), aiClass ) == FREE_NONE;
}

CVec2 FindPositionForUnit( const CVec2 &vDefaultPos, const int nBoundTileRadius, const EAIClasses aiClass )
{
	std::list<SObjTileInfo> tiles;

	for ( CUnitsIter<0,0> it( 0, ANY_PARTY, vDefaultPos, nBoundTileRadius * FIND_POSITION_STEP * AI_TILE_SIZE ); !it.IsFinished(); it.Iterate() )
	{
		CAIUnit *pUnit = *it;
		if ( IsValid( pUnit ) )
		{
			std::vector<SObjTileInfo> tempTiles;
			const SUnitProfile &profile = pUnit->GetUnitProfile();
			if ( profile.bRect )
				GetAIMap()->GetTilesCoveredByRect( profile.rect, &tempTiles );
			else
				GetAIMap()->GetTilesCoveredByCircle( profile.circle.center, profile.circle.r, &tempTiles );
			tiles.insert( tiles.end(), tempTiles.begin(), tempTiles.end() );
		}
	}

	const int nTempLockID = GetTerrain()->TemporaryLockTiles( tiles );

	CVec2 vResult( vDefaultPos );
	if ( PositionLocked( vResult, nBoundTileRadius, aiClass ) )
	{
		const float fDistDelta = nBoundTileRadius * AI_TILE_SIZE;
		bool bPositionFound = false;
		float fDist = fDistDelta;
    for ( int i = 1; i < FIND_POSITION_STEP && !bPositionFound; ++i, fDist += fDistDelta )
		{
			WORD wDir = 0;
			for ( int n = 0; n < DIRECTION_STEP && !bPositionFound; ++n, wDir += 65536/DIRECTION_STEP )
			{
				const CVec2 vTempResult = vResult + GetVectorByDirection( wDir ) * fDist;
				if ( !PositionLocked( vTempResult, nBoundTileRadius, aiClass ) )
				{
					vResult = vTempResult;
					bPositionFound = true;
					//DebugInfoManager()->CreateCircle( NDebugInfo::OBJECT_ID_GENERATE, CCircle( vTempResult, 16 ), NDebugInfo::GREEN );
				}
			}
		}
	}

	GetTerrain()->RemoveTemporaryLock( nTempLockID );
	return vResult;
}

void PlaceReinforcement( EReinforcementType eType, const int nPlayer, const std::vector<NDb::SReinforcementEntry> &entries,
	const std::vector<NDb::SDeployTemplate::SDeployTemplateEntry> &pos, const CVec2 &vPosition, WORD wDirection,
	std::list< std::pair<int, CObjectBase*> > *pObjects, const int nForceID, const int nScriptID, const bool bDisableUpdates )
{
	if ( !bDisableUpdates )
	{
		updater.AddUpdate( EFB_REINFORCEMENT_ARRIVED );			
		if ( nPlayer == theDipl.GetMyNumber() )
			updater.AddUpdate( EFB_REINFORCEMENT_CENTER_LOCAL_PLAYER, MAKELONG( vPosition.x, vPosition.y ) );
	}

	LinkInfo links;
	std::list<CCommonUnit*> units;

	std::list<int> freeLinks;
	CLinkObject::GetFreeLinks( &freeLinks, entries.size() );
	std::list<int>::const_iterator itLink = freeLinks.begin();
	det_map<int,int> linksDistrib;
	for ( int i = 0; i < entries.size(); ++i )
	{
		NI_ASSERT( i < pos.size(), StrFmt( "MAP DESIGN: not enough positions in deploy mask, need %i, unit not deployed", i+1 )  );
		if ( i < pos.size() )
		{
			CVec2 vPos2( vPosition + MoveVectorByDirection( pos[i].vPosition, wDirection ) );
			const WORD wDir = wDirection + pos[i].nDirection;
			SMapObjectInfo info;
			const NDb::SHPObjectRPGStats *pStats = entries[i].pMechUnit;
			EAIClasses aiClass = EAC_HUMAN;
			//CVec2 vSize = VNULL2;
			int nBoundTileRadius = 0;
			if ( pStats == 0 ) 
			{
				pStats = entries[i].pSquad;

				NI_ASSERT( pStats, StrFmt( "Null entry %d in reinforcement (no mech, no squad)", i ) );

				if ( !pStats )
					continue;
			}
			else
			{
				aiClass = (EAIClasses)entries[i].pMechUnit->nAIPassabilityClass;
				nBoundTileRadius = entries[i].pMechUnit->nBoundTileRadius + 1;
				//vSize = entries[i].pMechUnit->vAABBHalfSize;
				//vSize = MoveVectorByDirection( vSize, wDir );
			}

			// Check if initial position is inside AI map, move if necessary
			CVec3 vPos( vPos2.x, vPos2.y, 0 );
			if ( pStats->eGameType == SGVOGT_SQUAD || !checked_cast<const SUnitBaseRPGStats*>(pStats)->IsAviation() )
			{
				const float fUnitSize = nBoundTileRadius * AI_TILE_SIZE;
				vPos2.x = Clamp( vPos.x, fUnitSize, GetAIMap()->GetSizeX() * AI_TILE_SIZE - fUnitSize );
				vPos2.y = Clamp( vPos.y, fUnitSize, GetAIMap()->GetSizeY() * AI_TILE_SIZE - fUnitSize );

				if ( aiClass != EAC_NONE )
				{
					vPos2 = FindPositionForUnit( vPos2, nBoundTileRadius, aiClass );
					vPos.x = vPos2.x;
					vPos.y = vPos2.y;
				}
			}
			info.pObject = pStats;
			info.nPlayer = nPlayer;
			info.fHP = 1.0f;
			info.nScriptID = nScriptID;
			info.nDir = wDir;
			info.vPos = vPos;
			info.link.nLinkWith = entries[i].nLinkWith;
			info.link.nLinkID = entries[i].nLinkIndex;
			info.link.bIntention = false;
			info.nFrameIndex = -1;

			if ( info.link.nLinkWith != -1 )
			{
				if ( linksDistrib.find( info.link.nLinkWith ) == linksDistrib.end() )
				{
					linksDistrib[info.link.nLinkWith] = *itLink;
					++itLink;
				}
				info.link.nLinkWith = linksDistrib[info.link.nLinkWith];
			}
			if ( info.link.nLinkID != -1 )
			{
				if ( linksDistrib.find( info.link.nLinkID ) == linksDistrib.end() )
				{
					linksDistrib[info.link.nLinkID] = *itLink;
					++itLink;
				}
				info.link.nLinkID = linksDistrib[info.link.nLinkID];
			}

			const int nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
			CObjectBase *pObj = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->AddObject( nUniqueID, info, &links, false, pStats, eType );
			if ( pObj )
			{
				pObjects->push_back( std::pair<int, CObjectBase*>( i, pObj ) );

				if ( CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>( pObj ) )
				{
					CAIUnit *pAIUnit = dynamic_cast<CAIUnit*>( pUnit );
					CFormation *pForm = dynamic_cast<CFormation*>( pUnit );
					if ( pAIUnit )
					{
						const EReinforcementType eTempType = eType == _RT_NONE ? NReinforcement::GetReinforcementTypeByUnitRPGType( pAIUnit->GetStats()->etype ) : eType;
						pAIUnit->SetReinforcementType( eTempType );
					}
					else if ( pForm )
					{
						for ( int j = 0; j < pForm->Size(); ++j )
						{
							const EReinforcementType eTempType = eType == _RT_NONE ? NReinforcement::GetReinforcementTypeByUnitRPGType( (*pForm)[j]->GetStats()->etype ) : eType;
							(*pForm)[j]->SetReinforcementType( eTempType );
						}
					}
					units.push_back( pUnit );
				}
			}
		}
	}
	
	// send acknowledgement
	if ( units.begin() != units.end() && nPlayer == theDipl.GetMyNumber() )
		(*units.begin())->SendAcknowledgement( NDb::ACK_REINFORCEMENT_ARRIVED, true );

	if ( theDipl.GetNParty( nPlayer ) != theDipl.GetMyParty() && 
		( eType == NDb::RT_BOMBERS || eType == NDb::RT_FIGHTERS || eType == NDb::RT_GROUND_ATTACK_PLANES || eType == NDb::RT_RECON ||
		  eType == NDb::RT_EXTRA_AIR_1 || eType == NDb::RT_EXTRA_AIR_2 || eType == NDb::RT_EXTRA_AIR_3 || eType == NDb::RT_EXTRA_AIR_4 || eType == NDb::RT_EXTRA_AIR_5 ||
		  eType == NDb::RT_EXTRA_MAXLVL_AIR_1 || eType == NDb::RT_EXTRA_MAXLVL_AIR_2 || eType == NDb::RT_EXTRA_MAXLVL_AIR_3 || eType == NDb::RT_EXTRA_MAXLVL_AIR_4 || eType == NDb::RT_EXTRA_MAXLVL_AIR_5 ) )
	{
		CPtr<SFeedBackUnitsArray> pParam = new SFeedBackUnitsArray;
		for ( std::list<CCommonUnit*>::iterator it = units.begin(); it != units.end(); ++it )
			pParam->unitIDs.push_back( (*it)->GetUniqueID() );

		if ( !bDisableUpdates )
			updater.AddUpdate( EFB_ENEMY_AVIATION_CALLED, MAKELONG( vPosition.x, vPosition.y ), pParam );
	}

	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->InitLinks( links );
	theSupremeBeing.GiveNewUnitsToGenerals( units, true );
}

void PlaceSingleLandReinforcement( EReinforcementType eType, const int nPlayer, const std::vector<NDb::SReinforcementEntry> &entries,
	const int nForceID,	const std::vector<NDb::SDeployTemplate::SDeployTemplateEntry> position, const CVec2 &vPosition, const WORD wDirection )
{
	std::list< std::pair<int, CObjectBase*> > objects;
	PlaceReinforcement( eType, nPlayer, entries, position, vPosition, wDirection, &objects, nForceID, -1, false );
}

void PlaceSingleLandReinforcement( const int nPlayer, const NDb::SReinforcement *pReinf, const EReinforcementType eType,
	const NDb::SDeployTemplate *pTemplate, const CVec2 &vPosition, WORD wDirection, const int nScriptID,
	std::list< std::pair<int, CObjectBase*> > *pObjects, const bool bDisableUpdates )
{
	CDBPtr<NDb::SDeployTemplate> pFinalTemplate = pTemplate;

	if ( pReinf->pTemplateOverride )
		pFinalTemplate = pReinf->pTemplateOverride;

	NI_ASSERT( pFinalTemplate != 0, StrFmt( "Map design error: No deploy template for player = %i reinforcement type = %i", nPlayer, pReinf->eType ) );
	if ( pFinalTemplate == 0 )
		return;

	std::list< std::pair<int, CObjectBase*> > objects;
	std::list< std::pair<int, CObjectBase*> > *pObjectsLocal = pObjects == 0 ? &objects : pObjects;
	PlaceReinforcement( eType, nPlayer, pReinf->entries, pFinalTemplate->entries, vPosition, wDirection, pObjectsLocal, -1, nScriptID, bDisableUpdates );

	/*list<CCommonUnit*> units;
	for ( list< pair<int, CObjectBase*> >::iterator iter = pObjectsLocal->begin(); iter != pObjectsLocal->end(); ++iter )
	{
		CObjectBase *pObj = iter->second;
		if ( CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>( pObj ) )
		{
			if ( CAIUnit *pAIUnit = dynamic_cast<CAIUnit*>(pUnit) )
				pAIUnit->SetReinforcementType( pReinf->eType );
			//units.push_back( pUnit );
		}
	}*/
}

void PlaceSingleSeaReinforcement( const int nPlayer, const NDb::SReinforcement *pReinf, const NDb::SDeployTemplate *pTemplate,
																	const CVec2 &vPosition, WORD wDirection, const int nScriptID, const CVec2 &vTarget )
{
	std::vector<NDb::SReinforcementEntry> entries;
	int nMinLink = 0;
	entries.resize( pReinf->transports.size() + pReinf->entries.size() );

	// Check for linked units
	for ( int i = 0; i < pReinf->entries.size(); ++i )
	{
		if ( pReinf->entries[i].nLinkIndex >= nMinLink )
			nMinLink = pReinf->entries[i].nLinkIndex;
	}

	for ( int i = 0; i < pReinf->transports.size(); ++i )
	{
		entries[i].pMechUnit = pReinf->transports[i];
		entries[i].pSquad = 0;
		entries[i].nLinkIndex = i + nMinLink + 1;
		entries[i].nLinkWith = -1;
	}

	// Add units to transports
	int nTransportId = 0;
	const int nTrCount = pReinf->transports.size();
	for ( int i = 0; i < pReinf->entries.size(); ++i )
	{
		entries[i + nTrCount].pMechUnit = pReinf->entries[i].pMechUnit;
		entries[i + nTrCount].pSquad = pReinf->entries[i].pSquad;
		entries[i + nTrCount].nLinkIndex = pReinf->entries[i].nLinkIndex;
		if ( pReinf->entries[i].nLinkWith == -1 )
		{
			entries[i + nTrCount].nLinkWith = nTransportId + nMinLink + 1;
			nTransportId = ( nTransportId + 1 ) % nTrCount;
		}
		else
		{
			entries[i + nTrCount].nLinkWith = pReinf->entries[i].nLinkWith;
		}
	}

	std::list< std::pair<int, CObjectBase*> > objects;
	PlaceReinforcement( pReinf->eType, nPlayer, entries, pTemplate->entries, vPosition, wDirection, &objects, -1, nScriptID, false );

	// Make a list of transports
	std::list<CAIUnit*> ships;
	int i = 0;
	for ( std::list< std::pair<int, CObjectBase*> >::iterator iter = objects.begin(); iter != objects.end(); ++iter, ++i )
	{
		CObjectBase *pObj = iter->second;
		CAIUnit *pUnit = dynamic_cast<CAIUnit*>( pObj );
		if ( i >= nTrCount )
		{
			continue;
		}

		if ( pUnit )
		{
			pUnit->SetSelectable( false, true );
			ships.push_back( pUnit );
		}
	}

	// Create direction
	CVec2 vSendTo = GetVectorByDirection( wDirection );
	vSendTo *= 50;
	vSendTo += vPosition;

	// Create executor
	CPtr<CExecutor> pEx = new CExecutorTransportReinforcement( &ships, pReinf, vPosition, vSendTo, vTarget );
	theExecutorContainer.Add( pEx );
}

}


