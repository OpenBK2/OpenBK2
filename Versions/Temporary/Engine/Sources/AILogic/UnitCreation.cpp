#include "stdafx.h"

#include "UnitCreation.h"
#include "GroupLogic.h"
#include "Common_RTS_AI/AIMap.h"
#include "Formation.h"
#include "Soldier.h"
#include "NewUpdater.h"
#include "Artillery.h"
#include "AIClassesID.h"
#include "StaticObjects.h"
#include "Weather.h"
#include "Aviation.h"
#include "Scripts.h"
#include "ScenarioTracker.h"
#include "Graveyard.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "ExecutorFormationFirstAid.h"
#include "ExecutorContainer.h"
#include "Statistics.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "UnitTorpedo.h"
#include "UnitsIterators.h"
#include "BalanceTest.h"
#include "DBAIConsts.h"
#include "RailRoads.h"
#include "GlobalWarFog.h"

extern CExecutorContainer theExecutorContainer;
extern CWeather theWeather;
extern CScripts *pScripts;
extern CStaticObjects theStatObjs;
extern CUnits units;
extern CGroupLogic theGroupLogic;
extern CEventUpdater updater;
CUnitCreation theUnitCreation;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CStatistics theStatistics;
extern CGraveyard theGraveyard;
extern CBalanceTest theBalanceTest;
extern SRailRoadSystem theRailRoadSystem;
extern CGlobalWarFog theWarFog;

void CLightPlaneCreation::CalcPositions( const int nMax, const CVec2 & box, const CVec2 & direction, std::vector<CVec2> * positions, CVec2 * offset, const bool bRandom )
{
	/* таким образом
		1
		 2

		3
		 4

		5
		 6

		..
			..
	*/

	NI_ASSERT( nMax , "рассчеты делаются для положительного числа только" );
	positions->clear();

	CVec2 vStartOffset( 0, 0 );
	const float resize = SConsts::PLANES_SMALL_FORMATION_SIZE;

	for ( int i=0; i< nMax; ++i )
	{
		if ( i % 2 == 0 )
			positions->push_back( vStartOffset /*^ direction*/ );
		else
		{
			positions->push_back( (vStartOffset + CVec2( -resize * box.y, resize / 2 * box.x ) ) ^ direction ) ;

			vStartOffset += CVec2( -resize * 3 * box.y, 0);
		}
	}

	*offset = ( CVec2( resize * box.y, 0 ) * nMax / 2 )/*^direction*/;
}

void CHeavyPlaneCreation::CalcPositions( const int nMax, const CVec2 &box, const CVec2 &direction, std::vector<CVec2> *positions, CVec2 *offset, bool bRandom )
{
	/*
	таким образом

					1
				5		6
			11	2		12
		..	7		8		..
			..	3		..
				9		10
					4
	*/
	NI_ASSERT( nMax , "рассчеты делаются для положительного числа только" );
	int nSize = sqrt( float(nMax-1.0f) ) + 1;
	positions->clear();

	int iLimit = nSize;
	CVec2 vStartOffset( 0, 0 );// смещение первой точки ( 1, 5, 11 )
	
	const float resize = SConsts::PLANES_HEAVY_FORMATION_SIZE;

	*offset = ( CVec2( resize * box.y, 0 ) * int(nSize/2) )^direction;

	while ( iLimit > 0 )
	{
		// последовательно заполняем диагонали
		for ( int i = 0; i < iLimit; ++i )
		{
			CVec2 res ( CVec2( vStartOffset.x, vStartOffset.y)+ CVec2( -resize * i * box.y, 0 ) );
			if ( bRandom )
				res.x -= box.y * NRandom::Random( 0.0f, SConsts::PLANES_START_RANDOM );

			positions->push_back( res/*^direction*/ );
			if ( positions->size() == nMax )
				return ;
			// не центровые - по 2 раза
			if ( iLimit != nSize )
			{
				CVec2 res ( CVec2( vStartOffset.x, -vStartOffset.y )+ CVec2( -resize * i * box.y, 0 ) );
				if ( bRandom )
					res.x -= box.y * NRandom::Random( 0.0f, SConsts::PLANES_START_RANDOM );
				positions->push_back( res /*^ direction*/ );
				if ( positions->size() == nMax )
					return ;
			}
		}
		vStartOffset += CVec2( -resize / 2 * box.y, resize / 2 * box.x );
		--iLimit;
	}
}


//STankPitInfo


const SMechUnitRPGStats * GetRandomTankPit( const STankPitInfo &info, const class CVec2 &vSize, const bool bCanDig, float *pfResize )
{
	const std::vector< CDBPtr<SMechUnitRPGStats> > * pPits = bCanDig ? &info.digTankPits : &info.sandBagTankPits;

	int nBestIndex = -1;
	float fDiff = 0;

	for ( int i = 0; i < pPits->size(); ++i )
	{
		const SMechUnitRPGStats * pStats = (*pPits)[i];
		const float fResize = vSize.y / pStats->vAABBHalfSize.y;
		const float fCurDiff = fabs( fResize - 1 );
		if ( -1 == nBestIndex || fDiff > fCurDiff )
		{
			*pfResize = fResize;
			fDiff = fCurDiff;
			nBestIndex = i;
		}
	}

	NI_ASSERT( nBestIndex != -1, "cannot find any tankpit in tankpits.xml" );
	return (*pPits)[nBestIndex];
}

CUnitCreation::CUnitCreation()
{
	bInit = false;
	feedbacks.push_back( SFeedBack(EFB_SCOUT_ENABLED,				EFB_SCOUT_DISABLED) );
	feedbacks.push_back( SFeedBack(EFB_FIGHTERS_ENABLED,		EFB_FIGHTERS_DISABLED) );
	feedbacks.push_back( SFeedBack(EFB_PARADROPS_ENABLED,		EFB_PARADROPERS_DISABLED) );
	feedbacks.push_back( SFeedBack(EFB_BOMBERS_ENABLED,			EFB_BOMBERS_DISABLED) );
	feedbacks.push_back( SFeedBack(EFB_SHTURMOVIKS_ENABLED,	EFB_SHTURMOVIKS_DISABLED) );
}

void CUnitCreation::Segment()
{
}

void SLocalInGameUnitCreationInfo::Copy( const SMapPlayerInfo &playerInfo )
{
}

SLocalInGameUnitCreationInfo::SLocalInGameUnitCreationInfo( const SMapPlayerInfo &rSUnitCreation )
{
	Copy( rSUnitCreation  );
}

SLocalInGameUnitCreationInfo & SLocalInGameUnitCreationInfo::operator=( const SMapPlayerInfo &rSUnitCreation  )
{
	Copy( rSUnitCreation );
	return *this;
}

void CUnitCreation::Init()
{
	bInit = false;
}

void CUnitCreation::SetConsts( const NDb::SAIGameConsts *_pConsts )
{
	pConsts = _pConsts;
}

void CUnitCreation::Init( const struct SMapInfo* pMapInfo, ICollisionsCollector *_pCollisionsCollector )
{
	inGameUnits.clear();
	partyDependentInfo.clear();
	pCollisionsCollector = _pCollisionsCollector;
	pCurrentMap = pMapInfo;

	IAIScenarioTracker *pScenarioTracker = Singleton<IAIScenarioTracker>();
	//NI_ASSERT( pScenarioTracker, "PRG: Scenario Tracker not present at the time of UnitCreation init" );
	if ( pScenarioTracker )
	{
		for ( int i = 0; i < pMapInfo->players.size(); ++i )
		{
			inGameUnits.push_back( pMapInfo->players[i] );
			partyDependentInfo.push_back( pScenarioTracker->GetPlayerParty( i ) );
		}
	}
	else
	{
		for ( std::vector<SMapPlayerInfo>::const_iterator it = pMapInfo->players.begin(); it != pMapInfo->players.end(); ++it )
		{
			inGameUnits.push_back( *it );
			partyDependentInfo.push_back( it->pPartyInfo );
		}
	}

	bInit = true;
}

void CUnitCreation::Clear()
{
	inGameUnits.clear();
	pCollisionsCollector = 0;
}

const SPartyDependentInfo * CUnitCreation::GetPartyDependentInfo( const int nDipl ) const 
{
	NI_ASSERT( nDipl < partyDependentInfo.size(), StrFmt("Wrong party Number ( %i )", nDipl ) );
	return partyDependentInfo[nDipl];
}

CMineStaticObject* CUnitCreation::CreateMine( const enum EMineType nType, const class CVec3 &vPoint, const int nPlayer )
{
	NI_ASSERT( nPlayer < inGameUnits.size(), StrFmt( "no mines for player %d", nPlayer ) );
	
//	const SPartyDependentInfo *pInfo = GetPartyDependentInfo( nDipl );

	//статы мин брать из структуры, задаваемой в карте
	const SMineRPGStats *pMineStats =0;
	switch ( nType )
	{
	case MT_INFANTRY_AND_TECHNICS:
		pMineStats = pConsts->common.pMineUniversal;
		break;
	case MT_INFANTRY:
		pMineStats = pConsts->common.pMineAP;
		break;
	case MT_TECHNICS:
		pMineStats = pConsts->common.pMineAT;
		break;
	case MT_CHARGE:
		pMineStats = pConsts->common.pMineCharge;
		break;
	case MT_LANDMINE:
		pMineStats = pConsts->common.pLandMine;
		break;
	default: 
		NI_ASSERT( false, StrFmt( "wrong mine type %d", nType ) );
		return 0;
	}

	return theStatObjs.AddNewMine( pMineStats, 1, vPoint, -1, nPlayer );
}

void CUnitCreation::SendFormationToWorld( CFormation * pUnit, const bool bSelectable ) const
{
	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pUnit, -1 );

	for ( int j = 0; j < pUnit->Size(); ++j )
	{
		CAIUnit *pSold = (*pUnit)[j];
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, pSold, -1 );
		updater.AddUpdate( 0, pSold->GetIdleAction(), pSold, -1 );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pSold, -1 );
	}
	pUnit->SetSelectable( bSelectable, true );
}

CFormation* CUnitCreation::CreateParatroopers( const CVec3 &where, CAIUnit *pPlane, const int nScriptID )const
{
	const int nDipl = pPlane->GetPlayer();
	
	NI_ASSERT( nDipl < inGameUnits.size(), StrFmt( "wrong player %d", nDipl ) );

	const SSquadRPGStats *pStats = inGameUnits[pPlane->GetPlayer()].pParatrooper;
	if ( !pStats )
		return 0;
	
	const WORD wDir = pPlane->GetDirection();
	const int nFormation = 0;
	float fHP = 1.0f;

	CPtr<CFormation> formation = checked_cast<CFormation*>( AddNewFormation( pStats, nFormation, fHP, where.x, where.y, where.z, wDir, nDipl,false,false, -1, pPlane->GetReinforcementType() ) );
		
	for ( int i = 0; i < formation->Size(); ++i )
	{
		(*formation)[i]->SetToSolidPlace();
		pScripts->AddObjToScriptGroup( (*formation)[i], nScriptID );
	}
	
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PARACHUTE ), formation, false );
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_PARADE, -1.0f ), formation, true );
	

	return formation;
}

CFormation* CUnitCreation::CreateResupplyEngineers( CAITransportUnit *pWithUnit )const
{
	NI_ASSERT( pWithUnit->GetPlayer() < inGameUnits.size(), StrFmt( "wrong player %d",pWithUnit->GetPlayer()) );
	
	const SSquadRPGStats *pStats = GetPartyDependentInfo( pWithUnit->GetPlayer())->pResupplyEngineerSquad;
	
	if ( !pStats )
		return 0;

	const WORD wDir = 0;
	const int nFormation = 0;
	float fHP = 1.0f;
	const int nPlayer = pWithUnit->GetPlayer();
	const CVec2 where = pWithUnit->GetEntrancePoint();

	CPtr<CFormation> pFormation = checked_cast<CFormation*>( AddNewFormation( pStats, nFormation, fHP, where.x, where.y, GetHeights()->GetZ( where.x, where.y ), wDir, nPlayer, false, false, -1, pWithUnit->GetReinforcementType() ) );

	if ( pFormation )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CPtr<CSoldier> pS = (*pFormation)[i];
			pS->SetToSolidPlace();
		}
		pFormation->SetInTransport( pWithUnit );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pWithUnit->GetUniqueId() ), pFormation, false );
	}
	return pFormation ;
}

CFormation * CUnitCreation::CreateCrew( CArtillery *pUnit, const int nUnits, const CVec3 &vPos, const int _nPlayer, const bool bImmidiateAttach )const
{
	NI_ASSERT( (_nPlayer == -1 ? pUnit->GetPlayer() : _nPlayer ) < inGameUnits.size(), StrFmt( "wrong player GetPlayer = %d, nPlayer = %d", pUnit->GetPlayer(), _nPlayer ) );
	
	CVec3 vCreatePos (vPos);
	if ( -1 == vPos.x )
		vCreatePos = pUnit->GetCenter();

	// если не задан игрок ручками, то используем того-же, что и у юнита
	const int nPlayer = ( _nPlayer == -1 ) ? pUnit->GetPlayer() : _nPlayer;

	if ( !GetPartyDependentInfo(nPlayer) )
		return 0;

	// для пулеметов - свой взвод - пулеметчики
	const SSquadRPGStats * pStats = ( RPG_TYPE_ART_HEAVY_MG == pUnit->GetStats()->etype ) ? 
													GetPartyDependentInfo(nPlayer)->pHeavyMachinegunSquad :
													GetPartyDependentInfo(nPlayer)->pGunCrewSquad;

	if ( RPG_TYPE_ART_AAGUN == pUnit->GetStats()->etype && GetPartyDependentInfo(nPlayer)->pAAGunSquad )
		pStats =  GetPartyDependentInfo(nPlayer)->pAAGunSquad;

	if ( !pStats )
		return 0;

	CFormation * pForm = checked_cast<CFormation*>
											 ( theUnitCreation.AddNewFormation(	pStats,
																		0, 1.0f, vCreatePos.x, vCreatePos.y, vCreatePos.z,
																		0, nPlayer, false, true, nUnits, pUnit->GetReinforcementType() )
										   );

	if ( pForm->Size() == 0 )
	{
		CPtr<CFormation> pToDelete = pForm;
		return 0;
	}
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_CATCH_ARTILLERY, pUnit->GetUniqueId()), pForm, false );
	pUnit->SetCapturingUnit( pForm );
	if ( bImmidiateAttach )
		pUnit->SetCrew( pForm );
	pForm->SetSelectable( false, true );
	return pForm;
}

class CAIUnit *CUnitCreation::CreateTorpedo( class CAIUnit *pOwner, const NDb::SWeaponRPGStats *pWeaponStats, const class CVec2 &vSource, const class CVec2 &vTarget )
{
	CPtr<CUnitTorpedo> pUnit = checked_cast<CUnitTorpedo*>( MakeObject<CAIUnit>( AI_TORPEDO ) );

	if ( !pUnit )
		return 0;

	const int nUniqueUnitID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;

	pUnit->SetUniqueIdForUnits( nUniqueUnitID );

	WORD wDir = GetDirectionByVector( vTarget - vSource );
	const NDb::SMechUnitRPGStats *pStats = pConsts->common.pTorpedoStats;

	const BYTE nPlayer = theDipl.GetNeutralPlayer();

	pUnit->Init( vSource, GetHeights()->GetZ( vSource ), pOwner, pWeaponStats, pStats, 
		pStats->fMaxHP, wDir, nPlayer, pCollisionsCollector );

	units.AddUnitToUnits( pUnit, nPlayer, pStats->etype );
	pUnit->Mem2UniqueIdObjs();
	units.AddUnitToMap( pUnit );

	CAIUnit::CheckCmdsSize( pUnit->GetUniqueId() );

	SWarFogUnitInfo fogInfo;
	pUnit->GetFogInfo( &fogInfo );
	theWarFog.AddUnit( pUnit->GetUniqueId(), fogInfo, pUnit->GetParty() );

	pUnit->SetSelectable( false, true );		// Torpedo cannot be selected

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pUnit, -1 );

	return pUnit;
}

int CUnitCreation::AddNewUnit( const int nUniqueID, const SUnitBaseRPGStats *pStats, 
															const float fHPFactor, const int x, const int y, const int z, const WORD dir,
															const BYTE player, 
															bool bInitialization, bool bSendToWorld, const NDb::EReinforcementType eType ) const
{
	CPtr<CAIUnit> pUnit;
	if ( pStats->IsInfantry() )
	{
		pUnit = pStats->etype == RPG_TYPE_SNIPER ?
						MakeObject<CAIUnit>( AI_SNIPER ) : MakeObject<CAIUnit>( AI_INFANTRY );
	}
	else if ( pStats->IsAviation() )
		pUnit = MakeObject<CAIUnit>( AI_AVIATION );
	else if ( pStats->IsTransport() )
		pUnit = MakeObject<CAIUnit>( AI_TRANSPORT_UNIT );
	else if ( pStats->IsArtillery() /*|| pStats->etype == RPG_TYPE_TRAIN_SUPER*/ )
		pUnit = MakeObject<CAIUnit>( AI_ARTILLERY );
	else if ( pStats->etype == RPG_TYPE_TRAIN_LOCOMOTIVE || pStats->etype == RPG_TYPE_TRAIN_ARMOR )
	{
		if ( theRailRoadSystem.segments.empty() )
			return 0;
		pUnit = MakeObject<CAIUnit>( AI_LOCOMOTIVE );
	}
	else if ( pStats->IsTrain() )
	{
		if ( theRailRoadSystem.segments.empty() )
			return 0;
		pUnit = MakeObject<CAIUnit>( AI_TRAIN_CAR );
	}
	else if ( pStats->IsArmor() || pStats->IsSPG() )
		pUnit = MakeObject<CAIUnit>( AI_TANK );
	else
		NI_ASSERT( false, StrFmt( "Unknown unit's type (DBID \"%s\", type %d)", NDb::GetResName(pStats), pStats->etype ) );
	NI_ASSERT( pUnit != 0, "cannot create unit" );
	if ( pUnit == 0 )
		return 0;

	pUnit->SetUniqueIdForUnits( nUniqueID );

	pUnit->Init( CVec2( x, y ), z, pStats, pStats->fMaxHP * fHPFactor, dir, player, pCollisionsCollector );
	units.AddUnitToUnits( pUnit, player, pStats->etype );
	pUnit->Mem2UniqueIdObjs();
	pUnit->SetReinforcementType( eType );

	units.AddUnitToMap( pUnit );

	// Apply nighttime modifier at night
	if ( pCurrentMap->eDayTime == NDb::DAY_NIGHT )
	{
		pUnit->ApplyStatsModifier( pConsts->common.pNightStatModifier, true );
	}

	// Apply weather modifier, if weather is active
	if ( theWeather.IsActive() )
	{
		pUnit->ApplyStatsModifier( pConsts->common.pBadWeatherStatModifier, true );
	}

	if ( GetScenarioTracker() )
	{
		// Apply XP level modifier
		IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();
		for ( int i = 0; i < pConsts->common.expLevels.size(); ++i )
		{
			const NDb::SAIExpLevel *pLevels = pConsts->common.expLevels[i];
			if ( !pLevels )
				continue;
			if ( pLevels->eDBType != pUnit->GetReinforcementType() )
				continue;
			int nXPLevel = pScenarioTracker->GetReinforcementXPLevel( player, eType );
			nXPLevel = Min<int>( pLevels->levels.size() - 1, nXPLevel );
			if ( nXPLevel < 0 )
				break;

			if ( pLevels->levels[nXPLevel].pStatsBonus )
			{
				pUnit->ApplyStatsModifier( pLevels->levels[nXPLevel].pStatsBonus, true );
				break;
			}
		}

		// Apply leader XP level modifier
		pUnit->ApplyStatsModifier( pScenarioTracker->GetLeaderModifier( player, pUnit->GetReinforcementType() ), true );

		// Apply difficulty modifier
		switch( theDipl.GetNParty( player ) )
		{
		case 0:
			pUnit->ApplyStatsModifier( pScenarioTracker->GetPlayerDifficultyModifier(), true );
			pUnit->ApplyStatsModifier( pScenarioTracker->GetPlayerChapterModifier( eType ), true );
			break;
		case 1:
			pUnit->ApplyStatsModifier( pScenarioTracker->GetEnemyDifficultyModifier(), true );
			break;
		}
	}
	pUnit->ApplyStatsModifier( theBalanceTest.GetModifier( player ), true );
	CAIUnit::CheckCmdsSize( pUnit->GetUniqueId() );

	if ( bSendToWorld )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pUnit, -1 );
		updater.AddUpdate( 0, pUnit->GetIdleAction(), pUnit, -1 );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pUnit, -1 );
		pUnit->InitSpecialAbilities();
	}
	pUnit->LockTiles();
	pUnit->CalcVisibility( true );

	SWarFogUnitInfo fogInfo;
	pUnit->GetFogInfo( &fogInfo );
	theWarFog.AddUnit( pUnit->GetUniqueId(), fogInfo, pUnit->GetParty() );
	
	// по умолчанию все наши юниты - селектятся
	pUnit->SetSelectable( pUnit->GetHitPoints() != 0.0f && player == theDipl.GetMyNumber() &&
												pStats->etype != RPG_TYPE_AVIA_BOMBER, bSendToWorld );
	
	if ( pUnit->GetHitPoints() != 0.0f && pStats->IsArtillery() && theDipl.GetNeutralPlayer() != player )
	{
		CArtillery *pArt = static_cast_ptr<CArtillery*>( pUnit );
		if ( pArt->MustHaveCrewToOperate() )
		{
			if ( !CreateCrew( pArt, -1, CVec3(-1,-1,-1), -1, true ) )
				pArt->ChangePlayer( theDipl.GetNeutralPlayer() );
		}
		else
			pArt->SetOperable( 1.0f ); 
	}

	if ( pUnit->GetHitPoints() == 0.0f )
	{
		pUnit->PrepareToDelete();
		const int nFatality = pUnit->ChooseFatality( 0 );
		const bool bPutMud = !GetTerrain()->IsBridge( pUnit->GetCenterTile() );
		updater.AddUpdate( 0, ACTION_NOTIFY_DEAD_UNIT, new CDeadUnit( pUnit, 0, pUnit->GetDieAction(), nFatality, bPutMud ), -1 );
		theGraveyard.AddKilledUnit( pUnit, 0, nFatality );
	}

	const int nRes = pUnit->GetUniqueId();
	return nRes;
}

CVec2 CUnitCreation::GetRandomAppearPoint( const int _nPlayer, const bool bLeave ) const
{
	return VNULL2;
}


const SObjectBaseRPGStats*CUnitCreation::GetRandomAntitankObject() const 
{
	return pConsts == 0 ? 0 : pConsts->common.antitankObjects[0];
}

const SEntrenchmentRPGStats* CUnitCreation::GetEntrenchment() const
{
	return pConsts == 0 ? 0 : pConsts->common.pEntrenchment;
}

const SFenceRPGStats *CUnitCreation::GetWireFence() const 
{
	return pConsts == 0 ? 0 : pConsts->common.pAPFence;
}

const SMechUnitRPGStats * CUnitCreation::GetFoxHole( const bool bCanDig ) const
{
	return pConsts == 0 ? 0 : pConsts->foxHoles[NRandom::Random( pConsts->foxHoles.size() )];
}

const SMechUnitRPGStats * CUnitCreation::GetRandomTankPit( const class CVec2 &vSize, const bool bCanDig, float *pfResize ) const
{
	return ::GetRandomTankPit( pConsts->tankPits, vSize, bCanDig, pfResize );	
}

void CUnitCreation::GetCentersOfAllFormationUnits( const SSquadRPGStats *pStats, const CVec2 &vFormCenter, const WORD wFormDir, const int nFormation, const int nUnits, std::list<CVec2> *pCenters ) const
{
	const SSquadRPGStats::SFormation &formation = pStats->formations[nFormation];
	const int nSizeOfFormation = (nUnits == -1) ? formation.order.size() : nUnits;

	CVec2 vRelFormDir = GetVectorByDirection( wFormDir );
	std::swap( vRelFormDir.x, vRelFormDir.y );
	vRelFormDir.y = -vRelFormDir.y;
	
	for ( int j = 0; j < nSizeOfFormation; ++j )
	{
		CVec2 vUnitCenter( vFormCenter + ((formation.order[j].vPos) ^ vRelFormDir) );
		vUnitCenter.x = Clamp( vUnitCenter.x, 0.0f, (float)(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vUnitCenter.y = Clamp( vUnitCenter.y, 0.0f, (float)(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );

		pCenters->push_back( vUnitCenter );
	}
}

CCommonUnit* CUnitCreation::AddNewFormation( const SSquadRPGStats *pStats, const int _nFormation, const float fHP, const float x, const float y, const float z, const WORD wDir, const int nDiplomacy, bool bInitialization, bool bSendToWorld, const int nUnits, NDb::EReinforcementType eType ) const
{
	NI_ASSERT( _nFormation < pStats->formations.size(), "" );
	int nFormation = 0;
	if ( _nFormation < pStats->formations.size() )
		nFormation = _nFormation;
#ifdef _DO_ASSERT_SLOW
	for ( int i = 0; i < pStats->members.size(); ++i ) 
	{
		if ( pStats->members[i] == 0 )
		{
			NI_ASSERT( pStats->members[i] != 0, StrFmt("Squad \"%s\" has ZERO member %d", NDb::GetResName(pStats), i) );
			return 0;
		}
	}
#endif

	const SSquadRPGStats::SFormation &formation = pStats->formations[nFormation];
	CObj<CFormation> pFormation = new CFormation();
	const CVec2 vFormCenter( x, y );
	pFormation->Init( pStats, vFormCenter, z, wDir, pCollisionsCollector );
	
	if ( bSendToWorld )
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pFormation, -1 );

	std::list<CVec2> centers;
	GetCentersOfAllFormationUnits( pStats, vFormCenter, wDir, nFormation, nUnits, &centers );

	// по слотам конфигурации
	const int nSizeOfFormation = Min( formation.order.size(), (nUnits == -1) ? formation.order.size() : nUnits );
	std::list<CVec2>::iterator iter = centers.begin();
	for ( int j = 0; j < nSizeOfFormation; ++j, ++iter )
	{
		NI_ASSERT( iter != centers.end(), "Centers of units of formation incorrectly initialized" );

		const CVec2 unitCoord( *iter );
		const WORD wUnitDir = wDir + formation.order[j].nDir;

		const int nUniqueUnitID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
		const int id = AddNewUnit( nUniqueUnitID, pStats->members[j], fHP, unitCoord.x, unitCoord.y, z, wUnitDir, nDiplomacy, bInit, bSendToWorld, eType );
		CObjectBase *pUnit = CAIUnit::GetUnitByUniqueID( id );
		NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit in formation" );
		pFormation->AddNewUnitToSlot( checked_cast<CSoldier*>( pUnit ), /* j, */ bSendToWorld );
		if ( bSendToWorld )
			updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, checked_cast<CSoldier*>( pUnit ), -1 );
	}

	// create formation ability executors
	bool bFirstAid = false;
	for ( int i = 0; i < pFormation->Size() && !bFirstAid; ++i )
	{
		CSoldier * pSold = (*pFormation)[i];
		const NDb::SUnitBaseRPGStats *pSt = pSold->GetStats();
		const int nLevel = Min<int>( 1 + theStatistics.GetAbilityLevel( pSold->GetPlayer(), pSold->GetReinforcementType()), pSt->GetActions()->specialAbilities.size() );
		for ( int nAb = 0; nAb < nLevel; ++nAb )
		{
			if ( pSt->GetActions()->specialAbilities[nAb]->eName == NDb::ABILITY_FIRST_AID )
			{
				bFirstAid = true;
				break;
			}
		}
	}
	if ( bFirstAid )
	{
		CPtr<CExecutorFormationFirstAid> pFA = new CExecutorFormationFirstAid( pFormation );
		pFA->RegisterOnEvents( &theExecutorContainer );
		theExecutorContainer.Add( pFA );
	}

	NI_ASSERT( iter == centers.end(), "Centers of units of formation incorrectly initialized" );

	pFormation->ChangeGeometry( nFormation );
	pFormation->MoveGeometries2Center();
	pFormation->TurnToDirection( wDir, false, true );

	pFormation->AddAvailCmd( ACTION_COMMAND_FOLLOW );

	// т.к. центр масс сдвинулся, координаты солдат поменялись
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		CVec2 vNewCoord( pFormation->GetUnitCoord( (*pFormation)[i] ) );
		vNewCoord.x = Clamp( vNewCoord.x, 0.0f, (float)(GetAIMap()->GetSizeX() * GetAIMap()->GetTileSize() - 1) );
		vNewCoord.y = Clamp( vNewCoord.y, 0.0f, (float)(GetAIMap()->GetSizeY() * GetAIMap()->GetTileSize() - 1) );

		(*pFormation)[i]->SetCenter( CVec3( vNewCoord, GetHeights()->GetZ( vNewCoord ) ) );
	}
	
	// по умолчанию все наши юниты - селектятся
	pFormation->SetSelectable( nDiplomacy == theDipl.GetMyNumber(), bSendToWorld );

	if ( pFormation->Size() == 0 )
	{
		pFormation->Disappear();
		return 0;
	}
	return pFormation;
}

CCommonUnit* CUnitCreation::CreateSingleUnitFormation( CSoldier *pSoldier ) const
{
	const SSquadRPGStats *pStats = GetSingleUnitFormation();
	
	CObj<CFormation> pFormation = new CFormation();
	pFormation->Init( pStats, pSoldier->GetCenterPlain(), pSoldier->GetCenter().z, pSoldier->GetFrontDirection(), pCollisionsCollector );
	pFormation->Mem2UniqueIdObjs();
	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pFormation, -1 );

	pFormation->AddSoldier( pSoldier );

	pFormation->ChangeGeometry( 0 );
	pFormation->MoveGeometries2Center();
	pFormation->AddAvailCmd( ACTION_COMMAND_FORM_FORMATION );
	pFormation->AddAvailCmd( ACTION_COMMAND_FOLLOW );

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, pSoldier, -1 );

	return pFormation;
}

bool CUnitCreation::IsAntiTank( const SHPObjectRPGStats *pStats ) const
{
	for ( int i = 0; i < pConsts->common.antitankObjects.size(); ++i )
	{
		if ( pStats == pConsts->common.antitankObjects[i] )
			return true;
	}

	return false;
}

bool CUnitCreation::IsAPFence( const SHPObjectRPGStats *pStats ) const
{
	return pStats == pConsts->common.pAPFence;
}

const NDb::SAIExpLevel * CUnitCreation::GetExpLevels( const enum EUnitRPGType eType )
{
	for ( int i = 0; i < pConsts->common.expLevels.size(); ++i )
	{
		if ( eType == pConsts->common.expLevels[i]->eType )
			return pConsts->common.expLevels[i];
	}
	NI_ASSERT( false, StrFmt( "cannot find expLevels for UnitRPGType %i", eType ) );
	return pConsts->common.expLevels[0];
}

CExistingObject* CUnitCreation::CreatePlayerFlag( int nPlayer, CStaticObject *pSample )
{
	if ( !pSample )
		return 0;
	IAIScenarioTracker* pScenarioTracker = GetScenarioTracker();
	if ( pScenarioTracker && pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		CDBPtr<NDb::SObjectBaseRPGStats> pFlag = pScenarioTracker->GetKeyBuildingFlagObject( nPlayer );

		if ( pFlag )
			return checked_cast<CExistingObject*>( theStatObjs.AddNewStaticObject( pFlag, 1.0f, pSample->GetCenter(), pSample->GetDir(), 0 ) );
		else
			return 0;
	}
	if ( !partyDependentInfo[nPlayer]->pKeyBuildingFlag )
		return 0;
	return checked_cast<CExistingObject*>( theStatObjs.AddNewStaticObject( partyDependentInfo[nPlayer]->pKeyBuildingFlag, 1.0f, pSample->GetCenter(), pSample->GetDir(), 0 ) );
}

const NDb::SVisObj *CUnitCreation::GetParatrooperVisObj( int nPlayer ) const 
{ 
	return partyDependentInfo[nPlayer]->pParatrooperVisObj; 
}

const NDb::SVisObj *CUnitCreation::GetParachuteVisObj() const
{
	return pConsts->pParachute;
}

int CUnitCreation::GetRemoveParachuteTime() const
{
	return pConsts->nRemoveParachuteTime;
}

const NDb::SUnitBaseRPGStats * CUnitCreation::GetMosinStats() const { return pConsts->common.pMosinStats; }

const NDb::SUnitBaseRPGStats * CUnitCreation::Get152mmML20Stats() const { return pConsts->common.pG152mmML20Stats; }

const NDb::SSquadRPGStats* CUnitCreation::GetSingleUnitFormation() const { return pConsts->common.pSingleUnitFormation; }

const NDb::SStaticObjectRPGStats *CUnitCreation::GetShellBox() const  { return pConsts->common.pShellBox; }

void CUnitCreation::ApplyWeatherModifier( const bool bForward )
{
	if ( bForward )
	{
		// Find planes and send them back
		for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;

			if ( pUnit->GetStats()->IsAviation() && pUnit->GetUnitAbilityDesc( NDb::ABILITY_MASTER_PILOT ) == 0 )
			{
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pUnit, false );
				pUnit->SetSelectable( false, true );
			}
		}
	}

	units.ApplyModifierToAll( pConsts->common.pBadWeatherStatModifier, bForward );
}


