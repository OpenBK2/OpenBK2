#include "stdafx.h"

#include "Misc/bresenham.h"
#include "System/Time.h"
#include "Graveyard.h"
#include "NewUpdater.h"
#include "AIUnit.h"
#include "GlobalWarFog.h"
#include "Units.h"
#include "Diplomacy.h"
#include "FakeObjects.h"
#include "Common_RTS_AI/AIMap.h"
#include "Stats_B2_M1/AnimationFromAction.h"
#include "Stats_B2_M1/AnimModelGet.h"

CGraveyard theGraveyard;

extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CGlobalWarFog theWarFog;
extern CUnits units;
extern CDiplomacy theDipl;

//*******************************************************************
//*                     SKilledUnit                                 *
//*******************************************************************
struct SKilledUnit : public CObjectBase
{ 
	OBJECT_NOCOPY_METHODS( SKilledUnit );
public:
	ZDATA
		CObj<CAIUnit> pUnit;

		NTimer::STime endFogTime; 
		NTimer::STime timeToEndDieAnimation;
		// кончилсась анимация смерти и проинициализировались и endFogTime
		bool bAnimFinished;
		bool bRemoveCorpseAfterAnimation;

		std::list<SObjTileInfo> lockedTiles; // залоканные тайлы
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&endFogTime); f.Add(4,&timeToEndDieAnimation); f.Add(5,&bAnimFinished); f.Add(6,&lockedTiles); f.Add(7,&bRemoveCorpseAfterAnimation); return 0; }

	SKilledUnit() : endFogTime( 0 ), timeToEndDieAnimation( 0 ), bAnimFinished( false ), bRemoveCorpseAfterAnimation( false ) { }
	SKilledUnit( CAIUnit *_pUnit, const NTimer::STime _timeToEndDieAnimation ) : pUnit( _pUnit ), timeToEndDieAnimation( _timeToEndDieAnimation ), endFogTime( 0 ), bAnimFinished( false ), bRemoveCorpseAfterAnimation( false ) {}
	SKilledUnit( CAIUnit *_pUnit, const NTimer::STime _timeToEndDieAnimation, const NTimer::STime _endFogTime ) : pUnit( _pUnit ), timeToEndDieAnimation( _timeToEndDieAnimation ), endFogTime( _endFogTime ), bAnimFinished( false ), bRemoveCorpseAfterAnimation( false ) {}
};

//*******************************************************************
//*													CGraveyard															*
//*******************************************************************

void CGraveyard::Segment()
{
	std::list< CPtr<SKilledUnit> >::iterator iter = killed.begin();
	while ( iter != killed.end() )
	{
		SKilledUnit *pKilled = *iter;

		if ( !pKilled->bAnimFinished && curTime >= pKilled->timeToEndDieAnimation	)
		{
			pKilled->bAnimFinished = true;
			pKilled->endFogTime = curTime + SConsts::DEAD_SEE_TIME;

			if ( !pKilled->pUnit->IsInfantry() )
			{
				GetTerrain()->RemoveStaticObjectTiles( pKilled->lockedTiles );
				const SMechUnitRPGStats *pMechStats = dynamic_cast<const SMechUnitRPGStats*>( pKilled->pUnit->GetStats() );
				const bool bIsAmphibian = pMechStats && pMechStats->IsAmphibious() && pMechStats->amphibianStats;
				const bool bIsShip = ( pKilled->pUnit->GetAIPassabilityClass() & EAC_WATER ) && !bIsAmphibian;
				if ( !bIsShip && !pKilled->bRemoveCorpseAfterAnimation )
					CFakeCorpseStaticObject::CreateFakeCorpseStaticObject( pKilled->pUnit, pKilled->lockedTiles, pKilled->pUnit->IsTrampled() );
				else
				{
					pKilled->pUnit->FixUnlocking();
					pKilled->pUnit->Disappear();
				}
			}

			++iter;
		}
		else if ( pKilled->bAnimFinished )
		{
			if ( pKilled->endFogTime != 0 && curTime >= pKilled->endFogTime )
			{
				pKilled->endFogTime = 0;
				if ( pKilled->pUnit->GetParty() < 2 )
					theWarFog.DeleteUnit( pKilled->pUnit->GetUniqueId() );
			}

			if ( pKilled->endFogTime == 0 )
			{
				units.FullUnitDelete( pKilled->pUnit );
				iter = killed.erase( iter );
			}
			else
				++iter;
		}
		else
			++iter;
	}

	CheckSoonBeDead();

	for ( CDissapeared::const_iterator it = dissapeared.begin(); it != dissapeared.end(); ++it )
	{
		CObj<CAIUnit> pUnit = *it;
		updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pUnit, 0 );
		pUnit->UnfixUnlocking();
		pUnit->UnlockTiles();
		units.DeleteUnitFromMap( pUnit );
		units.FullUnitDelete( pUnit );
		if ( pUnit->GetParty() < 2 )
			theWarFog.DeleteUnit( pUnit->GetUniqueId() );
	}
	dissapeared.clear();

	for ( std::unordered_map<int,bool>::iterator it = diedVisible.begin(); it != diedVisible.end(); )
	{
		if ( !CLinkObject::IsLinkObjectExists( it->first ) )
			diedVisible.erase( it++ );
		else
			++it;
	}
}

void CGraveyard::AddKilledUnit( CAIUnit *pUnit, const NTimer::STime &timeOfVisDeath, const int nFatality, const int nDeathAnimType )
{
	pUnit->UnfixUnlocking();
	if ( pUnit->CanLockTiles() )
		pUnit->ForceLockTiles();

	CPtr<SKilledUnit> pKillInfo = new SKilledUnit();
	
	const SUnitBaseRPGStats *pStats = pUnit->GetStats();
	const SMechUnitRPGStats* mech = dynamic_cast<const SMechUnitRPGStats*>(pStats);
	std::vector<const NDb::SAnimB2*> animable_anims;
	const NDb::SAmphibianStats *pAmphibianStats = mech && mech->IsAmphibious() ? mech->amphibianStats : 0;
	const bool bRemoveAmphCorpseInWater = pAmphibianStats && pAmphibianStats->removeCorpseInWater && pUnit->IsInWater();
	// играем fatality
	if ( nFatality > -1 )
	{
		pKillInfo->bAnimFinished = false;
		pKillInfo->endFogTime = 0;

		int nAABBD = -1;
		
		if (pStats->animdescs.size() > NDb::ANIMATION_DEATH_FATALITY && pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims.size() > nFatality)
			nAABBD = pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims[nFatality].nAABB_D;

		SRect finishRect;
		const CVec2 vFrontDir = GetVectorByDirection( pUnit->GetFrontDirection() );
		const CVec2 vRectTurn( vFrontDir.y, -vFrontDir.x );
		CVec2 vRectCenter(VNULL2), vRectHalfSize(VNULL2);
		if ( !pStats->aabb_as.empty() && nAABBD != -1 && pStats->aabb_as.size() > nAABBD )
		{
			vRectCenter = pStats->aabb_as[nAABBD].vCenter;
			vRectHalfSize = pStats->aabb_as[nAABBD].vHalfSize;
		}

		if (nAABBD == -1)
		{
			vRectCenter = pStats->vAABBCenter;
			vRectHalfSize = pStats->vAABBHalfSize;
		}

		finishRect.InitRect( pUnit->GetCenterPlain() + ( (vRectCenter) ^ vRectTurn ), vFrontDir,
												 vRectHalfSize.y, vRectHalfSize.x );

		GetAIMap()->GetTilesCoveredByRect( finishRect, &(pKillInfo->lockedTiles) );

		pKillInfo->pUnit = pUnit;
		if ( timeOfVisDeath == 0 )
		{
			pKillInfo->timeToEndDieAnimation = 0;
			pKillInfo->bAnimFinished = false;
		}
		else if (pStats->animdescs.size() > NDb::ANIMATION_DEATH_FATALITY && pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims.size() > nFatality)
			pKillInfo->timeToEndDieAnimation = timeOfVisDeath + pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims[nFatality].nLength + 2 * SConsts::AI_SEGMENT_DURATION;
		else
		{
			if (!mech || !mech->pAnimableModel)
				return;	// fatal error

			animable_anims = GetVisObjAnimsFromModel(mech->pAnimableModel, NDb::EAnimationType::ANIMATION_DEATH_FATALITY);
			
			if (animable_anims.size() < nFatality)
				return;

			pKillInfo->timeToEndDieAnimation = timeOfVisDeath + animable_anims[nFatality]->nLength + 2 * SConsts::AI_SEGMENT_DURATION;
		}
	}
	else
	{
		pKillInfo->bAnimFinished = false;
		pKillInfo->endFogTime = 0;
		pKillInfo->pUnit = pUnit;

		GetAIMap()->GetTilesCoveredByRect( pUnit->GetUnitRect(), &(pKillInfo->lockedTiles) );

		const int nAnimation = nDeathAnimType >= 0 ? nDeathAnimType : GetAnimationFromAction( pUnit->GetDieAction() );
		const int nAnimIndex = - ( nFatality + 2 );
		if ( nAnimation >= 0 && pStats->animdescs.size() > nAnimation && !pStats->animdescs[nAnimation].anims.empty() && nFatality != -1 && nAnimIndex < pStats->animdescs[nAnimation].anims.size() )
			pKillInfo->timeToEndDieAnimation = timeOfVisDeath + pStats->animdescs[nAnimation].anims[nAnimIndex].nLength + 2 * SConsts::AI_SEGMENT_DURATION;
		else if ( nAnimation >= 0 && mech && mech->pAnimableModel )
		{
			animable_anims = GetVisObjAnimsFromModel( mech->pAnimableModel, NDb::EAnimationType( nAnimation ) );
			if ( nFatality != -1 && nAnimIndex >= 0 && nAnimIndex < animable_anims.size() )
				pKillInfo->timeToEndDieAnimation = timeOfVisDeath + animable_anims[nAnimIndex]->nLength + 2 * SConsts::AI_SEGMENT_DURATION;
			else
				pKillInfo->timeToEndDieAnimation = timeOfVisDeath + 2 * SConsts::AI_SEGMENT_DURATION;
		}
		else
			pKillInfo->timeToEndDieAnimation = timeOfVisDeath + 2 * SConsts::AI_SEGMENT_DURATION;
		if ( timeOfVisDeath == 0 )
		{
			pKillInfo->timeToEndDieAnimation = 0;
		}
	}

	pKillInfo->bRemoveCorpseAfterAnimation = bRemoveAmphCorpseInWater;
	pUnit->UnlockTiles();
	pUnit->FixUnlocking();

	if ( !pUnit->IsInfantry() )
		GetTerrain()->AddStaticObjectTiles( pKillInfo->lockedTiles );

	killed.push_back( pKillInfo );
	units.DeleteUnitFromMap( pUnit );
}

const int GetTileNum( const SVector &tile )
{
	return (tile.x << 12) | tile.y;
}

void CGraveyard::DelKilledUnitsFromBridge( const SRect &bridgeRect )
{
	std::list<SVector> rectTiles;
	GetAIMap()->GetTilesCoveredByRect( bridgeRect, &rectTiles );
	for ( std::list<SVector>::iterator iter = rectTiles.begin(); iter != rectTiles.end(); ++iter )
	{
		const SVector &tile = *iter;
		const int nTileNum = GetTileNum( tile );
		for ( std::list< CObj<CDeadUnit> >::iterator iter = bridgeDeadSoldiers[nTileNum].begin(); iter != bridgeDeadSoldiers[nTileNum].end(); ++iter )
		{
			CDeadUnit *pDeadUnit = *iter;
			updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pDeadUnit->GetDieObject(), -1 );
		}

		bridgeDeadSoldiers.erase( nTileNum );
	}
}

void CGraveyard::CheckSoonBeDead()
{
	// The quection is: to be or not to be?...
	std::vector<CAIUnit*> deadObjs{16};

	std::vector<std::pair<CObj<CAIUnit>, float>> soonToDie{16};
	for ( UpdateObjSet::iterator iter = soonBeDead.begin(); iter != soonBeDead.end(); ++iter )
		soonToDie.push_back(iter->second);
	
	std::sort(soonToDie.begin(), soonToDie.end(), [](std::pair<CObj<CAIUnit>, float> u1, std::pair<CObj<CAIUnit>, float> u2)
	{
		return u1.first->GetUniqueId() > u2.first->GetUniqueID();
	});

	for ( size_t i = 0; i < soonToDie.size(); i++ )
	{
		auto& iter = soonToDie[i];
		CAIUnit *pUnit = iter.first;
		if ( pUnit->GetTimeOfDeath() <= curTime )
		{
			const float fDamage = iter.second;
			NDb::EAnimationType eDeathAnimType = NDb::ANIMATION_DEATH;
			const int nFatality = pUnit->ChooseFatality( fDamage, &eDeathAnimType );
			const bool bPutMud = !GetTerrain()->IsBridge( pUnit->GetCenterTile() );
			//pUnit->CalcVisibility( true );
			
			if ( !pUnit->IsInSolidPlace() )
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_DEAD_UNIT, new CDeadUnit( pUnit, curTime, pUnit->GetDieAction(), nFatality, bPutMud, eDeathAnimType ), -1 );
				AddKilledUnit( pUnit, curTime, nFatality, eDeathAnimType );
			}
			else
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pUnit, 1 );
				pUnit->UnlockTiles();
				pUnit->UnfixUnlocking();
				units.DeleteUnitFromMap( pUnit );
			}

			deadObjs.push_back( pUnit );
		}
	}

	for ( std::vector<CAIUnit*>::iterator iter = deadObjs.begin(); iter != deadObjs.end(); ++iter )
		soonBeDead.erase( (*iter)->GetUniqueId() );
}

void CGraveyard::Clear()
{
	killed.clear();
	soonBeDead.clear();
	bridgeDeadSoldiers.clear();
	dissapeared.clear();
}

void CGraveyard::AddToDissapeared( CAIUnit *pUnit )
{
	dissapeared.push_back( pUnit );
}

void CGraveyard::AddToSoonBeDead( CAIUnit *pUnit, const float fDamage )
{
	const int nUniqueID = pUnit->GetUniqueID();
	soonBeDead[nUniqueID].first = pUnit;
	soonBeDead[nUniqueID].second = fDamage;
	if ( pUnit->IsVisible( theDipl.GetMyParty() ) )
		diedVisible[nUniqueID] = true;
}

void CGraveyard::AddBridgeKilledSoldier( const SVector &tile, CAIUnit *pSoldier )
{
	//CRAP{
	CDeadUnit *pDeadUnit = new CDeadUnit( pSoldier, 0, ACTION_NOTIFY_NONE, /*pSoldier->GetDBID()*/ 0, false, NDb::ANIMATION_DEATH );
	//CRAP}
	bridgeDeadSoldiers[GetTileNum( tile )].push_back( pDeadUnit );
}

//*******************************************************************
//*												CDeadUnit																	*
//*******************************************************************

CDeadUnit::CDeadUnit( CCommonUnit *_pDieObj, const NTimer::STime _dieTime, const EActionNotify _dieAction, bool _bPutMud, const int _nDeathAnimType )
: pDieObj( _pDieObj ), dieTime( _dieTime ), dieAction( _dieAction ), nFatality( -1 ), nDeathAnimType( _nDeathAnimType ), tileCenter( _pDieObj->GetCenterTile() ), bPutMud( _bPutMud )
{
	SetUniqueIdForObjects();
	bVisibleWhenDie = pDieObj->IsVisible( theDipl.GetMyParty() );
}

CDeadUnit::CDeadUnit( CCommonUnit *_pDieObj, const NTimer::STime _dieTime, const EActionNotify _dieAction, const int _nFatality, bool _bPutMud, const int _nDeathAnimType )
: pDieObj( _pDieObj ), dieTime( _dieTime ), dieAction( _dieAction ), nFatality( _nFatality ), nDeathAnimType( _nDeathAnimType ), tileCenter( _pDieObj->GetCenterTile() ), bPutMud( _bPutMud )
{
	SetUniqueIdForObjects();
	bVisibleWhenDie = pDieObj->IsVisible( theDipl.GetMyParty() );
}

const bool CDeadUnit::IsVisible( const BYTE cParty ) const
{
	return theWarFog.IsTileVisible( tileCenter, cParty );
}

void CDeadUnit::GetTilesForVisibility( CTilesSet *pTiles ) const
{
	pTiles->clear();
	if ( GetAIMap()->IsTileInside( tileCenter ) )
		pTiles->push_back( tileCenter );
}

bool CDeadUnit::ShouldSuspendAction( const EActionNotify &eAction ) const
{

	return
		( (
				eAction == ACTION_NOTIFY_DEAD_UNIT && curTime != 0 && 
				!theGraveyard.IsDiedVisible( pDieObj->GetUniqueID() )
			) || 
			eAction == ACTION_NOTIFY_GET_DEAD_UNITS_UPDATE ||
			eAction == ACTION_NOTIFY_NEW_ST_OBJ );
}

void CDeadUnit::GetDyingInfo( SAINotifyAction *pDyingInfo, bool *pbVisibleWhenDie )
{
	pDyingInfo->nObjUniqueID = pDieObj->GetUniqueId();
	pDyingInfo->time = dieTime;
	CAITimer::ToClientTime( &pDyingInfo->time );
	pDyingInfo->typeID = dieAction;
	// для disappeared units
	if ( dieAction != ACTION_NOTIFY_NONE )
	{
		if ( nFatality >= 0 )
			pDyingInfo->nParam = ( NDb::ANIMATION_DEATH_FATALITY << 16 ) | nFatality ;
		else
			pDyingInfo->nParam = ( nDeathAnimType << 16 ) | ( -nFatality - 2 );
	}
	else
	{
		if ( nFatality == -1 )
			nFatality =	WORD( -1 );
		
		pDyingInfo->nParam = nFatality;
	}

	if ( bPutMud )
		pDyingInfo->nParam |= 0x80000000;

	if ( pbVisibleWhenDie )
		*pbVisibleWhenDie = bVisibleWhenDie;
}

CUpdatableObj* CDeadUnit::GetDieObject() const 
{ 
	return pDieObj; 
}

bool CGraveyard::IsDiedVisible( int nUniqueID )
{
	return diedVisible.find( nUniqueID ) != diedVisible.end();
}

void CGraveyard::OnSerialize( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
		saver.Add( 6, &diedVisible );
	//CRAP{ for old saves support
	/*if ( saver.IsReading() && dissapeared.empty() )
	{
		list<CObj<CAIUnit> > oldDissapeared;
		saver.Add( 4, &oldDissapeared );
		for ( list<CObj<CAIUnit> >::const_iterator it = oldDissapeared.begin(); it != oldDissapeared.end(); ++it )
			dissapeared.push_back( *it );
	}*/
	//CRAP}
}

REGISTER_SAVELOAD_CLASS( 0x1108D4CB, CDeadUnit );
REGISTER_SAVELOAD_CLASS( 0x3015A500, SKilledUnit );

