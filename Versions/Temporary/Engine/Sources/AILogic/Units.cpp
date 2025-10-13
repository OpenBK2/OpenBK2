#include "stdafx.h"

#include "Units.h"
#include "Aviation.h"
#include "Formation.h"
#include "General.h"
#include "UnitsIterators.h"
#include "UnitsIterators2.h"

#include <cstdint>

REGISTER_SAVELOAD_CLASS( 0x1108D440, CUnits );

extern CSupremeBeing theSupremeBeing;
CUnits units;
extern CDiplomacy theDipl;
extern NTimer::STime curTime;

//*******************************************************************
//*													  CUnits																*
//*******************************************************************

const SVector GetLeveledCell( const SVector &bigCell, const int nCellLevel )
{
	return SVector( bigCell.x / (1 << (nCellLevel+1)), bigCell.y / (1 << (nCellLevel+1) ) );
}

const bool CUnits::IsUnitInCell( const int nUnitID ) const
{
	return posUnitInCell[nUnitID].nUnitPos != 0 || posUnitInCell[nUnitID].nCellID != 0;
}

void CUnits::Init()
{
  nUnitsOfType.resize( 3 );
	
	units.IncreaseListsNum( 3 + 1 );
	posUnitInCell.resize( SConsts::AI_START_VECTOR_SIZE );
	
	nBigCellsSizeX = GetAIMap()->GetSizeX() / SConsts::BIG_CELL_COEFF;
	nBigCellsSizeY = GetAIMap()->GetSizeY() / SConsts::BIG_CELL_COEFF;
	
	nUnitsCell.SetSizes( nBigCellsSizeX, nBigCellsSizeY );
	nUnitsCell.FillZero();

	nCell.SetSizes( nBigCellsSizeX, nBigCellsSizeY );
	nCell.FillZero();

	sizes.resize( 3, 0 );
	
	for ( int nVis = 0; nVis < 2; ++nVis )
	{
		for ( int nCellLevel = 0; nCellLevel < N_CELLS_LEVELS; ++nCellLevel )
		{
			for ( int nDipl = 0; nDipl < 3; ++nDipl )
			{
				for ( int nType = 0; nType < 2; ++nType )
				{
					numUnits[nVis][nCellLevel][nDipl][nType].SetSizes
					(
						nBigCellsSizeX / ( 1 << (nCellLevel+1) ) + 1,
						nBigCellsSizeY / ( 1 << (nCellLevel+1) ) + 1
					);

					numUnits[nVis][nCellLevel][nDipl][nType].FillZero();
				}
			}
		}
	}
	
	unitsInCells.resize( 2 );
}

void CUnits::CheckCorrectness( const SVector &tile )
{
	const SVector cell = AICellsTiles::GetBigCell( AICellsTiles::GetPointByTile( tile ) );

	int cnt = 0;
	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( !pUnit->IsInSolidPlace() )
		{
			const SVector unitCell = AICellsTiles::GetBigCell( pUnit->GetCenterPlain() );
			if ( unitCell == cell )
				++cnt;
		}
	}

	NI_ASSERT( cnt == nUnitsCell[cell.y][cell.x], "Wrong number of units in cell" );
}

void CUnits::AddUnitToLeveledCells( CAIUnit *pUnit, const SVector &bigCell, const int nVis )
{
	NI_ASSERT( nVis < 2, StrFmt( "Wrong nVis (%d)", nVis ) );

	const int nUnitParty = pUnit->GetParty();
	const bool bUnitMech = pUnit->IsMech();
	for ( int i = 0; i < N_CELLS_LEVELS; ++i )
	{
		const SVector leveledCell( GetLeveledCell( bigCell, i ) );
		++numUnits[nVis][i][nUnitParty][!bUnitMech][leveledCell.y][leveledCell.x];
	}
}

void CUnits::DelUnitFromLeveledCells( CAIUnit *pUnit, const SVector &bigCell, const int nVis )
{
	NI_ASSERT( nVis < 2, StrFmt( "Wrong nVis (%d)", nVis ) );

	const int nUnitParty = pUnit->GetParty();
	const bool bUnitMech = pUnit->IsMech();
	for ( int i = 0; i < N_CELLS_LEVELS; ++i )
	{
		const SVector leveledCell( GetLeveledCell( bigCell, i ) );
		--numUnits[nVis][i][nUnitParty][!bUnitMech][leveledCell.y][leveledCell.x];

		NI_ASSERT( numUnits[nVis][i][nUnitParty][!bUnitMech][leveledCell.y][leveledCell.x] >= 0, "Wrong number of units in leveled cell" );
	}
}

const int CUnits::GetVisIndex( CAIUnit *pUnit )
{
	const int nUnitParty = pUnit->GetParty();
	const int nOppositeParty = nUnitParty < 2 ? 1 - nUnitParty : nUnitParty;

	return pUnit->IsVisible( nOppositeParty ) || pUnit->IsRevealed();
}

void CUnits::AddUnitToConcreteCell( CAIUnit *pUnit, const SVector &cell, bool bWithLeveledCelles )
{
	const int nUnitUniqueID = pUnit->GetUniqueId();
	CIDsRemap::const_iterator pos = idsRemap.find( nUnitUniqueID );
	if ( pos == idsRemap.end() )
	{
		NI_ASSERT( false, "Unit added to cell without being added to units first" );
		return;
	}

	const int nUnitID = pos->second;
	const int nVisIndex = pUnit->GetNVisIndexInUnits();

	if ( IsUnitInCell( nUnitID ) )
	{
		// Re-adding without removing first leaves a stale unit entry in unitsInCells.
		// That stale entry can later produce duplicate collision/explosion candidates.
		DelUnitFromCell( pUnit, bWithLeveledCelles );
	}

	// если юнит единственный в свой ячейке, записать ячейку в список
	if ( ++nUnitsCell[cell.y][cell.x] == 1 )
		nCell[cell.y][cell.x] = cellsIds.Get();
	
	// добавить юнит в список стоящих на этой ячейке
	const int newId = nCell[cell.y][cell.x] * 2 * 3 + ( 2 * pUnit->GetParty() + uint8_t( pUnit->GetStats()->IsInfantry() ) ) + 1;

	if ( newId >= unitsInCells[0].GetListsNum() || newId >= unitsInCells[1].GetListsNum() )
	{
		unitsInCells[0].IncreaseListsNum( newId * 1.5 );
		unitsInCells[1].IncreaseListsNum( newId * 1.5 );
	}

//	NI_ASSERT( !IsUnitInCell( nUnitID ), "Unit is in cell, trying to add to another cell" );

	posUnitInCell[nUnitID].nCellID = newId;
	posUnitInCell[nUnitID].nUnitPos = unitsInCells[nVisIndex].Add( newId, nUnitID );
	posUnitInCell[nUnitID].cell = cell;

	if ( bWithLeveledCelles )
		AddUnitToLeveledCells( pUnit, cell, nVisIndex );

//	NI_ASSERT( unitsInCellsSet.find( nUnitID ) == unitsInCellsSet.end(), "Unit is in cell" );
	unitsInCellsSet.insert( nUnitID );
}

void CUnits::AddUnitToCell( CAIUnit *pUnit, const CVec2 &newPos, bool bWithLeveledCelles )
{
	const SVector bigCell( AICellsTiles::GetBigCell( newPos ) );
	if ( IsBigCellInside( bigCell ) )
		AddUnitToConcreteCell( pUnit, bigCell, bWithLeveledCelles );
}

void CUnits::AddUnitToCell( CAIUnit *pUnit, bool bWithLeveledCelles )
{
	const CVec2 vCenter( pUnit->GetCenterPlain() );
	const SVector bigCell = AICellsTiles::GetBigCell( vCenter );

	if ( IsBigCellInside( bigCell ) )
		AddUnitToConcreteCell( pUnit, bigCell, bWithLeveledCelles );
}

void CUnits::DelUnitFromCell( CAIUnit *pUnit, bool bWithLeveledCelles )
{
	const int nUnitUniqueID = pUnit->GetUniqueId();
	CIDsRemap::const_iterator pos = idsRemap.find( nUnitUniqueID );
	if ( pos == idsRemap.end() )
		return;
	const int nUnitID = pos->second;

	if ( IsUnitInCell( nUnitID ) )
	{
		SUnitPosition &unitPos = posUnitInCell[nUnitID];
		SVector &cell = unitPos.cell;
		const int nVisIndex = pUnit->GetNVisIndexInUnits();

		unitsInCells[nVisIndex].Erase( unitPos.nCellID, unitPos.nUnitPos );
		unitsInCells[nVisIndex].GetEl( unitPos.nUnitPos ) = 0;
		unitPos.nCellID = 0;
		unitPos.nUnitPos = 0;

		if ( --nUnitsCell[cell.y][cell.x] == 0 )
			cellsIds.Return( nCell[cell.y][cell.x] );

		if ( bWithLeveledCelles )
			DelUnitFromLeveledCells( pUnit, cell, nVisIndex );

		cell.x = 0;
		cell.y = 0;

		NI_ASSERT( unitsInCellsSet.find( nUnitID ) != unitsInCellsSet.end(), "Unit is not in cell" );
		unitsInCellsSet.erase( nUnitID );
	}
}

void CUnits::AddUnitToUnits( CAIUnit *pUnit, const int nPlayer, const int nUnitType )
{
	const int nParty = theDipl.GetNParty( nPlayer );
	NI_ASSERT( nParty >= 0 && nParty < 3, StrFmt( "Wrong number of player (%d)", nPlayer ) );
	NI_ASSERT( pUnit != 0, "epmty unit added to units" );
	const int nUniqueID = pUnit->GetUniqueId();
	idsRemap[nUniqueID] = units.Add( nParty, pUnit );
}

void CUnits::AddUnitToMap( CAIUnit *pUnit )
{
	if ( pUnit->GetStats()->IsAviation() )
		planes.push_back( checked_cast<CAviation*>(pUnit) );

	const int nUnitUniqueID = pUnit->GetUniqueId();

	CIDsRemap::const_iterator pos = idsRemap.find( nUnitUniqueID );
	if ( pos == idsRemap.end() )
	{
		NI_ASSERT( 0, "Unit added to map without being added to units first" );
		return;
	}
	const int nUnitID = pos->second;

	if ( nUnitID >= posUnitInCell.size() )
		posUnitInCell.resize( nUnitID * 1.5 );

	// нужно добавить в ячейку
	if ( !pUnit->IsInSolidPlace() )
		AddUnitToCell( pUnit, true );

	const int nParty = pUnit->GetParty();

	++sizes[nParty];

	const int nUnitType = pUnit->GetStats()->etype;
	if ( nUnitsOfType[nParty].find( nUnitType ) == nUnitsOfType[nParty].end() )
		nUnitsOfType[nParty][nUnitType] = 1;
	else
		++nUnitsOfType[nParty][nUnitType];
}

void CUnits::DeleteUnitFromMap( CAIUnit *pUnit )
{
	if ( pUnit->GetStats()->IsAviation() )
	{
		NI_ASSERT( find( planes.begin(), planes.end(), pUnit ) != planes.end(), "Can't find deleted plane in units" );
		planes.erase( find( planes.begin(), planes.end(), pUnit ) );
	}

	const int nUnitUniqueID = pUnit->GetUniqueId();
	
	CIDsRemap::const_iterator pos = idsRemap.find( nUnitUniqueID );
	if ( pos == idsRemap.end() )
		return;
	const int nUnitID = pos->second;

	// ещё не удалён из ячеек
	if ( units.GetEl( nUnitID ) != 0 )
	{
		DelUnitFromCell( pUnit, true );
//		units.GetEl( nUnitID ) = 0;
	}

	const int nType = pUnit->GetStats()->etype;
	--nUnitsOfType[pUnit->GetParty()][nType];
	--sizes[pUnit->GetParty()];
}

void CUnits::FullUnitDelete( CAIUnit *pUnit )
{
	const int nParty = pUnit->GetParty();
	NI_ASSERT( nParty >= 0 && nParty < 3, StrFmt( "Wrong number of player (%d)", nParty ) );

	const int nUnitUniqueID = pUnit->GetUniqueId();
	CIDsRemap::iterator pos = idsRemap.find( nUnitUniqueID );
	if ( pos != idsRemap.end() )
	{
		units.Erase( nParty, pos->second );
		units.GetEl( pos->second ) = 0;
		idsRemap.erase( pos );
	}
}

void CUnits::UnitChangedPosition( CAIUnit *pUnit, const CVec2 &newPos )
{
	const int nUnitUniqueID = pUnit->GetUniqueId();
	CIDsRemap::const_iterator pos = idsRemap.find( nUnitUniqueID );
	if ( pos == idsRemap.end() )
		return;
	const int nUnitID = pos->second;

	const SVector oldBigCell = posUnitInCell[nUnitID].cell;
	const SVector newBigCell = AICellsTiles::GetBigCell( newPos );
	const bool bInOldCell = IsUnitInCell( nUnitID );

	NI_ASSERT( !bInOldCell || IsBigCellInside( oldBigCell ), 
								StrFmt( "Wrong old big cell (%d,%d)", oldBigCell.x, oldBigCell.y ) );
	if ( oldBigCell != newBigCell || !bInOldCell )
	{
		//DEBUG{
		/*
#ifndef _FINALRELEASE
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 1, 
			StrFmt( "Unit %i ChangedPosition (%i, %i) > (%i, %i) TIME %i", pUnit->GetUniqueId(), oldBigCell.x, oldBigCell.y, newBigCell.x, newBigCell.y, curTime ) );
#endif
			*/
		//DEBUG}

		if ( bInOldCell ) 
			DelUnitFromCell( pUnit, false );

		AddUnitToCell( pUnit, newPos, false );
		const bool bInNewCell = IsUnitInCell( nUnitID );

		const int nVisIndex = pUnit->GetNVisIndexInUnits();
		NI_ASSERT( nVisIndex < 2, StrFmt( "Wrong nVis (%d)", nVisIndex ) );

		// leveled cells
		const int nUnitParty = pUnit->GetParty();
		const bool bUnitMech = pUnit->IsMech();
		for ( int i = 0; i < N_CELLS_LEVELS; ++i )
		{
			const SVector oldLeveledCell = GetLeveledCell( oldBigCell, i );
			const SVector newLeveledCell = GetLeveledCell( newBigCell, i );

			if ( oldLeveledCell != newLeveledCell || !bInOldCell || !bInNewCell )
			{
				if ( bInOldCell )
				{
					--numUnits[nVisIndex][i][nUnitParty][!bUnitMech][oldLeveledCell.y][oldLeveledCell.x];
					NI_ASSERT( numUnits[nVisIndex][i][nUnitParty][!bUnitMech][oldLeveledCell.y][oldLeveledCell.x] >= 0, "Wrong number of units in leveled cell" );
				}

				if ( bInNewCell )
					++numUnits[nVisIndex][i][nUnitParty][!bUnitMech][newLeveledCell.y][newLeveledCell.x];
			}
		}
	}

	//if ( AICellsTiles::GetGeneralCell( pUnit->GetCenterPlain() ) != AICellsTiles::GetGeneralCell( newPos ) )
	//no need to check, checked inside
	theSupremeBeing.UnitChangedPosition( pUnit, newPos );
}

CAIUnit* CUnits::operator[]( const int id )
{ 
	if ( id == 0 /*|| !units.GetEl( id )->IsRefValid() */)
		return 0;			// Cheat to avoid assert for trains
	CAIUnit * pUnit = units.GetEl( id );

//	NI_ASSERT( pUnit != 0, StrFmt("zero ID (id = %d)", id) );
	if ( pUnit == 0 )
		return 0;

	NI_ASSERT( pUnit->IsRefValid(), StrFmt("Invalid Ref (id = %d)", id) );
	if ( !pUnit->IsRefValid() )
		return 0;

	return pUnit; 
}

void CUnits::ChangePlayer( CAIUnit *pUnit, const uint8_t cNewPlayer )
{
	if ( pUnit && pUnit->IsAlive() && pUnit->GetPlayer() != cNewPlayer )
	{
		// чтобы не удалился		
		CObj<CAIUnit> pSaveUnit = pUnit;
		DeleteUnitFromMap( pUnit );
		FullUnitDelete( pUnit );

		pUnit->SetPlayer( cNewPlayer );
		AddUnitToUnits( pUnit, cNewPlayer, pUnit->GetStats()->etype );
		AddUnitToMap( pUnit );
	}
}

void CUnits::AddFormation( CFormation *pFormation )
{
	formations[pFormation->GetUniqueId()] = pFormation;
}

void CUnits::DelFormation( CFormation *pFormation )
{
	formations.erase( pFormation->GetUniqueId() );
}

const int CUnits::Size( const int nParty ) const
{
	NI_ASSERT( nParty < 3, "Wrong number of party" );
	return sizes[nParty];
}

const int CUnits::GetNSoldiers( const CVec2 &vCenter, const float fRadius, const int nParty )
{
	int cnt = 0;
	const float fRadius2 = sqr( fRadius );
	for ( CUnitsIter<0,0> iter( nParty, EDI_FRIEND, vCenter, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( IsValidObj( pUnit ) && pUnit->GetStats()->IsInfantry() )
		{
			const float fDist2 = fabs2( pUnit->GetCenterPlain() - vCenter );
			if ( fDist2 < fRadius2 )
				++cnt;
		}
	}

	return cnt;
}

const int CUnits::GetNUnits( const CVec2 &vCenter, const float fRadius, const int nParty )
{
	int cnt = 0;
	const float fRadius2 = sqr( fRadius );
	for ( CUnitsIter<0,0> iter( nParty, EDI_FRIEND, vCenter, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( !pUnit )
			continue;
		const float fDist2 = fabs2( pUnit->GetCenterPlain() - vCenter );
		if ( fDist2 < fRadius2 )
			++cnt;
	}

	return cnt;
}

void CUnits::CheckUnitCell()
{
	for ( int k = 0; k < 2; ++k )
	{
		for ( int i = 0; i < GetAIMap()->GetSizeY() / SConsts::BIG_CELL_COEFF; ++i )
		{
			for ( int j = 0; j < GetAIMap()->GetSizeX() / SConsts::BIG_CELL_COEFF; ++j )
			{
				int nCnt = 0;
				if ( nUnitsCell[i][j] != 0 )
				{
					for ( int cCurDipl = 0; cCurDipl < 3; ++cCurDipl )
					{
						for ( int cCurMech = 0; cCurMech < 2; ++cCurMech )
						{
							const int id = nCell[i][j] * 2 * 3 + 2 * cCurDipl + cCurMech + 1;
							int nIter = unitsInCells[k].begin( id );

							while ( nIter != unitsInCells[k].end() )
							{
								NI_ASSERT( unitsInCells[k].GetEl( nIter ).nValue != 0, "Wrong cell" );
								const int nOldIter = nIter;
								nIter = unitsInCells[k].GetNext( nIter );
								++nCnt;

								NI_ASSERT( nOldIter != nIter, "Wrong iter" );
							}
						}
					}
				}
				NI_ASSERT( nCnt == nUnitsCell[i][j], "Wrong number of units in cell" );
			}
		}
	}

	/*
	bool bOk = false;
	if ( !pUnit->IsInSolidPlace() && theStaticMap.IsPointInside( pUnit->GetCenter() ) )
	{
		const SVector cell = AICellsTiles::GetBigCell( pUnit->GetCenter() );
		const int id = nCell[cell.y][cell.x] * 2 * 3 + ( 2 * pUnit->GetParty() + uint8_t( pUnit->GetStats()->IsInfantry() ) ) + 1;

		for ( int i = unitsInCells.begin( id ); i != unitsInCells.end(); i = unitsInCells.GetNext( i ) )
		{
			const int unitID = unitsInCells.GetEl( i );
			if ( unitID == pUnit->GetUniqueId() )
				bOk = true;

			CAIUnit *pCellUnit = units.GetEl( unitID );
			SVector cell1 = AICellsTiles::GetBigCell( pCellUnit->GetCenter() );
			if ( cell != cell1 )
				DEBUG_BREAK;
		}
	}

	if ( !bOk )
		DEBUG_BREAK;
	*/
}

void CUnits::WriteSpatialCellDebugInfo( FILE* f )
{
	if ( !f )
		return;

	det_map<int, int> unitListHits;
	int nOccupiedCells = 0;
	int nListEntries = 0;
	int nBadListEntries = 0;
	int nCountMismatches = 0;

	fprintf( f, "Spatial cell list anomalies:\n" );
	for ( int y = 0; y < nBigCellsSizeY; ++y )
	{
		for ( int x = 0; x < nBigCellsSizeX; ++x )
		{
			if ( nUnitsCell[y][x] == 0 )
				continue;

			++nOccupiedCells;
			int nRealCellEntries = 0;
			const SVector cell( x, y );

			for ( int nVis = 0; nVis < 2; ++nVis )
			{
				for ( int nParty = 0; nParty < 3; ++nParty )
				{
					for ( int nInfantry = 0; nInfantry < 2; ++nInfantry )
					{
						const int nListID = nCell[y][x] * 2 * 3 + 2 * nParty + nInfantry + 1;
						for ( int nIter = unitsInCells[nVis].begin( nListID ); nIter != unitsInCells[nVis].end(); nIter = unitsInCells[nVis].GetNext( nIter ) )
						{
							++nRealCellEntries;
							++nListEntries;

							const int nUnitID = unitsInCells[nVis].GetEl( nIter ).nValue;
							++unitListHits[nUnitID];

							CAIUnit *pCellUnit = 0;
							bool bBadEntry = false;
							bool bWrongTrackedPos = false;
							bool bWrongActualCell = false;
							bool bSolidUnitInCell = false;
							int nUID = 0;
							int nPlayer = -1;
							SVector actualCell( 0, 0 );

							if ( nUnitID <= 0 || nUnitID >= (int)posUnitInCell.size() )
								bBadEntry = true;
							else
							{
								pCellUnit = (*this)[nUnitID];
								if ( !pCellUnit )
									bBadEntry = true;
								else
								{
									nUID = pCellUnit->GetUniqueId();
									nPlayer = pCellUnit->GetPlayer();
									actualCell = AICellsTiles::GetBigCell( pCellUnit->GetCenterPlain() );
									bWrongActualCell = !( actualCell == cell );
									bSolidUnitInCell = pCellUnit->IsInSolidPlace();
									bWrongTrackedPos = posUnitInCell[nUnitID].nCellID != nListID || posUnitInCell[nUnitID].nUnitPos != nIter;
								}
							}

							if ( bBadEntry || bWrongTrackedPos || bWrongActualCell || bSolidUnitInCell )
							{
								++nBadListEntries;
								fprintf( f, "\tCell(%d,%d) vis=%d party=%d infantry=%d list=%d iter=%d unitID=%d uid=%d player=%d actualCell=(%d,%d) bad=%d wrongTracked=%d wrongActualCell=%d solid=%d\n",
									x, y, nVis, nParty, nInfantry, nListID, nIter, nUnitID, nUID, nPlayer,
									actualCell.x, actualCell.y, bBadEntry, bWrongTrackedPos, bWrongActualCell, bSolidUnitInCell );
							}
						}
					}
				}
			}

			if ( nRealCellEntries != nUnitsCell[y][x] )
			{
				++nCountMismatches;
				fprintf( f, "\tCell(%d,%d) count mismatch: nUnitsCell=%d realListEntries=%d cellID=%d\n",
					x, y, (int)nUnitsCell[y][x], nRealCellEntries, (int)nCell[y][x] );
			}
		}
	}

	fprintf( f, "\tSummary: occupiedCells=%d listEntries=%d badEntries=%d countMismatches=%d\n",
		nOccupiedCells, nListEntries, nBadListEntries, nCountMismatches );
	fprintf( f, "\n" );

	fprintf( f, "Spatial cell membership [UID, UnitID, Player, Party, ActualCell, TrackedCell, CellID, UnitPos, VisIndex, InSet, ListHits]:\n" );
	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( !pUnit )
			continue;

		const int nUID = pUnit->GetUniqueId();
		CIDsRemap::const_iterator pos = idsRemap.find( nUID );
		const int nUnitID = pos != idsRemap.end() ? pos->second : 0;
		const SVector actualCell = AICellsTiles::GetBigCell( pUnit->GetCenterPlain() );
		const int nVisIndex = pUnit->GetNVisIndexInUnits();

		SVector trackedCell( 0, 0 );
		int nCellID = 0;
		int nUnitPos = 0;
		bool bInCell = false;
		if ( nUnitID > 0 && nUnitID < (int)posUnitInCell.size() )
		{
			bInCell = IsUnitInCell( nUnitID );
			trackedCell = posUnitInCell[nUnitID].cell;
			nCellID = posUnitInCell[nUnitID].nCellID;
			nUnitPos = posUnitInCell[nUnitID].nUnitPos;
		}

		const int nInSet = unitsInCellsSet.find( nUnitID ) != unitsInCellsSet.end();
		const int nListHits = unitListHits[nUnitID];
		fprintf( f, "\tUID=%d UnitID=%d Player=%d Party=%d actual=(%d,%d) tracked=(%d,%d) cellID=%d unitPos=%d vis=%d inSet=%d listHits=%d inSolid=%d inCell=%d\n",
			nUID, nUnitID, (int)pUnit->GetPlayer(), (int)pUnit->GetParty(),
			actualCell.x, actualCell.y, trackedCell.x, trackedCell.y, nCellID, nUnitPos, nVisIndex,
			nInSet, nListHits, pUnit->IsInSolidPlace(), bInCell );
	}
	fprintf( f, "\n" );
}

void CUnits::UpdateUnitVis4Enemy( CAIUnit *pUnit )
{
	const int nVisIndex = GetVisIndex( pUnit );
	const int nOldVisIndex = pUnit->GetNVisIndexInUnits();
	if ( nVisIndex != nOldVisIndex )
	{
		const int nUnitUniqueID = pUnit->GetUniqueId();
		const int nUnitID = idsRemap[nUnitUniqueID];

		if ( nUnitID && IsUnitInCell( nUnitID ) )
		{
			unitsInCells[nOldVisIndex].Erase( posUnitInCell[nUnitID].nCellID, posUnitInCell[nUnitID].nUnitPos );
			unitsInCells[nOldVisIndex].GetEl( posUnitInCell[nUnitID].nUnitPos ) = 0;
			posUnitInCell[nUnitID].nUnitPos = unitsInCells[nVisIndex].Add( posUnitInCell[nUnitID].nCellID, nUnitID );

			DelUnitFromLeveledCells( pUnit, posUnitInCell[nUnitID].cell, nOldVisIndex );
			AddUnitToLeveledCells( pUnit, posUnitInCell[nUnitID].cell, nVisIndex );

			pUnit->SetNVisIndexInUnits( nVisIndex );
		}
	}
}

void CUnits::ApplyModifierToAll( const NDb::SUnitStatsModifier *pBonus, const bool bForward )
{
	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;

		pUnit->ApplyStatsModifier( pBonus, bForward );
		pUnit->WarFogChanged();
	}
}

