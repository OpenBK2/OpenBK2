#include "stdafx.h"

#include "AIUnit.h"
#include "AIUnitInfoForGeneral.h"
#include "Diplomacy.h"
#include "GeneralConsts.h"
#include "GeneralInternal.h"
#include "ScenarioTracker.h"
#include "Scripts.h"

CSupremeBeing theSupremeBeing;

extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CScripts *pScripts;

//*******************************************************************
//*											CSupremeBeing																*
//*******************************************************************

struct IEnemyContainer *CSupremeBeing::GetEnemyConatiner( int nPlayer )
{
	const auto it = generals.find( nPlayer );
	return it == generals.end() ? nullptr : it->second.GetPtr();
}

void CSupremeBeing::SetAAVisible( class CAIUnit *pUnit, const int nGeneralParty, const bool bVisible )
{
	for ( const auto &entry : generals )
	{
		if ( entry.second->GetParty() == nGeneralParty )
		{
			entry.second->SetAAVisible( pUnit, bVisible );
		}
	}
}

void CSupremeBeing::SetUnitVisible( class CAIUnit *pUnit, const int nGeneralParty, const bool bVisible )
{
	for ( const auto &entry : generals )
	{
		if ( entry.second->GetParty() == nGeneralParty )
		{
			entry.second->SetUnitVisible( pUnit, bVisible );
		}
	}
}

bool CSupremeBeing::MustShootToObstacles( const int nPlayer )
{
	return false;
}

void CSupremeBeing::Clear()
{
	delayedTasks.clear();
	generals.clear();
	ironmans.clear();
}

void CSupremeBeing::Init( const NDb::SMapInfo *pMapInfo )
{
	Clear();
	const bool bCooperative = GetScenarioTracker() && GetScenarioTracker()->IsCooperative();
	if ( !pMapInfo || ( theDipl.IsNetGame() ? !bCooperative : NGlobal::GetVar( "nogeneral", 0 ) != 0 ) )
	{
		return;
	}
	// Local developer overrides must never change a network simulation.
	SGeneralConsts::Init( bCooperative );
	for ( int i = 0; i < pMapInfo->players.size() && i < theDipl.GetNeutralPlayer(); ++i )
	{
		const int nParty = theDipl.GetNParty( i );
		if ( bCooperative ? theDipl.IsAIPlayer( i ) && theDipl.IsPlayerExist( i ) : nParty == 1 && generals.empty() )
		{
			generals[i] = new CGeneral( i, nParty );
			generals[i]->Init( pMapInfo->players[i].general );
		}
	}
}

void CSupremeBeing::GiveNewUnitsToGenerals( const std::list<CCommonUnit *> &pUnits, bool bFromReinforcement )
{
	for ( const auto &entry : generals )
	{
		entry.second->GiveNewUnits( pUnits, bFromReinforcement );
		// Initial fog notifications can precede General creation. Seed visible map
		// units now, and also observe newly spawned enemy/neutral units in later batches.
		std::vector<CAIUnit *> visibleUnits;
		for ( CCommonUnit *pUnit : pUnits )
		{
			CAIUnit *pAIUnit = dynamic_cast<CAIUnit *>( pUnit );
			if ( pAIUnit && pAIUnit->IsRefValid() && pAIUnit->IsAlive() && pAIUnit->IsVisible( entry.second->GetParty() ) )
			{
				visibleUnits.push_back( pAIUnit );
			}
		}
		std::sort( visibleUnits.begin(),
		           visibleUnits.end(),
		           []( const CAIUnit *a, const CAIUnit *b ) { return a->GetUniqueId() < b->GetUniqueId(); } );
		for ( CAIUnit *pUnit : visibleUnits )
		{
			entry.second->SetUnitVisible( pUnit, true );
		}
	}
}

void CSupremeBeing::AddIronman( const int nScriptGroup )
{
	ironmans.insert( nScriptGroup );
}

bool CSupremeBeing::IsIronman( const int nScriptGroup ) const
{
	return ironmans.find( nScriptGroup ) != ironmans.end();
}

bool CSupremeBeing::IsMobileReinforcement( int nPlayer, int nGroup ) const
{
	for ( const auto &entry : generals )
	{
		if ( theDipl.IsNetGame() ? entry.first == nPlayer : entry.second->GetParty() == theDipl.GetNParty( nPlayer ) )
		{
			return entry.second->IsMobileReinforcement( nGroup );
		}
	}
	return false;
}

void CSupremeBeing::AddReinforcement( class CAIUnit *pUnit )
{
	for ( const auto &entry : generals )
	{
		if ( entry.second->OwnsUnit( pUnit ) )
		{
			entry.second->GiveNewUnits( { pUnit }, true );
		}
	}
}

void CSupremeBeing::Segment()
{
	if ( theDipl.IsNetGame() || !NGlobal::GetVar( "temp.nogeneral_sript", 0 ) )
	{
		if ( IsActive() )
		{
			// запуск отложенных заждач
			for ( DelayedTasks::iterator it = delayedTasks.begin(); it != delayedTasks.end(); )
			{
				IGeneralDelayedTask *pTask = *it;
				if ( pTask && pTask->IsTimeToRun() )
				{
					pTask->Run();
					it = delayedTasks.erase( it );
				}
				else
				{
					++it;
				}
			}

			// вызывать не каждый сегмент и разнести по сегментам.
			for ( Generals::iterator it = generals.begin(); it != generals.end(); ++it )
			{
				it->second->Segment();
			}
		}
	}
}

void CSupremeBeing::RegisterDelayedTask( IGeneralDelayedTask *pTask )
{
	delayedTasks.push_back( pTask );
}

void CSupremeBeing::UpdateEnemyUnitInfo( CAIUnitInfoForGeneral *pInfo,
                                         const NTimer::STime lastVisibleTimeDelta,
                                         const CVec2 &vLastVisiblePos,
                                         const NTimer::STime lastAntiArtTimeDelta,
                                         const CVec2 &vLastVisibleAntiArtCenter,
                                         const float fDistToLastVisibleAntiArt )
{
	for ( const auto &entry : generals )
	{
		if ( theDipl.GetDiplStatusForParties( entry.second->GetParty(), pInfo->GetOwner()->GetParty() ) != EDI_ENEMY )
		{
			continue;
		}
		entry.second->UpdateEnemyUnitInfo( pInfo,
		                                   lastVisibleTimeDelta,
		                                   vLastVisiblePos,
		                                   lastAntiArtTimeDelta,
		                                   vLastVisibleAntiArtCenter,
		                                   fDistToLastVisibleAntiArt );
	}
}

void CSupremeBeing::UnitDied( class CCommonUnit *pUnit )
{
	for ( Generals::iterator it = generals.begin(); it != generals.end(); ++it )
	{
		it->second->UnitDied( pUnit );
	}
}

void CSupremeBeing::UnitDied( CAIUnitInfoForGeneral *pInfo )
{
	for ( Generals::iterator it = generals.begin(); it != generals.end(); ++it )
	{
		it->second->UnitDied( pInfo );
	}
}

void CSupremeBeing::UnitChangedPosition( class CCommonUnit *pUnit, const CVec2 &vNewPos )
{
	if ( AICellsTiles::GetGeneralCell( pUnit->GetCenterPlain() ) != AICellsTiles::GetGeneralCell( vNewPos ) )
	{
		for ( const auto &entry : generals )
		{
			if ( entry.second->OwnsUnit( pUnit ) )
			{
				entry.second->UnitChangedPosition( pUnit, vNewPos );
			}
		}
	}
}

void CSupremeBeing::UnitAskedForResupply( class CCommonUnit *pUnit, const EResupplyType eType, const bool bSet )
{
	if ( !pScripts || IsIronman( pScripts->GetScriptID( pUnit ) ) )
	{
		return;
	}

	if ( pUnit->GetParty() == theDipl.GetNeutralParty() )
	{
		for ( Generals::iterator it = generals.begin(); it != generals.end(); ++it )
		{
			it->second->UnitAskedForResupply( pUnit, eType, bSet );
		}
	}
	else
	{
		for ( const auto &entry : generals )
		{
			if ( entry.second->OwnsUnit( pUnit ) )
			{
				entry.second->UnitAskedForResupply( pUnit, eType, bSet );
			}
		}
	}
}

void CSupremeBeing::UnitChangedParty( CAIUnit *pUnit, const int nNewParty )
{
	//	const int nGeneralParty = 1 - pUnit->GetParty();
	//if ( nGeneralParty == 1 && generals.find( nGeneralParty ) != generals.end() )
	//generals[nGeneralParty]->UnitChangedParty( pUnit, nNewParty );
	for ( Generals::iterator it = generals.begin(); it != generals.end(); ++it )
	{
		it->second->UnitChangedParty( pUnit, nNewParty );
	}
}

bool CSupremeBeing::IsInResistanceCircle( const CVec2 &vPoint, const int nGeneralParty )
{
	for ( const auto &entry : generals )
	{
		if ( entry.second->GetParty() == nGeneralParty && entry.second->IsInResistanceCircle( vPoint ) )
		{
			return true;
		}
	}
	return false;
}
