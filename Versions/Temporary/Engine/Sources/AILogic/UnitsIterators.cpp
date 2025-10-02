#include "stdafx.h"

#include "UnitsIterators.h"
#include "../Misc/Bresenham.h"

//*******************************************************************
//*													CGlobalIter															*
//*******************************************************************

void CGlobalIter::Init( const BYTE cStartDipl, const BYTE cFilter )
{
	nParties = 0;
	for ( int i = 0; i < 3; ++i )
	{
		if ( theDipl.GetDiplStatusForParties( cStartDipl, i ) & cFilter )
			parties[nParties++] = i;
	}

	nCurParty = 0; 
	if ( nParties == 0 )
		iter = 0;
	else
	{
		iter = units.units.begin( parties[nCurParty] );
		while ( nCurParty < nParties && ( iter == 0 || units.units.GetEl(iter) == 0 ) )
		{
			if ( iter != 0 )
				iter = units.units.GetNext( iter );
			else
			{
				++nCurParty;			

				if ( nCurParty < nParties )
					iter = units.units.begin( parties[nCurParty] );
			}
		}
	}
	
	visitedUnits.clear();
}

void CGlobalIter::Iterate()
{
	iter = units.units.GetNext( iter );

	// something went wrong
	if ( visitedUnits.find( iter ) != visitedUnits.end() )
		iter = 0;

	visitedUnits.insert( iter );

	while ( ( iter == 0 || units.units.GetEl(iter) == 0 ) && nCurParty < nParties )
	{
		if ( iter != 0 )
			iter = units.units.GetNext( iter );
		else
		{
			++nCurParty;

			if ( nCurParty < nParties )
				iter = units.units.begin( parties[nCurParty] );
		}
	}
}

CAIUnit* CGlobalIter::operator*() const 
{ 
	return units.units.GetEl(iter);
}

//*******************************************************************
//*													CLineIter																*
//*******************************************************************

bool CLineIter::Cells::operator() ( long x, long y ) 
{ 
	const SVector v( x, y );
	if ( units.IsBigCellInside( v ) )
		push_back( v ); 
	return true; 
}

void CLineIter::GetNext()
{
	cells.pop_front();
	if ( !cells.empty() )
		vCurPoint = *cells.begin();
}

const bool CLineIter::IsFinished() const
{ 
	return cells.empty(); 
}

CLineIter::CLineIter( const CVec2 &vStart, const CVec2 &vFinish )
{
	const SVector vBigStart( AICellsTiles::GetBigCell( vStart.x, vStart.y ) );
	const SVector vBigEnd( AICellsTiles::GetBigCell( vFinish.x, vFinish.y ) );
	MakeLine2( vBigStart.x, vBigStart.y, vBigEnd.x, vBigEnd.y, cells );
	
	if ( !cells.empty() )
		vCurPoint = *cells.begin();
}

//*******************************************************************
//*													CPlanesIter															*
//*******************************************************************

CPlanesIter::CPlanesIter()
{
	iter = units.planes.begin();
	while ( iter != units.planes.end() && *iter == 0)
		++iter;
}

void CPlanesIter::Iterate()
{
 	++iter;
	while ( iter != units.planes.end() && *iter == 0 )
		++iter;
}

CAviation* CPlanesIter::operator*() const
{
	return *iter;
}

const bool CPlanesIter::IsFinished() const
{
	return iter == units.planes.end();
}

