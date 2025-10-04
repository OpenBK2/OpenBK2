#include "stdafx.h"

#include "Resistance.h"
#include "GeneralConsts.h"
#include "AIUnitInfoForGeneral.h"
#include "AIUnit.h"
#include "UnitCreation.h"

extern CUnitCreation theUnitCreation;

//#include "..\Scene\Statistics.h"

bool IsCellInExcludingCircle( const CVec2 &vCellCenter, const CCircle &circle )
{
	return fabs2( vCellCenter - circle.center ) <= sqr( circle.r + SGeneralConsts::RESISTANCE_CELL_SIZE * SConsts::TILE_SIZE / 3 );
}

const CVec2 SResistance::GetResistanceCellCenter( const int nCell )
{
	const SVector cell( nCell >> 12, nCell & 0xfff );
	const SVector tile( cell.x * SGeneralConsts::RESISTANCE_CELL_SIZE + SGeneralConsts::RESISTANCE_CELL_SIZE / 2, cell.y * SGeneralConsts::RESISTANCE_CELL_SIZE + SGeneralConsts::RESISTANCE_CELL_SIZE / 2 );
	return AICellsTiles::GetPointByTile( tile );
}

const float GetWeightOfUnit( const SUnitBaseRPGStats* pStats )
{
	NI_VERIFY( pStats != 0, "", return 0 );

	const float fArmorMember = 100.0f / ( 2.0f + pStats->GetMinPossibleArmor( RPG_TOP ) );
	const float fPrice = pStats->fPrice;
	const float fUnitTypeCoeff = pStats->IsInfantry() ? 0.1f : 1.0f;

	return 
		fUnitTypeCoeff * ( 1.0f * fPrice + 1.0f * fArmorMember );
}

const float GetCoeffByDeltaTime( const float fDeltaTime, const float fUpperBound, bool bNeedDeinstall, bool bCanMove )
{
	float fCoeff = 1.0f - ( fDeltaTime / fUpperBound ) / 3.0f;
	if ( bNeedDeinstall )
		fCoeff = Min( 1.0f, fCoeff * 1.2f );
	if ( !bCanMove )
		fCoeff = Min( 1.0f, fCoeff * 1.1f );

	return fCoeff;
}

const float GetWeightByVisiblePos( CAIUnit *pUnit, const NTimer::STime deltaTime )
{
	const float fTimeToForgetUnit = pUnit->GetTimeToForget();
	if ( deltaTime > fTimeToForgetUnit )
		return 0.0f;
	else
	{
		// коэффициент, обеспечивающий вероятность того, что юнит ушёл
		const float fTimeCoeff = GetCoeffByDeltaTime( deltaTime, fTimeToForgetUnit, pUnit->NeedDeinstall(), pUnit->CanMove() );
		const float fWeight = GetWeightOfUnit( pUnit->GetStats() );
		const float fFreeCoeff = pUnit->GetCover();
		const float fResult = fWeight * fTimeCoeff * fFreeCoeff;

		return fResult;
	}
}

const float GetWeightByAntiArtPos( CAIUnit *pUnit, const NTimer::STime deltaTime, const float fDistToLastVisibleAntiArt )
{
	const float fTimeToForget = SGeneralConsts::TIME_TO_FORGET_ANTI_ARTILLERY;
	if ( deltaTime > fTimeToForget )
		return 0.0f;
	else
	{
		// коэффициент, обеспечивающий вероятность того, что юнит ушёл
		const float fTimeCoeff = GetCoeffByDeltaTime( deltaTime, fTimeToForget, pUnit->NeedDeinstall(), pUnit->CanMove() );
		// коэффициент, отвечающий за точность круга антиартиллерии
//		const float fAccuracyCoeff = Max( 1.0f, fDistToLastVisibleAntiArt / 320.0f );
		// вес юнита
		const float fWeight = GetWeightOfUnit( theUnitCreation.Get152mmML20Stats() );

		return 
			fWeight * fTimeCoeff;// * fAccuracyCoeff;
	}
}

//*******************************************************************
//*											CResistancesContainer												*
//*******************************************************************

const int CResistancesContainer::GetResistanceCellNumber( const CVec2 &vPos )
{
	const SVector tile = AICellsTiles::GetTile( vPos );
	const SVector cell( tile.x / SGeneralConsts::RESISTANCE_CELL_SIZE, tile.y / SGeneralConsts::RESISTANCE_CELL_SIZE );

	return ( cell.x << 12 ) | cell.y;
}

void CResistancesContainer::UpdateEnemyUnitInfo(
	CAIUnitInfoForGeneral *pInfo,
	const NTimer::STime lastVisibleTimeDelta, const CVec2 &vLastVisiblePos,
	const NTimer::STime lastAntiArtTimeDelta, const CVec2 &vLastVisibleAntiArtCenter, const float fDistToLastVisibleAntiArt )
{
	if ( !pInfo->GetOwner()->GetStats()->IsAviation() && ( vLastVisiblePos != VNULL2 || fDistToLastVisibleAntiArt != -1.0f ) )
	{
		const int nLastCell = GetResistanceCellNumber( pInfo->GetRegisteredPos() );

		if ( cellsWeights.find( nLastCell ) != cellsWeights.end() )
		{
			CResistance::iterator pos = find( resistances.begin(), resistances.end(), SResistance( nLastCell, 0, 0 ) );
			const float fNew = Max( 0.0f, cellsWeights[nLastCell].fCellWeight - pInfo->GetWeight() );
			cellsWeights[nLastCell].fCellWeight = fNew;
			if ( pos != resistances.end() )
				pos->SetWeight( fNew );
			if ( pInfo->GetWeight() > SGeneralConsts::MIN_UNIT_WEIGHT_TO_SEND_GUNPLANES ) 
				--goodWeight[nLastCell];
		}

		CVec2 vNewPos;
		float fNewWeight;
		// считаем по видимой позиции
		if ( lastVisibleTimeDelta <= lastAntiArtTimeDelta || fDistToLastVisibleAntiArt == -1.0f )
		{
			vNewPos = vLastVisiblePos;
			fNewWeight = GetWeightByVisiblePos( pInfo->GetOwner(), lastVisibleTimeDelta );
		}
		// считаем по позиции круга антиартиллерийской борьбы
		else
		{
			vNewPos = vLastVisibleAntiArtCenter;
			fNewWeight = GetWeightByAntiArtPos( pInfo->GetOwner(), lastAntiArtTimeDelta, fDistToLastVisibleAntiArt );
		}

		pInfo->SetWeight( fNewWeight );
		pInfo->SetRegisteredPos( vNewPos );
		
		const int nNewCell = GetResistanceCellNumber( vNewPos );
		CCellsWeights::iterator posCells = cellsWeights.find( nNewCell );
		
		if ( posCells == cellsWeights.end() )
		{
			cellsWeights[nNewCell] = SSellInfo( fNewWeight, false, !IsCellExcluded( SResistance::GetResistanceCellCenter( nNewCell ) ) );
			posCells = cellsWeights.find( nNewCell );
		}
		else
			posCells->second.fCellWeight += fNewWeight;

		if ( fNewWeight > SGeneralConsts::MIN_UNIT_WEIGHT_TO_SEND_GUNPLANES ) 
			++goodWeight[nLastCell];

		CResistance::iterator posRes = find( resistances.begin(), resistances.end(), SResistance( nNewCell, 0, 0 ) );
		if ( posRes == resistances.end() )
			resistances.push_back( SResistance( nNewCell, posCells->second.fCellWeight, goodWeight[nLastCell] ) );
		else
			posRes->SetWeight( posCells->second.fCellWeight );
	}

	for ( CResistance::iterator iter = resistances.begin(); iter != resistances.end(); ++iter )
	{
		for ( CResistance::iterator iter1 = resistances.begin(); iter1 != resistances.end(); ++iter1 )
		{
				if ( iter != iter1 )
			{
				NI_ASSERT( iter->GetCellNumber() != iter1->GetCellNumber(), "Same resistance cells" );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
void CResistancesContainer::UnitDied( CAIUnitInfoForGeneral *pInfo )
{
	if ( !pInfo->GetOwner()->GetStats()->IsAviation() )
	{
		const int nCell = GetResistanceCellNumber( pInfo->GetRegisteredPos() );
		CResistance::iterator pos = find( resistances.begin(), resistances.end(), SResistance( nCell, 0, 0 ) );
		if ( pos == resistances.end() )
			return;
		cellsWeights[nCell].fCellWeight = Max( 0.0f, cellsWeights[nCell].fCellWeight - pInfo->GetWeight() );
		pos->SetWeight( cellsWeights[nCell].fCellWeight	);
		if ( pInfo->GetWeight() > SGeneralConsts::MIN_UNIT_WEIGHT_TO_SEND_GUNPLANES ) 
			--goodWeight[nCell];
	}
}

void CResistancesContainer::UnitChangedParty( CAIUnitInfoForGeneral *pInfo )
{
	if ( !pInfo->GetOwner()->GetStats()->IsAviation() )
	{
		const int nCell = GetResistanceCellNumber( pInfo->GetRegisteredPos() );
		CResistance::iterator pos = find( resistances.begin(), resistances.end(), SResistance( nCell, 0, 0 ) );
		if ( pos == resistances.end() )
			return;

		cellsWeights[nCell].fCellWeight = Max( 0.0f, cellsWeights[nCell].fCellWeight - pInfo->GetWeight() );
		pos->SetWeight( cellsWeights[nCell].fCellWeight	);
		if ( pInfo->GetWeight() > SGeneralConsts::MIN_UNIT_WEIGHT_TO_SEND_GUNPLANES ) 
			--goodWeight[nCell];
	}
}

void CResistancesContainer::SetCellInUse( const int nResistanceCellNumber, bool bInUse )
{
	if ( cellsWeights.find( nResistanceCellNumber ) != cellsWeights.end() )
		cellsWeights[nResistanceCellNumber].bInUse = bInUse;
}

void CResistancesContainer::RemoveExcluded( const CVec2 &vCenter )
{
	/*
	list<CCircle>::iterator it = excluded.remove_if(  
		binder1st( ptr_mem_fun( CCircle::GetCenter() ),
		equal_to<CVec2>( vCenter ) );
*/
	list<CCircle>::iterator it = excluded.begin();
	while ( it != excluded.end() )
	{
		const CCircle &circle = *it;
		if ( circle.center == vCenter )
		{
			for ( CCellsWeights::iterator iter = cellsWeights.begin(); iter != cellsWeights.end(); ++iter )
			{
				if ( IsCellInExcludingCircle( SResistance::GetResistanceCellCenter( iter->first ), circle ) )
					iter->second.bAllowShoot = true;
			}

			it = excluded.erase( it );
		}
		else
			++it;
	}
}

void CResistancesContainer::AddExcluded( const CVec2 &vCenter, const float fRadius )
{
	excluded.push_back( CCircle( vCenter, fRadius ) );
}

bool CResistancesContainer::IsCellExcluded( const CVec2 &vCellCenter )
{
	for ( list<CCircle>::const_iterator it = excluded.begin(); it != excluded.end(); ++it )
		if ( IsCellInExcludingCircle( vCellCenter, *it ) )
			return true;
	return false;
}

bool CResistancesContainer::IsInUse( const int nResistanceCellNumber )
{
	CCellsWeights::iterator iter = cellsWeights.find( nResistanceCellNumber );
	if ( iter != cellsWeights.end() )
	{
		const SSellInfo &cell = iter->second;
		return cell.bInUse || !cell.bAllowShoot;
	}
	else
		return false;
}

bool CResistancesContainer::IsInResistanceCircle( const CVec2 &vCenter ) const
{
	list<CCircle>::const_iterator iter = excluded.begin();
	while ( iter != excluded.end() && fabs2( vCenter - iter->center ) >= sqr( iter->r ) )
		++iter;

	return iter != excluded.end();
}

//*******************************************************************
//*										CResistancesContainer::CIter									*
//*******************************************************************

void CResistancesContainer::CIter::IterateToNotInUse()
{
	while ( iter != pContainter->resistances.end() && pContainter->IsInUse( (*iter).GetCellNumber() ) )
		++iter;
}

void CResistancesContainer::CIter::Iterate()
{
	if ( iter != pContainter->resistances.end() )
	{
		++iter;
		IterateToNotInUse();
	}
}


