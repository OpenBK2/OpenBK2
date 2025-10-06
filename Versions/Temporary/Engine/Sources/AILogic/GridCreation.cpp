#include "stdafx.h"

#include "GridCreation.h"
#include "AIUnit.h"
#include "GroupLogic.h"

#include "Misc/Checker.h"

#include <algorithm>

extern CGroupLogic theGroupLogic;

const float STANDART_HEIGHT = 250.0f;

//*******************************************************************
//*													CSortedGridUnits												*
//*******************************************************************

CSortedGridUnits* CSortedGridUnits::SUnitsCompare::pSortedUnits = 0;

CSortedGridUnits::CSortedGridUnits( )
: units( 10 )
{
	nUnits = 0;

	SUnitsCompare::pSortedUnits = this;
}

void CSortedGridUnits::AddUnit( CCommonUnit* pUnit )
{
	if ( units.size() >= nUnits - 1 )
		units.resize( nUnits * 2 );
	
	units[nUnits++] = pUnit;

	bSorted = false;
}

const CVec2 CSortedGridUnits::GetAverageDir() const
{
	return vAverageDir;
}

bool CSortedGridUnits::IsFormation( const int n ) const
{
	return units[n]->IsFormation();
}

bool CSortedGridUnits::SUnitsCompare::operator()( const int a, const int b ) const
{
	const bool bAFormation = pSortedUnits->IsFormation( a );
	const bool bBFormation = pSortedUnits->IsFormation( b );

	if ( bAFormation ^ bBFormation )
		return bBFormation;
	else
	{
		const CVec2 vACenter( pSortedUnits->rotatedUnitsCoord[a] );
		const CVec2 vBCenter( pSortedUnits->rotatedUnitsCoord[b] );
		const int nARow = fmod( vACenter.y, STANDART_HEIGHT );
		const int nBRow = fmod( vBCenter.y, STANDART_HEIGHT );
		
		return nARow > nBRow || nARow == nBRow && vACenter.x > vBCenter.x;
	}
}

void CSortedGridUnits::Sort()
{
	sortedUnitsNums.resize( nUnits, 0 );

	for ( int i = 0; i < nUnits; ++i )
		sortedUnitsNums[i] = i;

	const CVec2 vAverageDir( GetAverageDir() );
	const CVec2 vRotateAngle( vAverageDir.y, vAverageDir.x );
	rotatedUnitsCoord.resize( nUnits );

	float fMinCoord = 1e10;
	for ( int i = 0; i < nUnits; ++i )
	{
		rotatedUnitsCoord[i] = ( units[i]->GetCenterPlain() ) ^ vRotateAngle;
		fMinCoord = (std::min)( fMinCoord, rotatedUnitsCoord[i].y );
	}
	for ( int i = 0; i < nUnits; ++i )
		rotatedUnitsCoord[i].y -= fMinCoord;

	SUnitsCompare cmp;
	std::sort( sortedUnitsNums.begin(), sortedUnitsNums.end(), cmp );

	bSorted = true;
}

const CVec2 CSortedGridUnits::GetAABB( const int n ) const
{
	NI_ASSERT( bSorted == true, "Units aren't sorted" );
	CheckFixedRange( n, nUnits, "SortedGridUnits" );

	return units[sortedUnitsNums[n]]->GetAABBHalfSize();
}

const int CSortedGridUnits::GetUnitByOrderNumber( const int n ) const
{
	NI_ASSERT( bSorted == true, "Units aren't sorted" );
	CheckFixedRange( n, nUnits, "SortedGridUnits" );

	return sortedUnitsNums[n];
}

CCommonUnit* CSortedGridUnits::GetUnit( const int n ) const
{
	return units[sortedUnitsNums[n]];
}

//*******************************************************************
//*															CGrid																*
//*******************************************************************

CGrid* CGrid::SColumnCompare::pGrid;

CGrid::CGrid( const CVec2 &vGridCenter, const int nGroup, const CVec2 &vGridDir )
: vCenter( vGridCenter ), fMaxWidth( 0.0f )
{
	SColumnCompare::pGrid = this;
	for ( int i = theGroupLogic.BeginGroup( nGroup ); i != theGroupLogic.EndGroup(); i = theGroupLogic.Next( i ) )
	{
		CCommonUnit *pUnit = theGroupLogic.GetGroupUnit( i );

		if ( pUnit->CanMoveAfterUserCommand() )
			sortedUnits.AddUnit( pUnit );
	}

	CVec2 vNormGridDir( vGridDir );
	Normalize( &vNormGridDir );
	
	sortedUnits.SetDir( vNormGridDir );
	sortedUnits.Sort();
	
	CreateGrid();
}

bool CGrid::CanPlaceUnitToColumn( const int nColumn, const CVec2 &vAABBUnitSize )
{
	return true;
}

void CGrid::SetUnitCenterByColumn( const int nUnit, const int nColumn )
{
	const CVec2 vABBHalfSize = sortedUnits.GetAABB( nUnit );
	newCenters[nUnit].x = nColumn * GetColumnWidth() + fMaxWidth * 0.5f;
	newCenters[nUnit].y = columns[nColumn] - vABBHalfSize.y; // - GetBWColumnsSpace();
}

const float CGrid::GetBWColumnsSpace() const
{
	return fMaxWidth + SConsts::TILE_SIZE * 1.75f;
}

const float CGrid::GetRowHeight( const float fUnitHeight ) const
{
	return 2.0f * fUnitHeight + GetBWColumnsSpace();
}

const float CGrid::GetColumnWidth() const
{
	return fMaxWidth + GetBWColumnsSpace();
}

void CGrid::CalculateMaxWidth()
{
	fMaxWidth = 2.0f * SConsts::TILE_SIZE;
	for ( int i = 0; i < sortedUnits.GetSize(); ++i )
	{
		if ( !sortedUnits.IsFormation( i ) )
			fMaxWidth = (std::max)( fMaxWidth, 2.0f * sortedUnits.GetAABB( i ).x );
	}
}

void CGrid::CreateGrid()
{
	CalculateMaxWidth();
	
	float fUnitsSquare = 0.0f;
	for ( int i = 0; i < sortedUnits.GetSize(); ++i )
	{
		fUnitsSquare += GetColumnWidth() * GetRowHeight( sortedUnits.GetAABB( i ).y );
	}
	const int nColumns = ceil( sqrt( fUnitsSquare ) / GetColumnWidth() );

	//
	newCenters.resize( sortedUnits.GetSize() );
	columns.resize( nColumns, 0.0f );
	sortedColumns.resize( nColumns, 0 );
	for ( int i = 0; i < nColumns; ++i )
		sortedColumns[i] = i;

	//
	float fHeight = 0;
	float fWidth = 0;
	SColumnCompare compare;
	for ( int i = 0; i < sortedUnits.GetSize(); ++i )
	{
		const CVec2 vAABBHalfSize( sortedUnits.GetAABB( i ) );

		const int nColumn = *sortedColumns.begin();
		SetUnitCenterByColumn( sortedUnits.GetUnitByOrderNumber(i), nColumn );

		sortedColumns.erase( sortedColumns.begin() );
		columns[nColumn] -= GetRowHeight( vAABBHalfSize.y );

		sortedColumns.push_back( nColumn );
		std::sort( sortedColumns.begin(), sortedColumns.end(), compare );

		fHeight = (std::max)( fHeight, static_cast<float>(fabs( newCenters[sortedUnits.GetUnitByOrderNumber(i)].y )) );
		fWidth = (std::max)( fWidth, newCenters[sortedUnits.GetUnitByOrderNumber(i)].x + vAABBHalfSize.x );
	}

	//
	const CVec2 vDir = sortedUnits.GetAverageDir();
	CVec2 vDirOfTurn( vDir.y, -vDir.x );
	for ( int i = 0; i < sortedUnits.GetSize(); ++i )
	{
		newCenters[i].y += fHeight * 0.5f;
		newCenters[i].x -= fWidth * 0.5f;

		newCenters[i] ^= vDirOfTurn;
		newCenters[i] += vCenter;
	}
}

const int CGrid::GetNUnitsInGrid() const
{
	return sortedUnits.GetSize();
}

const CVec2 CGrid::GetUnitCenter( const int n ) const
{
	CheckRange( newCenters, n );
	return newCenters[n];
}

class CCommonUnit* CGrid::GetUnit( const int n ) const
{
	CheckRange( newCenters, n );
	return sortedUnits.GetUnit( n );
}

bool CGrid::SColumnCompare::operator()( const int a, const int b ) const
{
	return
		pGrid->columns[a] > pGrid->columns[b] ||
		pGrid->columns[a] == pGrid->columns[b] && a < b;

}


